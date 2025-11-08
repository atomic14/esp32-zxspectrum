#include "../Config.h"
#include "../FileLog.h"
#include "../Screens/EmulatorScreen/Machine.h"
#include "DebuggerData.h"

/**
 * Breakpoint that will last only until the next interrupt
 */
void DebuggerData::setTempBreakpoint(uint no, uint16_t addr)
{
    FileLog fl;
    if (no > 1){
        return;
    }
    temp_bkp[no].address = addr;
    // Save the byte at address
    temp_bkp[no].saved_byte = Config::getConfig().speccy->z80_peek(addr);
    // Put RST instead
    Config::getConfig().speccy->z80_poke(addr, BKP_INST);
    fl.log("setTmpBkp %d: %04X %02X %02x", no, addr, temp_bkp[no].saved_byte,
        Config::getConfig().speccy->z80_peek(addr));
}

uint8_t DebuggerData::addBreakpoint(uint16_t addr){
    FileLog fl;
    char vrt[10] = "--VIRT";
    ZXSpectrum *speccy = Config::getConfig().speccy;
    Breakpoint *bkp = new Breakpoint();
    bkp->address = addr;
    bkp->saved_byte = speccy->z80_peek(addr);
    breakpoints[addr] = bkp;
    // Check if this breakpoint would be at the current address
    // In that case make it a virtual breakpoint
    if (addr == speccy->z80Regs->PC.W){
        speccy->z80Regs->virtual_breakpoint = addr;
        vrt[0] = '+';
    } else {
        // Normal breakpoint - just change the instruction
        speccy->z80_poke(addr, BKP_INST);
        vrt[0] = 0;
    }
    fl.log("addBkp %04X %02X %02x %s", addr, bkp->saved_byte,
        Config::getConfig().speccy->z80_peek(addr), vrt);
    return bkp->saved_byte;
}

/**
 * Return true if one of the breakpoints is hit
 */
bool DebuggerData::breakpointHit(uint16_t addr)
{
    if (breakpoints.contains(addr)){
        return true;
    }
    if (temp_bkp[0].address == addr || temp_bkp[1].address == addr){
        return true;
    }
    return false;
}

/**
 * Clean all temporary breakpoints
 */
void DebuggerData::clearTempBreakpoints()
{
    FileLog fl;
    restoreMem(temp_bkp[0]);
    temp_bkp[0].address = 0;
    restoreMem(temp_bkp[1]);
    temp_bkp[1].address = 0;
    fl.log("Clr tmp bkp");
}

void DebuggerData::restoreMem(Breakpoint bkp)
{
    Config::getConfig().speccy->z80_poke(bkp.address, bkp.saved_byte);
}