#pragma once

#include <map>
typedef unsigned short uint16_t;

// Pause reasons
#define PAUSE_NO_REASON         0
#define PAUSE_MANUAL            1
#define PAUSE_BREAKPOINT_HIT    2
#define PAUSE_WATCHPOINT_READ   3
#define PAUSE_WATCHPOINT_WRITE  4
// Just used as not set value
#define PAUSE_NOT_SET           200
#define PAUSE_OTHER             255

// The instruction used for breakpoints (RST 08)
#define BKP_INST                0xCF

template<typename T>
class AddrMap : public std::map<uint16_t, T>
{
public:
    bool contains(uint16_t addr) {
        try {
            if (this->at(addr)){
                return true;
            } else {
                return false;
            }
        } catch (const std::out_of_range& ex){
            return false;
        }
    }
};

class Breakpoint
{
public:
    uint16_t address;
    uint8_t saved_byte;
};

class DebuggerData 
{
protected:
    AddrMap<Breakpoint*> breakpoints;
    Breakpoint temp_bkp[2];

public:
    DebuggerData() = default;
    ~DebuggerData() = default;

    void setTempBreakpoint(uint no, uint16_t addr);
    void clearTempBreakpoints();
    uint8_t addBreakpoint(uint16_t addr);
    void removeBreakpoint(uint16_t addr);
    bool breakpointHit(uint16_t addr);
    void restoreMem(Breakpoint bkp);
};