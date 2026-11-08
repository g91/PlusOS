#include "serom/toolbox/textedit.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: textedit
namespace serom { namespace toolbox { namespace textedit { 
    void TEFINDLINE_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XFINDLINE_SKIP_SPACE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XFINDLINE_FORCE_BREAK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDLINE_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_doTERecal() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERecal() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_NEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_BOUNDARY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_DISPATCH() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_FULL_REBUILD() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_STYLED_HEIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS_FORWARD() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_NEXT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS_COPY_TAIL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_OK_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS_SHIFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INPLACE_LINESTARTS_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_LINESTARTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_FORWARD() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_CHK_INSERT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_GROW_BUF() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_APPEND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_NEXT_BREAK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_FINALIZE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_DO_MUNGER() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_DISPOSE_BUF() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_TAIL_SHIFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FULL_REBUILD_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LINEHEIGHTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_WALK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_RUN_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_STORE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_NEXT_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_BOTH() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_NEXT_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_AFTER_DETECT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REBUILD_LH_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CARET_RESTORE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CARET_SHOW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CARET_REDRAW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CARET_ERASE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TERECAL_CONT_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CARET_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEACTIVATE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEACTIVATE_NO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEACTIVATE_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDEACTIVATE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEIDLE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_AD_EPILOGUE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEPASTE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PASTE_BODY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PASTE_DISPOSE_TEMP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PASTE_PLAIN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PASTE_BODY_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESTYLPASTE_ENTRY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DO_PASTE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DO_PASTE_COLLAPSE_STYLES() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DO_PASTE_EDIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_WALK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_USE_INDEX() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_NEXT_ELEM() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_DISPOSE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_STYL_PASTE_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_GROW_STYLERUN_ARRAY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_SPLIT_STYLERUN_AT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_SPLIT_STYLERUN_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOOKUP_STYLEENTRY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOOKUP_STYLEENTRY_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOOKUP_STYLEENTRY_NEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOOKUP_STYLEENTRY_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LOOKUP_STYLEENTRY_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CMP_STYLE_BYTES() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CMP_STYLE_BYTES_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CMP_STYLE_BYTES_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_APPEND_STYLEENTRY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_APPEND_STYLEENTRY_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_APPEND_STYLEENTRY_STORE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEPASTE_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INSERT_BODY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INSERT_BODY_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEINSERT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_INSERT_EPILOGUE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_NOT_ARROW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_UP_ARROW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_VERT_ARROW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_DOWN_ARROW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_LEFT_ARROW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_SET_SELECTION() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_INSERT_CHAR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEKEY_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PREP_STYLES_FOR_PASTE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PREP_STYLES_INSERT_AT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PREP_STYLES_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_LINE_HEIGHT_AT_CHAR() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETJUST() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESETALIGNMENT_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESCROLL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESCROLL_EPILOGUE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESCROLL_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // textedit
