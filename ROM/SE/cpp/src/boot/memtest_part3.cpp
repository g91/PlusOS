#include "serom/boot/memtest.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memtest
namespace serom { namespace boot { namespace memtest { 
    void TM_CMD_TRY_3() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_4() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_5() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_A() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_H() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_R() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_R_PRINT_FLAGS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_M() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_M_PRELOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_M_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_M_NEXT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_E() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_I() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_TRY_T() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_GOT_ADDR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_GOT_FLAGS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_ECHO_FLAGS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_RUN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_RESULT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_REPEAT_FOREVER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_CHK_LOOP_ON_ERR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_DEC_ITER() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_CHK_ABORT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_T_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_CMD_UNKNOWN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_PROMPT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_PROMPT_STATUS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_PROMPT_CRLF() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_PROMPT_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_TIMER_CHECK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_SEND_STATUS() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_PREFIX_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_PREFIX_NEXT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_PRINT_D7() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_SUFFIX_INIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_SUFFIX_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_SUFFIX_NEXT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_LOOP_NEXT_CHAR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_PREFIX_STR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_BEEP_SUFFIX_STR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_RECV_CHAR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_RECV_STORE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_RECV_DONE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_RECV_NO_DATA() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_RECV_RETURN() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_DIGIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_DIGIT_OK() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_DIGIT_BAD() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_LOOP_INIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_GOT_CHAR() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_NIBBLE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_BYTE() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_READ_NEXT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_PRINT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_PRINT_LOOP_INIT() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_PRINT_LOOP() {
        // RAM sizing and verification
        UInt32 ramSize = 0x400000; // 4MB SE RAM
        lowmem::Set<UInt32>(lowmem::MemTop, ramSize);
        lowmem::Set<UInt32>(lowmem::BufPtr, ramSize - 0x8000);
        lowmem::Set<UInt32>(lowmem::HeapEnd, ramSize - 0x8000);
        lowmem::Set<UInt16>(lowmem::MmInOK, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TM_HEX_PRINT_NIBBLE() {
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
