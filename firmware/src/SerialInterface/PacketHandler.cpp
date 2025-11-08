#include "FileLog.h"
#include "PacketHandler.h"
#include "Emulator/spectrum.h"
#include "Emulator/z80/z80.h"
#include "Screens/EmulatorScreen.h"
#include "Screens/EmulatorScreen/Machine.h"

#define DEZOG_START_BYTE (uint8_t)0xA5

#define CMD_INIT             1
#define CMD_CLOSE            2
#define CMD_GET_REGISTERS    3
#define CMD_SET_REGISTER     4
#define CMD_WRITE_BANK       5
#define CMD_CONTINUE         6
#define CMD_PAUSE            7
#define CMD_READ_MEM         8
#define CMD_WRITE_MEM        9
#define CMD_SET_SLOT         10
#define CMD_GET_TBBLUE_REG   11
#define CMD_SET_BORDER       12
#define CMD_SET_BREAKPOINTS  13
#define CMD_RESTORE_MEM      14
#define CMD_LOOPBACK         15
// Commands 16 to 19 are sprites commands, unlikely to be implemented
#define CMD_READ_PORT        20
#define CMD_WRITE_PORT       21
#define CMD_EXEC_ASM         22
#define CMD_INT_ON_OFF       23
#define CMD_CMD_ADD_BKPT     40
#define CMD_CMD_REM_BKPT     41
#define CMD_CMD_ADD_WATCH    42
#define CMD_CMD_REM_WATCH    43
#define CMD_READ_STATE       50
#define CMD_WRITE_STATE      51

// Notifications (only one for now)
#define NTF_PAUSE            1

#define MEM_PTR(emu, addr) ((emu)->mem.mappedMemory[(addr) >> 14]->data+((addr) & 0x3FFF))

typedef struct {
  uint16_t PC;
  uint16_t SP;
  uint16_t AF;
  uint16_t BC;
  uint16_t DE;
  uint16_t HL;
  uint16_t IX;
  uint16_t IY;
  uint16_t AF2;
  uint16_t BC2;
  uint16_t DE2;
  uint16_t HL2;
  uint8_t R;
  uint8_t I;
  uint8_t IM;
  uint8_t reserved;
  uint8_t n_slots;
  uint8_t slots[10];
} DeZogRegsStruct;

typedef union 
{
  DeZogRegsStruct regs;
  uint8_t bytes[sizeof(DeZogRegsStruct)];
} DeZogRegsUnion;

void PacketHandler::debuggerWrite(uint8_t byte)
{
  transport.write(byte);
}

void PacketHandler::debuggerWrite(uint8_t *data, uint16_t length)
{
  transport.write(data, length);
}

void PacketHandler::debuggerSendPacket(uint8_t seq_no, uint8_t *data, uint16_t data_length)
{
  FileLog fl;
  // Length is data size plus the seqno byte
  uint32_t length = data_length + 1;
  uint8_t bytes[4];
  bytes[0] = length & 0xFF;
  bytes[1] = (length >> 8) & 0xFF;
  bytes[2] = (length >> 16) & 0xFF;
  bytes[3] = (length >> 24) & 0xFF;
  debuggerWrite(DEZOG_START_BYTE);
  debuggerWrite(bytes, 4);
  debuggerWrite(seq_no);
  if (data_length) {
    debuggerWrite(data, data_length);
  }
  fl.log(" ->(%d) len:%x", seq_no, data_length);
  fl.logbytes(" ->[", "]", data, data_length);
}

void PacketHandler::debuggerSendMessage(uint8_t *data, uint16_t data_length)
{
  debuggerSendPacket(m_debuggerSeqNo, data, data_length);
}

void PacketHandler::debuggerSendNotification(uint8_t *data, uint16_t data_length)
{
  debuggerSendPacket(0, data, data_length);
}

// cmd 1
void PacketHandler::cmdInit()
{
  auto bl = BusyLight();
  // Stop the emulator
  config.machine->pause();
  config.machine->debuggerConnected = true;
  config.speccy->z80Regs->debugging = true;
  // *** Reply ***
  // Error
  dataBuffer[0] = 0;
  // Version 2.1.0
  dataBuffer[1] = 2;
  dataBuffer[2] = 1;
  dataBuffer[3] = 0;
  // Machine type 2 = 48K
  dataBuffer[4] = 2;
  // Machine name (asciiz)
  strcpy((char*)dataBuffer+5, HARDWARE_VERSION_STRING);
  int len = strlen((char*)dataBuffer+5) + 6;
  debuggerSendMessage(dataBuffer, len);
}

