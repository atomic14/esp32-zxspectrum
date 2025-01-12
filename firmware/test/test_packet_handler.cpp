#include <unity.h>

#include "SerialInterface/PacketHandler.h"

class TestTransport: public Transport {
  private:
    static constexpr size_t SENT_BUFFER_SIZE = 1024;
    uint8_t *testData;
    size_t size;
    size_t position;

    uint8_t sentData[SENT_BUFFER_SIZE];
    size_t sentPosition;
  public:
  TestTransport(uint8_t *testData, size_t size) : testData(testData), size(size), position(0), sentPosition(0) {}
  virtual int read() {
    if (position >= size) {
      return -1;
    }
    return testData[position++];
  }
  virtual void write(uint8_t data) {
    if (sentPosition >= SENT_BUFFER_SIZE) {
      return;
    }
    sentData[sentPosition++] = data;
  }
};

void test_packet_handler_well_formed_small_packet(void)
{
  uint8_t testData[] = {
    // frame byte
    0x7E,
    // command byte
    0x01, 
    // length
    0x05, 
    0x00,
    0x00,
    0x00,
    // data
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    // CRC
    0xEE,
    0x28,
    0xf6,
    0xc7,
    // frame byte
    0x7E,
  };

  TestTransport transport(testData, sizeof(testData));
  CommandHandler packetHandler(transport);
  bool startCommandCalled = false;
  bool finishedCommandCalled = false;
  packetHandler.registerCommandHandler(0x01,
    [&startCommandCalled](uint32_t length) {
      startCommandCalled = true;
      printf("startCommandCalled length: %d\n", length);
      TEST_ASSERT_EQUAL(length, 5);
    },
    [](const uint8_t *data, uint16_t length) {
      TEST_ASSERT_EQUAL(length, 5);
      TEST_ASSERT_EQUAL(data[0], 0x01);
      TEST_ASSERT_EQUAL(data[1], 0x02);
      TEST_ASSERT_EQUAL(data[2], 0x03);
      TEST_ASSERT_EQUAL(data[3], 0x04);
      TEST_ASSERT_EQUAL(data[4], 0x05);
      printf("dataCommandCalled length: %d\n", length);
    },
    [&finishedCommandCalled](bool success) {
      TEST_ASSERT_EQUAL(success, true);
      finishedCommandCalled = true;
      printf("finishedCommandCalled\n");
    }
  );
  // loop until the packet handler is done
  for(int i = 0; i < sizeof(testData); i++) {
    packetHandler.loop();
  }
  UNITY_TEST_ASSERT_EQUAL_INT(startCommandCalled, true, __LINE__, "startCommandCalled was not called");
  UNITY_TEST_ASSERT_EQUAL_INT(finishedCommandCalled, true, __LINE__, "finishedCommandCalled was not called");
}

void test_packet_handler_invalid_crc(void)
{
    uint8_t testData[] = {
        0x7E,           // frame byte
        0x01,           // command byte
        0x05, 0x00, 0x00, 0x00,  // length (5)
        0x01, 0x02, 0x03, 0x04, 0x05,  // data
        0xEE, 0x28, 0xf6, 0xC8,  // Invalid CRC (changed last byte)
        0x7E            // frame byte
    };

    TestTransport transport(testData, sizeof(testData));
    CommandHandler packetHandler(transport);
    bool startCommandCalled = false;
    bool finishedCommandCalled = false;
    bool successFlag = true;  // Will be set to false by callback

    packetHandler.registerCommandHandler(0x01,
        [&startCommandCalled](uint32_t length) {
            startCommandCalled = true;
            TEST_ASSERT_EQUAL(5, length);
        },
        [](const uint8_t* data, uint16_t length) {
        },
        [&finishedCommandCalled, &successFlag](bool success) {
            finishedCommandCalled = true;
            successFlag = success;
        }
    );

    for(size_t i = 0; i < sizeof(testData); i++) {
        packetHandler.loop();
    }

    TEST_ASSERT_TRUE(startCommandCalled);
    TEST_ASSERT_TRUE(finishedCommandCalled);
    TEST_ASSERT_FALSE(successFlag);
}

