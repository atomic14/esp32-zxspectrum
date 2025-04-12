#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <map>
#include "Messages/Message.h"
#include "Transport.h"

#define FRAME_BYTE 0x7E
#define ESCAPE_BYTE 0x7D
#define ESCAPE_MASK 0x20

// Reads and write HDLC packets to and from a transport layer
// Handles the HDLC framing and escape sequences along with the packet type and length
class MessageReciever;
class PacketHandler
{
private:
  // create an enum to handle our state machine for decoding packets
  enum class State
  {
    // we're waiting for the start of a packet
    WAITING_FOR_START_BYTE,
    // we've started a packet and are waiting for the message type
    READING_COMMAND_TYPE,
    // we've read the message type and are waiting for the message length
    READING_COMMAND_LENGTH,
    // we've read the message length and are waiting for the data
    READING_DATA,
    // we've read the data and are waiting for the CRC
    READING_CRC,
    // we've read the CRC and are expecting the frame byte next
    EXPECTING_FRAME_BYTE
  };

  static constexpr uint16_t PACKET_DATA_BUFFER_SIZE = 1024;

  State state = State::WAITING_FOR_START_BYTE;

  MessageId messageType = MessageId::NullResponse;
  uint32_t messageLength = 0;
  uint16_t lengthByteCount = 0;
  uint32_t receivedCrc = 0;
  uint16_t receivedCrcLength = 0;
  uint32_t calculatedCrc = 0;
  uint8_t dataBuffer[PACKET_DATA_BUFFER_SIZE];
  uint16_t bufferPosition = 0;
  uint32_t totalRead = 0;
  bool escapeNextByte = false;

  // packet handlers by packet type
  std::map<MessageId, MessageReciever *> messageHandlers;

public:
  PacketHandler(Transport &transport)
      : transport(transport)
  {
    generateCRCTable();
  }

  void registerMessageHandler(MessageReciever *messageReciever, MessageId type)
  {
    messageHandlers[type] = messageReciever;
  }

  State processWaitingForStartByte(uint8_t data)
  {
    // we only expect to see the frame byte - anything else is ignored
    if (data == FRAME_BYTE && state == State::WAITING_FOR_START_BYTE)
    {
      return State::READING_COMMAND_TYPE;
    }
    return State::WAITING_FOR_START_BYTE;
  }

  State processExpectingFrameByte(uint8_t data)
  {
    receivedCrc = finalizeCRC32(receivedCrc);
    bool isValid = receivedCrcLength == 4 && calculatedCrc == receivedCrc && data == FRAME_BYTE;
    if (messageHandlers.find(messageType) != messageHandlers.end())
    {
      messageHandlers[messageType]->messageFinished(isValid);
    }
    resetState();
    return State::WAITING_FOR_START_BYTE;
  }

  State processReadingMessageType(uint8_t data)
  {
    messageType = MessageId(data);
    lengthByteCount = 0;
    messageLength = 0;
    receivedCrc = 0;
    receivedCrcLength = 0;
    totalRead = 0;
    totalRead = 0;
    bufferPosition = 0;
    calculatedCrc = 0xFFFFFFFF;
    // update the calculated CRC to include the packet type
    calculatedCrc = updateCRC32(calculatedCrc, data);
    return State::READING_COMMAND_LENGTH;
  }

  State processReadingMessageLength(uint8_t data)
  {
    // update the calculated CRC to include the length bytes
    calculatedCrc = updateCRC32(calculatedCrc, data);
    // read the packet length - 4 bytes little endian
    messageLength |= (data << (lengthByteCount * 8));
    lengthByteCount++;
    if (lengthByteCount < 4)
    {
      return State::READING_COMMAND_LENGTH;
    }
    // we've got the start of a packet - along with the type and length
    if (messageHandlers.find(messageType) != messageHandlers.end())
    {
      messageHandlers[messageType]->messageStart(messageLength);
    }
    totalRead = 0;
    bufferPosition = 0;
    return messageLength > 0 ? State::READING_DATA : State::READING_CRC;
  }

  State processReadingData(uint8_t data)
  {
    if (totalRead < messageLength)
    {
      // still reading data
      // update the calculated CRC to include the data
      calculatedCrc = updateCRC32(calculatedCrc, data);
      // we're reading data
      dataBuffer[bufferPosition++] = data;
      totalRead++;
      // buffer is full or we've read all the data
      if (bufferPosition >= PACKET_DATA_BUFFER_SIZE || totalRead == messageLength)
      {
        if (messageHandlers.find(messageType) != messageHandlers.end())
        {
          messageHandlers[messageType]->messageData(dataBuffer, bufferPosition);
        }
        bufferPosition = 0;
      }
    }
    // is there still data to read?
    if (totalRead < messageLength)
    {
      return State::READING_DATA;
    }
    // we've got all the data - now read the CRC
    return State::READING_CRC;
  }

  State processReadingCRC(uint8_t data)
  {
    receivedCrc |= (data << (receivedCrcLength * 8));
    receivedCrcLength++;
    if (receivedCrcLength < 4)
    {
      return State::READING_CRC;
    }
    // we've got the CRC - now we expect the frame byte
    return State::EXPECTING_FRAME_BYTE;
  }