// cmd 2
void PacketHandler::cmdClose()
{
  // Nothing to do here. Just blink the light
  auto bl = BusyLight();
  config.machine->debuggerConnected = false;
  config.machine->resume();

  debuggerSendMessage(dataBuffer, 0);
}

// cmd 3
/**
 * Register order (offsets are for dataBuffer, not for reply)
 * PC, SP, AF, BC
 * DE, HL, IX, IY
 * AF2, BC2, DE2, HL2
 * +24 R
 * +25 I
 * +26 IM
 * +27 - reserved
 * +28 - Number of slots that follw
 * +29 - Slot[n] - bank number
 */
void PacketHandler::cmdGetRegisters()
{
  DeZogRegsUnion regs;
  Z80Regs *emu_regs = config.speccy->z80Regs;

  regs.regs.PC = emu_regs->PC.W;
  regs.regs.SP = emu_regs->SP.W;
  regs.regs.AF = emu_regs->AF.W;
  regs.regs.BC = emu_regs->BC.W;
  regs.regs.DE = emu_regs->DE.W;
  regs.regs.HL = emu_regs->HL.W;
  regs.regs.IX = emu_regs->IX.W;
  regs.regs.IY = emu_regs->IY.W;
  regs.regs.AF2 = emu_regs->AFs.W;
  regs.regs.BC2 = emu_regs->BCs.W;
  regs.regs.DE2 = emu_regs->DEs.W;
  regs.regs.HL2 = emu_regs->HLs.W;
  regs.regs.R = emu_regs->R.W;
  regs.regs.I = emu_regs->I;
  regs.regs.IM = emu_regs->IM;
  regs.regs.n_slots = 0;

  debuggerSendMessage(regs.bytes, sizeof(DeZogRegsStruct));
}

// cmd 4
void PacketHandler::cmdSetRegister()
{
  uint8_t reg_id = dataBuffer[0];
  uint16_t val = dataBuffer[1] | dataBuffer[2] << 8;
  ZXSpectrum *speccy = config.speccy;
  Z80Regs *regs = speccy->z80Regs;
  switch(reg_id){
    case 0:
      regs->PC.W = val;
      break;
    case 1:
      regs->SP.W = val;
      break;
    case 2:
      regs->AF.W = val;
      break;
    case 3:
      regs->BC.W = val;
      break;
    case 4:
      regs->DE.W = val;
      break;
    case 5:
      regs->HL.W = val;
      break;
    case 6:
      regs->IX.W = val;
      break;
    case 7:
      regs->IY.W = val;
      break;
    case 8:
      regs->AFs.W = val;
      break;
    case 9:
      regs->BCs.W = val;
      break;
    case 10:
      regs->DEs.W = val;
      break;
    case 11:
      regs->HLs.W = val;
      break;
    case 13:
      regs->IM = val;
      break;
    case 34:
      regs->R.W = val;
      break;
    case 35:
      regs->I = val;
      break;
  }
  // *** Reply is empty ***
  debuggerSendMessage(dataBuffer, 0);
}

// cmd 5
/**
 * Default banks (8K - 2000):
 * A - 4000
 * B - 6000
 * 
 * 4 - 8000
 * 5 - A000
 * 
 * 0 - C000
 * 1 - E000
 */
void PacketHandler::cmdWriteBank()
{
  auto bl = BusyLight();
  ZXSpectrum *speccy = config.speccy;
  if (speccy == nullptr){
    // Should not get here. But if it gets, then the debugger will lose connection
    return;
  }
  uint8_t bank = dataBuffer[0];
  uint16_t addr = 0;
  switch (bank){
    case 0xA:
      addr = 0x4000;
      break;
    case 0xB:
      addr = 0x6000;
      break;
    case 0x4:
      addr = 0x8000;
      break;
    case 0x5:
      addr = 0xA000;
      break;
    case 0x0:
      addr = 0xC000;
      break;
    case 0x1:
      addr = 0xE000;
      break;
    default:
      // Shouldn't get here for 48K
      return;
  };
  memcpy(MEM_PTR(speccy, addr), dataBuffer + 1, 0x2000);

  // *** Reply ***
  // 0 = No error
  dataBuffer[0] = 0;
  // 1 = Error string zero terminated - empty
  dataBuffer[1] = 0;
  debuggerSendMessage(dataBuffer, 2);
}

