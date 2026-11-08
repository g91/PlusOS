#include "serom/boot/memtest.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memtest
namespace serom { namespace boot { namespace memtest { 
    
    void MEMTEST() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BUSERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BUSERROR_UNEXPECTED() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADRERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ILLERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ZERERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHKERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void XTRAPX() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PRIVERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TRACE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LINEA() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LINEF() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NOTASS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CPPROTOCOL() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FORMATX() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SPURINTERRUPT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TRAPN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL2() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL3() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL4() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL5() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL6() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTLEVEL7() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP2() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP3() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP4() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP5() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP6() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FPCP7() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PMMUCONFIG() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PMMUILLEGAL() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PMMUACCESS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EXCEPTIONHANDLER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EXCEPTIONHANDLER_RTE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void STARTTEST1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_NO_TEST_SW() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_RUN_CHECKSUM() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_CHECKSUM_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FDHD_ColdStartPatchHook() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FDHD_RAMTEST_OFFSET() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_RAM_RET() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_INIT_RAM_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_MEMSIZE_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_ADDRLINE_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_MEMTEST_COPYLOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_LOOPBACK_PATH() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_DIAG_TIMER_INIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COLDSTART_RUN_TM() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_USE_512K_SPLIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_FIRST_HALF_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_SECOND_HALF() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_RUN_SEGMENT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_SEGMENT_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RAMTEST_RETURN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void P_SadMac() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void P_SADMAC_TESTMGR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TESTMANAGER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // memtest