  void loop()
  {
    while (transport.available())
    {
      uint8_t data = transport.read();
      // are we waiting for the start of a packet?
      if (state == State::WAITING_FOR_START_BYTE)
      {
        state = processWaitingForStartByte(data);
      }
      else if (state == State::EXPECTING_FRAME_BYTE)
      {
        state = processExpectingFrameByte(data);
      }
      else
      {
        // if we get the frame byte then something has gone wrong
        if (data == FRAME_BYTE)
        {
          if (messageHandlers.find(messageType) != messageHandlers.end())
          {
            messageHandlers[messageType]->messageFinished(false);
          }
          resetState();
          state = State::WAITING_FOR_START_BYTE;
          continue;
        }
        // handle escape sequences
        if (escapeNextByte)
        {
          data ^= ESCAPE_MASK;
          escapeNextByte = false;
        }
        else if (data == ESCAPE_BYTE)
        {
          escapeNextByte = true;
          continue;
        }
        // hanndle the packet state machine
        switch (state)
        {
        case State::READING_COMMAND_TYPE:
          state = processReadingMessageType(data);
          break;
        case State::READING_COMMAND_LENGTH:
          state = processReadingMessageLength(data);
          break;
        case State::READING_DATA:
          state = processReadingData(data);
          break;
        case State::READING_CRC:
          state = processReadingCRC(data);
          break;
        default:
          // should not reach here unless something went wrong
          resetState();
          state = State::WAITING_FOR_START_BYTE;
        }
      }
    }
  }

  void resetState() {
    lengthByteCount = 0;
    messageLength = 0;
    receivedCrc = 0;
    receivedCrcLength = 0;
    totalRead = 0;
    totalRead = 0;
    bufferPosition = 0;
    calculatedCrc = 0xFFFFFFFF;
  }

  /**
   * Send a packet to the transport layer
   * @param type The packet type
   * @param data The packet data
   * @param length The length of the packet data
   *
   * Packets are framed with a start byte, followed by the packet type, length, data and a CRC
   * The CRC is calculated over the packet type, length and data
   */
  void sendPacket(MessageId type, const uint8_t *data, uint32_t length)
  {
    transport.write(uint8_t(FRAME_BYTE));
    uint32_t crc = write(uint8_t(type));
    crc = write(length, crc);
    crc = write(data, length, crc);
    crc = finalizeCRC32(crc);
    write(crc);
    transport.write(uint8_t(FRAME_BYTE));
    transport.flush();
  }

  /**
   * Update the CRC32 value
   * @param crc The current CRC value
   * @param byte The byte to update the CRC with
   * @return The updated CRC value
   */
  uint32_t updateCRC32(uint32_t crc, uint8_t byte)
  {
    uint8_t lookupIndex = (crc ^ byte) & 0xFF;
    return (crc >> 8) ^ crc32Table[lookupIndex];
  }

  /**
   * Finalize the CRC32 value
   * @param crc The current CRC value
   * @return The finalized CRC value
   */
  uint32_t finalizeCRC32(uint32_t crc)
  {
    return crc ^ 0xFFFFFFFF;
  }

  uint32_t calculateCRC32(uint8_t *data, uint32_t length)
  {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++)
    {
      crc = updateCRC32(crc, data[i]);
    }
    return finalizeCRC32(crc);
  }

private:
  /**
   * Write a byte to the transport layer
   * @param data The byte to write
   * @param crc The current CRC value
   * @return The updated CRC value
   */
  uint32_t write(uint8_t data, uint32_t crc = 0xFFFFFFFF)
  {
    writeEscaped(data);
    return updateCRC32(crc, data);
  }

  /**
   * Write a 32 bit value to the transport layer
   * @param value The 32 bit value to write
   * @param crc The current CRC value
   * @return The updated CRC value
   */
  uint32_t write(uint32_t value, uint32_t crc = 0xFFFFFFFF)
  {
    return write((const uint8_t *)&value, sizeof(uint32_t), crc);
  }

  /**
   * Write a block of data to the transport layer escaping if necessary
   * @param data The data to write
   * @param length The length of the data
   * @param crc The current CRC value
   * @return The updated CRC value
   */
  uint32_t write(const uint8_t *data, uint32_t length, uint32_t crc = 0xFFFFFFFF)
  {
    for (uint32_t i = 0; i < length; i++)
    {
      crc = write(data[i], crc);
    }
    return crc;
  }

  /**
   * Write a byte to the transport layer escaping if necessary
   * @param data The byte to write
   */
  void writeEscaped(uint8_t data)
  {
    if (data == FRAME_BYTE || data == ESCAPE_BYTE)
    {
      transport.write(ESCAPE_BYTE);
      transport.write(data ^ ESCAPE_MASK);
    }
    else
    {
      transport.write(data);
    }
  }

  /**
   * Generate the CRC32 table
   */
  uint32_t crc32Table[256];

  // Generate CRC32 Lookup Table
  void generateCRCTable()
  {
    for (uint32_t i = 0; i < 256; i++)
    {
      uint32_t crc = i;
      for (uint8_t j = 0; j < 8; j++)
      {
        if (crc & 1)
        {
          crc = (crc >> 1) ^ 0xEDB88320;
        }
        else
        {
          crc = crc >> 1;
        }
      }
      crc32Table[i] = crc;
    }
  }

  /**
   * The underlying transport layer
   */
  Transport &transport;
};