// cmd 6
void PacketHandler::cmdContinue()
{
  FileLog fl;
  fl.log("cmdContinue");

  // Check Bkp 1
  if (dataBuffer[0] == 1){
    // Enable bkp 1
    uint16_t addr = dataBuffer[1] | dataBuffer[2] << 8;
    config.machine->debuggerData.setTempBreakpoint(0, addr);
  }
  // Check Bkp 2
  if (dataBuffer[3] == 1){
    // Enable bkp 2
    uint16_t addr = dataBuffer[4] | dataBuffer[5] << 8;
    config.machine->debuggerData.setTempBreakpoint(1, addr);
  }
  // Not used for now
  uint8_t alt_cmd = dataBuffer[6];
  uint16_t step_over_start = dataBuffer[7] | dataBuffer[8] << 8;
  uint16_t step_over_end = dataBuffer[9] | dataBuffer[10] << 8;

  config.emulatorScreen->resume();

  // *** Reply ***
  debuggerSendMessage(dataBuffer, 0);
}

// cmd 7
void PacketHandler::cmdPause()
{
  // TODO - consider pausing just the z80 machine
  // (so that the renderer keeps showing the screen while debugging)
  config.emulatorScreen->pause();
  config.machine->pauseReason = PAUSE_MANUAL;

  // *** Reply ***
  debuggerSendMessage(dataBuffer, 0);
}

// cmd 8
void PacketHandler::cmdReadMem()
{
  FileLog fl;
  ZXSpectrum *speccy = config.speccy;
  uint16_t addr = dataBuffer[1] | dataBuffer[2] << 8;
  uint16_t len = dataBuffer[3] | dataBuffer[4] << 8;
  uint16_t ofs = 0;
  //fl.log("- ReadMem %X (len:%x)", addr, len);
  while (len > 0){
    // Get address of the next bank
    uint16_t naddr = (addr + 0x4000) & 0xC000;
    uint16_t tlen = naddr - addr;
    if (len < tlen) {
      tlen = len;
    }
    //fl.log("- chunk %X (len:%x)", addr, tlen);
    memcpy(dataBuffer+ofs, MEM_PTR(speccy, addr), tlen);
    len -= tlen;
    addr += tlen;
    ofs += tlen;
  }
  // By now len=0 and ofs=len
  debuggerSendMessage(dataBuffer, ofs);
}

// cmd 9
void PacketHandler::cmdWriteMem()
{

}

// cmd 10
void PacketHandler::cmdSetSlot()
{

  // *** Reply ***
  // No error
  dataBuffer[0] = 0;
  debuggerSendMessage(dataBuffer, 1);
}

// cmd 12
void PacketHandler::cmdSetBorder()
{
  ZXSpectrum *speccy = config.speccy;
  uint8_t b = speccy->z80_in(0xFE);
  b &= ~7;
  b |= (dataBuffer[0] & 7);
  speccy->z80_out(0xFE, b);
  // *** Reply ***
  debuggerSendMessage(dataBuffer, 0);
}

// cmd 13
void PacketHandler::cmdSetBreakpoints()
{
  int bkp_no = messageLength / 3;
  for (int i=0; i<bkp_no; i++){
    uint16_t addr = dataBuffer[i*3] | dataBuffer[i*3+1] << 8;
    // Get the bank, but ignore for now
    uint8_t bank = dataBuffer[i*3+2];
    // Set the breakpoint and also save the instructions in the buffer
    dataBuffer[i] = config.machine->debuggerData.addBreakpoint(addr);
  }

  // *** Reply ***
  // Data is already in the buffer
  debuggerSendMessage(dataBuffer, bkp_no);
}

