const FRAME_BYTE = 0x7e;
const ESCAPE_BYTE = 0x7d;
const ESCAPE_MASK = 0x20;
const PACKET_DATA_BUFFER_SIZE = 256;

// Basic interface for a transport layer
interface Transport {
  read(): Promise<number | null>; // Read a byte (async for browser environments)
  write(data: number): void;      // Write a byte
}

enum State {
  WAITING_FOR_START_BYTE,
  READING_COMMAND_TYPE,
  READING_COMMAND_LENGTH,
  READING_DATA,
  READING_CRC,
  EXPECTING_FRAME_BYTE,
}

class CommandHandler {
  private state: State = State.WAITING_FOR_START_BYTE;

  private commandType: number = 0;
  private commandLength: number = 0;
  private lengthByteCount: number = 0;
  private receivedCrc: number = 0;
  private receivedCrcLength: number = 0;
  private calculatedCrc: number = 0;
  private dataBuffer: Uint8Array = new Uint8Array(PACKET_DATA_BUFFER_SIZE);
  private bufferPosition: number = 0;
  private totalRead: number = 0;
  private escapeNextByte: boolean = false;

  private commandStartHandlers: Map<number, (length: number) => void> = new Map();
  private commandDataHandlers: Map<number, (data: Uint8Array, length: number) => void> = new Map();
  private commandFinishedHandlers: Map<number, (isValid: boolean) => void> = new Map();

  private crc32Table: Uint32Array = new Uint32Array(256);

  constructor(private transport: Transport) {
    this.generateCRCTable();
  }

  registerCommandHandler(
    type: number,
    startCallback: (length: number) => void,
    dataCallback: (data: Uint8Array, length: number) => void,
    finishedCallback: (isValid: boolean) => void
  ) {
    if (startCallback) this.commandStartHandlers.set(type, startCallback);
    if (dataCallback) this.commandDataHandlers.set(type, dataCallback);
    if (finishedCallback) this.commandFinishedHandlers.set(type, finishedCallback);
  }

  private async process() {
    while (true) {
      const data = await this.transport.read();
      if (data === null) break;

      switch (this.state) {
        case State.WAITING_FOR_START_BYTE:
          this.state = this.processWaitingForStartByte(data);
          break;
        case State.EXPECTING_FRAME_BYTE:
          this.state = this.processExpectingFrameByte(data);
          break;
        case State.READING_COMMAND_TYPE:
          this.state = this.processReadingCommandType(data);
          break;
        case State.READING_COMMAND_LENGTH:
          this.state = this.processReadingCommandLength(data);
          break;
        case State.READING_DATA:
          this.state = this.processReadingData(data);
          break;
        case State.READING_CRC:
          this.state = this.processReadingCRC(data);
          break;
        default:
          this.state = State.WAITING_FOR_START_BYTE;
      }
    }
  }

  private processWaitingForStartByte(data: number): State {
    return data === FRAME_BYTE ? State.READING_COMMAND_TYPE : State.WAITING_FOR_START_BYTE;
  }

  private processExpectingFrameByte(data: number): State {
    this.receivedCrc = this.finalizeCRC32(this.receivedCrc);
    const isValid = this.receivedCrcLength === 4 && this.calculatedCrc === this.receivedCrc && data === FRAME_BYTE;
    this.commandFinishedHandlers.get(this.commandType)?.(isValid);
    return State.WAITING_FOR_START_BYTE;
  }

  private processReadingCommandType(data: number): State {
    this.commandType = data;
    this.lengthByteCount = 0;
    this.commandLength = 0;
    this.receivedCrc = 0;
    this.receivedCrcLength = 0;
    this.totalRead = 0;
    this.calculatedCrc = 0xffffffff;
    this.calculatedCrc = this.updateCRC32(this.calculatedCrc, data);
    return State.READING_COMMAND_LENGTH;
  }

  private processReadingCommandLength(data: number): State {
    this.calculatedCrc = this.updateCRC32(this.calculatedCrc, data);
    this.commandLength |= data << (this.lengthByteCount * 8);
    this.lengthByteCount++;

    if (this.lengthByteCount < 4) return State.READING_COMMAND_LENGTH;

    this.commandStartHandlers.get(this.commandType)?.(this.commandLength);
    this.totalRead = 0;
    this.bufferPosition = 0;
    return State.READING_DATA;
  }

  private processReadingData(data: number): State {
    if (this.totalRead < this.commandLength) {
      this.calculatedCrc = this.updateCRC32(this.calculatedCrc, data);
      this.dataBuffer[this.bufferPosition++] = data;
      this.totalRead++;

      if (this.bufferPosition >= PACKET_DATA_BUFFER_SIZE || this.totalRead === this.commandLength) {
        this.commandDataHandlers.get(this.commandType)?.(
          this.dataBuffer.subarray(0, this.bufferPosition),
          this.bufferPosition
        );
        this.bufferPosition = 0;
      }
    }

    return this.totalRead < this.commandLength ? State.READING_DATA : State.READING_CRC;
  }

  private processReadingCRC(data: number): State {
    this.receivedCrc |= data << (this.receivedCrcLength * 8);
    this.receivedCrcLength++;
    return this.receivedCrcLength < 4 ? State.READING_CRC : State.EXPECTING_FRAME_BYTE;
  }

  private generateCRCTable() {
    for (let i = 0; i < 256; i++) {
      let crc = i;
      for (let j = 0; j < 8; j++) {
        if (crc & 1) {
          crc = (crc >>> 1) ^ 0xedb88320;
        } else {
          crc >>>= 1;
        }
      }
      this.crc32Table[i] = crc >>> 0;
    }
  }

  private updateCRC32(crc: number, byte: number): number {
    const lookupIndex = (crc ^ byte) & 0xff;
    return (crc >>> 8) ^ this.crc32Table[lookupIndex];
  }

  private finalizeCRC32(crc: number): number {
    return crc ^ 0xffffffff;
  }

  async sendPacket(type: number, data: Uint8Array) {
    this.transport.write(FRAME_BYTE);
    let crc = this.writeEscaped(type);
    crc = this.writeEscaped(data.length & 0xff, crc);
    crc = this.writeEscaped((data.length >> 8) & 0xff, crc);
    crc = this.writeEscaped((data.length >> 16) & 0xff, crc);
    crc = this.writeEscaped((data.length >> 24) & 0xff, crc);
    for (const byte of data) {
      crc = this.writeEscaped(byte, crc);
    }
    crc = this.finalizeCRC32(crc);
    for (let i = 0; i < 4; i++) {
      this.writeEscaped((crc >> (i * 8)) & 0xff);
    }
    this.transport.write(FRAME_BYTE);
  }

  private writeEscaped(byte: number, crc: number = 0xffffffff): number {
    if (byte === FRAME_BYTE || byte === ESCAPE_BYTE) {
      this.transport.write(ESCAPE_BYTE);
      this.transport.write(byte ^ ESCAPE_MASK);
    } else {
      this.transport.write(byte);
    }
    return this.updateCRC32(crc, byte);
  }
}
