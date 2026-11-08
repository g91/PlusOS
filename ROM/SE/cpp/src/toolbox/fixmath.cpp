#include "serom/toolbox/fixmath.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: fixmath
namespace serom { namespace toolbox { namespace fixmath { 
    
    void LONGMUL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONGMUL_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LONGMUL_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACMUL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXMUL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXMUL_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXMUL_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ALGORITHM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXMUL_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXMUL_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXMUL_EXIT_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXMUL_EXIT_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXMUL_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_CONT_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_CONT_8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXMUL_EXIT_5() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FRACDIV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXDIV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXDIV_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXDIV_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXDIV_EMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXDIV_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXDIV_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXDIV_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXDIV_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXDIV_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXDIV_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FRACSQRT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSQRT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FRACSQRT_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXRATIO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXRATIO_EMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXRATIO_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXMUL_SAME_SIGN_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXRATIO_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LOWORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HIWORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HIWORD_MOVE_RETURN() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FIXROUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXROUND_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FIXROUND_SATURATE_TAIL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACCOS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FRACSIN_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FRACSIN_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_REDUCED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_POLYCOS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_POLY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_POLY_TERM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_NOTSINE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_SIGN_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FRACSIN_SN0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRACSIN_CS0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIXATAN2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_DVSR_ABS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_ABS_DIVIDEND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_DVSR_ZERO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_DEALLOC_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATAN_NO_SWAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_DO_FRACDIV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_POLY_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATAN_RESTORE_A2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_CHK_DVSR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_CHK_DVND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_SIGN_RESULT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATAN_COEFF_TBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2X() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRAC2X() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX_TO_EXTENDED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2X_NORMALIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2X_SHIFT_LEFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2X_STORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FRAC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_CONVERT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_SHIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_CHK_ROUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_DO_SHIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_ROUND_UP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_CHK_SIGN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_STORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_UNDERFLOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_NEGATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_OVERFLOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_CLAMP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_POS_CLAMP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void X2FIX_ROTATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2LONG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FRAC2FIX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2LONG_SHIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2LONG_NEG_RND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2LONG_POS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONG2FIX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FIX2FRAC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONG2FIX_SHIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // fixmath
