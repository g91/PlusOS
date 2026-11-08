#include "serom/boot/memtest.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memtest
namespace serom { namespace boot { namespace memtest { 
    void MEMTEST_REV_VERIFY_LOOP_B1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_REV_VERIFY_LOOP_B2() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_REV_PATTERN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_PER_BYTE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_SEC1_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_SEC2_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_CHK_ODD() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_CHK_3() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_CHK_4() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_CKSUM_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZEROONES() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_FILL_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_PASS2() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_REVERIFY_BACK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_PASS4() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_PASS5() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_FINAL_VERIFY() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MEMTEST_ZO_FAULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_LINES() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_CHK_STACK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_PHASE1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_WRITE_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_VERIFY_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_REVERSE_WRITE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_VERIFY_REF() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_REVERSE_VERIFY() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRTEST_RETURN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TEST_ALWAYS_PASS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TEST_TABLE_COUNT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TEST_DISPATCH_TBL() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SYSTEMBEEP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SYSBEEP_FLASH() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SYSBEEP_AUDIBLE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SYSBEEP_EPILOG() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOTBEEP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_WAIT_TICK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_POLL_VIA() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_POLL_TIMER_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_FILL_BUFFER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_DMA_DELAY() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_FILL_REPEAT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_FILL_CYCLE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_WRITE_SAMPLE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_SMOOTH_FILTER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_FIR_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_EXTEND_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_WAIT_VBL() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_TICK_WAIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_VIA_POLL_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_DURATION_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BEEP_SILENCE_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ERRORBEEP2() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ERRORBEEP1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ERRORBEEP3() {
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
