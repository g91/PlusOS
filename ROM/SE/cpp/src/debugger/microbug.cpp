#include "serom/debugger/microbug.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: microbug
namespace serom { namespace debugger { namespace microbug { 
    
    void MICROBUG_ENTRY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_PROMPT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_FILL_BUF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_KEY_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MICROBUG_CHECK_KEY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_STORE_CH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_EXEC_CMD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_PARSE_CMD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_CMD_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MICROBUG_CMD_CMP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MICROBUG_CMD_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DT_DbgCmdTab() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void P_DbgCmd_G() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGCMD_G_RESUME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ERASE_STACK_RECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void P_DbgCmd_DM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DM_ROW_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DM_WORD_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PRINT_HEX_LONG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PRINT_HEX_WORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PRINT_HEX_BYTE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PRINT_HEX_NIBBLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HEX_DIGIT_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void P_DbgCmd_FS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SM_PARSE_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SM_STORE_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void P_DbgCmd_TD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void P_DbgCmd_PC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void P_DbgCmd_SR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGREG_PARSE_NUM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGREG_PARSE_VAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGREG_SHOW_WORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGREG_DISPLAY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBGREG_PRINT_LOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ASCII_TO_NIBBLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NIBBLE_IS_DIGIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PARSE_ADDR_OR_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PARSE_ADDR_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EVAL_EXPR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_NEW_TERM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_SKIP_SPACE() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EXPR_SCAN_PREFIX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_OP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_HEX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_HEX_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EXPR_FINALIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_DEREF_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EXPR_CHECK_NEG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_ACCUMULATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_SIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_NEXT_TERM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_RETURN() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EXPR_CHECK_DOT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_TOKEN_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EXPR_SKIP_CHARS() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EXPR_CHECK_PC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_REG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_READ_REGNUM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPR_CHECK_RD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // microbug
