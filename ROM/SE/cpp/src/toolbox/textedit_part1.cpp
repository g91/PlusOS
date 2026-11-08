#include "serom/toolbox/textedit.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: textedit
namespace serom { namespace toolbox { namespace textedit { 
    
    void GETMGREND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTEDIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEGETTEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEINIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDISPOSE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDISPOSE_FREE_STYLEREC() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEXTBOX() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_EMPTY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_CONT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_NO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_SKIP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TETEXTBOX_DISPOSE_TAIL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETTEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETTEXT_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RESIZE_STYLERECS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RESIZE_STYLERECS_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RESIZE_TEXTHANDLE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RESIZE_SYSERROR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INIT_STYLEREC() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_GET_FONT_INFO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETTEXT_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECALTEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECALTEXT_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_EXIT_4PARAM() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_EXIT_3PARAM() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_EXIT_BODY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_EXIT_TAIL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETSELECT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TENEW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESTYLNEW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RESIZE_AND_DEREF() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEUPDATE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEUPDATE_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEUPDATE_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEUPDATE_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_NO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_AFTER_ANCHOR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_UNHIGHLIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_EMPTY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_OK_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_SET_CARET() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_5() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_6() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_7() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_Pixel2Char() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void VPIXEL2CHAR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_HEAP_ALLOC() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_MEASURE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_SCAN_BACK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_FOUND_CHAR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_CLAMP_ZERO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_FINALIZE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_DISPOSE_BUF() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void VPIXEL2CHAR_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_ENTRY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_MULTI_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_RUN_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_CLAMP_START() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_MEASURE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_PAST_END() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_CR_CHECK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_LEFT_SIDE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_FINALIZE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PIXELTOCHAR_STY_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_DRAW_BEGIN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_NEXT_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_NORMAL_DRAW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_NEG_ASCENT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_USE_ASCENT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_DRAW_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_CLAMP_DRAW_END() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_USE_RUN_START() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_EXIT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDELETE_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_EXTEND_LEFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_ERASE_TAIL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOCATE_STYLERUNS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOCATE_STYLERUNS_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADVANCE_STYLERUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADVANCE_STYLERUN_LTR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADVANCE_STYLERUN_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_NEXT_VISLINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_BOUNDS_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_10() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_11() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_OK_5() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_CONT_10_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADVANCE_LINESTART() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_FOUND_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECLICK_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_doTEDoText() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDoText() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDOTEXT_NO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDOTEXT_NO_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_LEFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEHILITE_DRAW_ENTRY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEHILITE_END_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDOTEXT_LAST_PARTIAL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_BOUNDS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_BOUNDS_LAST_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_BOUNDS_CMP_SEL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // textedit
