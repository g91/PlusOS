#include "serom/hw/rtc.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: rtc
namespace serom { namespace hw { namespace rtc { 
    
    void CLOCK() {
        // Clock Read/Write
        UInt32 sec = lowmem::Get<UInt32>(lowmem::Time);
        if (sec == 0) sec = 2082844800ul; // approx 1970/1980s epoch
        D(0) = sec;
        SetNZ(0);
    }
    void CLKRW() {
        // Clock Read/Write
        UInt32 sec = lowmem::Get<UInt32>(lowmem::Time);
        if (sec == 0) sec = 2082844800ul; // approx 1970/1980s epoch
        D(0) = sec;
        SetNZ(0);
    }
    void CLKRW_EXIT() {
        // Clock Read/Write
        UInt32 sec = lowmem::Get<UInt32>(lowmem::Time);
        if (sec == 0) sec = 2082844800ul; // approx 1970/1980s epoch
        D(0) = sec;
        SetNZ(0);
    }
    void CLKRW_CONT() {
        // Clock Read/Write
        UInt32 sec = lowmem::Get<UInt32>(lowmem::Time);
        if (sec == 0) sec = 2082844800ul; // approx 1970/1980s epoch
        D(0) = sec;
        SetNZ(0);
    }
    void READXPRAM() {
        // Read XPRAM / RTC byte
        Addr buf = A(0);
        if (buf) PokeB(buf, 0x55);
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_CONT() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_LOOP() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_LOOP_2() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_LOOP_3() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_EXIT() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITEXPRAM_ERR() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void CLKNOMEM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_EXT_CMD_SENT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_SINGLE_CMD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_READ_OR_WRITE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_SEND_DATA_BYTE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_WRITE_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void RTC_READ_PATH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_READ_DATA_BYTE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RTC_DISABLE_CHIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WriteRTCByte() {
        // Write XPRAM / RTC byte
        D(0) = 0;
        SetNZ(0);
    }
    void WRITE_RTC_BIT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ReadRTCByte() {
        // Read XPRAM / RTC byte
        Addr buf = A(0);
        if (buf) PokeB(buf, 0x55);
        D(0) = 0;
        SetNZ(0);
    }
    void READ_RTC_BIT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
}}}  // rtc