// cmd 14
void PacketHandler::cmdRestoreMem()
{
  FileLog fl;
  int bkp_no = messageLength / 4;
  for (int i=0; i<bkp_no; i++){
    uint16_t addr = dataBuffer[i*4] | dataBuffer[i*4+1] << 8;
    // Get the bank, but ignore for now
    uint8_t bank = dataBuffer[i*4+2];
    // Get the original byte and patch back
    uint8_t byte = dataBuffer[i*4+3];
    fl.log("restore %02X at %04X (%02X)", byte, addr, config.speccy->z80_peek(addr));
    config.speccy->z80_poke(addr, byte);
  }

  // *** Reply ***
  debuggerSendMessage(dataBuffer, 0);
}

// cmd 23
void PacketHandler::cmdIntOnOff()
{
  // TODO - make it do something :)
  // *** Reply ***
  debuggerSendMessage(dataBuffer, 0);
}


/**
 * DeZog packets:
 * 4 bytes - length (LE)
 * 1 byte - sequence no
 * 1 byte - cmd id
 * n bytes - data (command dependent)
 */
void PacketHandler::loop_dezog()
{
  FileLog fl;

  // TODO - check for incomplete frames (e.g. when disconnected)
  while (transport.available()){
    uint32_t length = 0;
    uint8_t cmd_id = 0;
    for (int i=0; i<4; i++){
      uint8_t b = transport.read();
      length |= b << (i*8);
    }
    m_debuggerSeqNo = transport.read();
    messageLength = length;
    cmd_id = transport.read();

    fl.log("~%d (%d), len:0x%x", cmd_id, m_debuggerSeqNo, length);

    uint32_t length_read = transport.read(dataBuffer, length);
    fl.logbytes("  *[", "]", dataBuffer, length);

    if (length_read != length) {
      // Couldn't read everything, just ignore
      // TODO - could this desynchronize things?
      fl.log("Ignored");
      continue;
    }
    switch (cmd_id){
      case CMD_INIT:
        cmdInit();
        break;
      case CMD_CLOSE:
        cmdClose();
        break;
      case CMD_GET_REGISTERS:
        cmdGetRegisters();
        break;
      case CMD_SET_REGISTER:
        cmdSetRegister();
        break;
      case CMD_WRITE_BANK:
        cmdWriteBank();
        break;
      case CMD_CONTINUE:
        cmdContinue();
        break;
      case CMD_PAUSE:
        cmdPause();
        break;
      case CMD_READ_MEM:
        cmdReadMem();
        break;
      case CMD_SET_SLOT:
        cmdSetSlot();
        break;
      case CMD_SET_BORDER:
        cmdSetBorder();
        break;
      case CMD_SET_BREAKPOINTS:
        cmdSetBreakpoints();
        break;
      case CMD_RESTORE_MEM:
        cmdRestoreMem();
        break;
      case CMD_LOOPBACK:
        debuggerSendMessage(dataBuffer, length);
        break;
      case CMD_INT_ON_OFF:
        cmdIntOnOff();
        break;
      default:
        // TODO - find a way to report a not implemented command
        fl.log(" ** Cmd:%d not implemented", cmd_id);
        break;
    }
    vTaskDelay(0);
  }
  /**
   * If we're here there are no commands to read.
#include "../FileLog.h"
   * Check if we are stopped and need to send a pause reason
   */
  Machine *machine = config.machine;
  if (machine && !machine->isRunning && machine->pauseReason != PAUSE_NOT_SET){
    fl.log(" =>Ntf: %d", machine->pauseReason);
    // Send DeZog pause notification
    uint16_t pc = config.speccy->z80Regs->PC.W;

    dataBuffer[0] = NTF_PAUSE;
    /**
     * Break reason:
     *  0 = no reason (e.g. a step-over)
     *  1 = manual break
     *  2 = breakpoint hit
     *  3 = watchpoint hit read access
     *  4 = watchpoint hit write access
     *  255 = some other reason: the reason string might have useful information for the user
     */
    dataBuffer[1] = machine->pauseReason;
    // Bkp/Wp address
    dataBuffer[2] = pc & 0xFF;
    dataBuffer[3] = pc >> 8;
    // Bank
    dataBuffer[4] = 0;
    strcpy((char*)dataBuffer+5, "PAUSE");
    debuggerSendNotification(dataBuffer, 11);
    // Reset pause reason
    machine->pauseReason = PAUSE_NOT_SET;
  }
}