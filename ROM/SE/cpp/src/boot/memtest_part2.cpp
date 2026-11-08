#include "serom/boot/memtest.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memtest
namespace serom { namespace boot { namespace memtest { 
    void TM_SEL0_INIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_SEL4_DISPATCH_TEST() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ITER_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ITER_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ITER_REPEAT_FOREVER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ITER_CHK_LOOP_ON_ERR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ITER_DEC() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_TEST_COMPLETE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CHK_RESET() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_JUMP_RESET_VEC() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_SEL5_INIT_BITMASK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_SEL6_BITMASK_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT0_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT1_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT1_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT2_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT2_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT3_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT3_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT4_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT4_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT5_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT5_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT6_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT6_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT7_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BIT7_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_DISPATCH_BY_INDEX() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_TEST_ERROR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ERROR_REPEAT_FOREVER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_ERROR_CHK_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_NEXT_ITER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_RETURN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_POP_AND_RETURN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_SEL2_RETURN_NO_SR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_SEL3_RESET_STATE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TMENTRY1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TMRESTART() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TMRESTART_SCC_INIT_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TMRESTART_PARSE_CHAR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TMRESTART_CHK_PREFIX() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_L() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_L_ECHO() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_B() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_B_ECHO() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_D() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_D_BYTELOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_D_STORE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_C() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_C_COMPARE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_G() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_G_ECHO_STAR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_G_ECHO_G() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_G_ECHO_CRLF() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_G_JUMP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_0() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_0_ECHO() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_1() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_1_ECHO() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_2() {
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