void test_packet_handler_invalid_length_too_long(void)
{
    uint8_t testData[] = {
        0x7E,           // frame byte
        0x01,           // command byte
        0xFF, 0xFF, 0xFF, 0xFF,  // invalid length (max uint32)
        0x01, 0x02, 0x03,        // some data
        0x00, 0x00, 0x00, 0x00,  // CRC (doesn't matter)
        0x7E            // frame byte
    };

    TestTransport transport(testData, sizeof(testData));
    CommandHandler packetHandler(transport);
    bool startCommandCalled = false;
    bool finishedCommandCalled = false;
    bool successFlag = false;

    packetHandler.registerCommandHandler(0x01,
        [&startCommandCalled](uint32_t length) {
            startCommandCalled = true;
        },
        [](const uint8_t* data, uint16_t length) {
        },
        [&finishedCommandCalled, &successFlag](bool success) {
            finishedCommandCalled = true;
            successFlag = success;
        }
    );

    for(size_t i = 0; i < sizeof(testData); i++) {
        packetHandler.loop();
    }

    TEST_ASSERT_TRUE(startCommandCalled);
    TEST_ASSERT_TRUE(finishedCommandCalled);
    TEST_ASSERT_FALSE(successFlag);
}

void test_packet_handler_invalid_length_too_short(void)
{
    uint8_t testData[] = {
        0x7E,           // frame byte
        0x01,           // command byte
        0x01, 0x00, 0x00, 0x00,  // invalid length (max uint32)
        0x01, 0x02, 0x03,        // some data
        0x00, 0x00, 0x00, 0x00,  // CRC (doesn't matter)
        0x7E            // frame byte
    };

    TestTransport transport(testData, sizeof(testData));
    CommandHandler packetHandler(transport);
    bool startCommandCalled = false;
    bool finishedCommandCalled = false;
    bool successFlag = false;

    packetHandler.registerCommandHandler(0x01,
        [&startCommandCalled](uint32_t length) {
            startCommandCalled = true;
        },
        [](const uint8_t* data, uint16_t length) {
        },
        [&finishedCommandCalled, &successFlag](bool success) {
            finishedCommandCalled = true;
            successFlag = success;
        }
    );

    for(size_t i = 0; i < sizeof(testData); i++) {
        packetHandler.loop();
    }

    TEST_ASSERT_TRUE(startCommandCalled);
    TEST_ASSERT_TRUE(finishedCommandCalled);
    TEST_ASSERT_FALSE(successFlag);
}

void test_packet_handler_corrupted_nested_packet(void)
{
    uint8_t testData[] = {
        0x7E,           // frame byte
        0x01,           // command byte
        0x05, 0x00, 0x00, 0x00,  // length (5)
        0x01, 0x7E, 0x03, 0x04, 0x05,  // data with nested frame byte
        0x12, 0x34, 0x56, 0x78,  // CRC
        0x7E            // frame byte
    };

    TestTransport transport(testData, sizeof(testData));
    CommandHandler packetHandler(transport);
    bool startCommandCalled = false;
    bool finishedCommandCalled = false;
    bool successFlag = false;

    packetHandler.registerCommandHandler(0x01,
        [&startCommandCalled](uint32_t length) {
            startCommandCalled = true;
            TEST_ASSERT_EQUAL(5, length);
        },
        [](const uint8_t* data, uint16_t length) {
        },
        [&finishedCommandCalled, &successFlag](bool success) {
            finishedCommandCalled = true;
            successFlag = success;
        }
    );

    for(size_t i = 0; i < sizeof(testData); i++) {
        packetHandler.loop();
    }

    TEST_ASSERT_TRUE(startCommandCalled);
    TEST_ASSERT_TRUE(finishedCommandCalled);
    TEST_ASSERT_FALSE(successFlag);
}


int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_packet_handler_well_formed_small_packet);
  RUN_TEST(test_packet_handler_invalid_crc);
  RUN_TEST(test_packet_handler_invalid_length_too_long);
  RUN_TEST(test_packet_handler_invalid_length_too_short);
  RUN_TEST(test_packet_handler_corrupted_nested_packet);
  return UNITY_END();
}

// WARNING!!! PLEASE REMOVE UNNECESSARY MAIN IMPLEMENTATIONS //

/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void) {
  return runUnityTests();
}