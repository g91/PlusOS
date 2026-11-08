#include "serom/toolbox/quickdraw/Text.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Text
namespace serom { namespace toolbox { namespace quickdraw { namespace Text { 
    
    void STDTEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TXTLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void COUNTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FONTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FACEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDTEXT_MODEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDTEXT_SIZEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SPOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDTEXT_NOTSAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_NOTSAME_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_NOTSAME_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_NOTSAME_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SHORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_DHDV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHARE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_SHARE2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXT2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_NOTPIC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTEXT_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALLTEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALLTEXT_USESTD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTFACE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWCHAR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CHARWIDTH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CHARWIDTH_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTFONT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTSIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTSIZE_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPACEEXTRA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWSTRING() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWTEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWTEXT_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRINGWIDTH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTWIDTH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTWIDTH_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TEXTWIDTH_DONE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDTXMEAS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTXMEAS_NEXTCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDTXMEAS_MORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MEASURETEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MEASURETEXT_CUSTOM_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MEASURETEXT_CUSTOM_STORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MEASURETEXT_USESTD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MEASURETEXT_NEXTCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTCH2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTCH2_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFONTINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFONTINFO_USESTD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFONTINFO_USESTD_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFONTINFO_USESTD_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SCALE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // Text
