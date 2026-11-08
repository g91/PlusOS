#include "serom/toolbox/textedit.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: textedit
namespace serom { namespace toolbox { namespace textedit { 
    void TEDOTEXT_LINE_BOUNDS_NORMAL_LEFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_LINE_BOUNDS_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_ADV_PAST_LINES() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDOTEXT_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_INIT_SCAN_STATE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_APPLY_JUST() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_APPLY_JUST_MEASURE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_JUST_FULL_OFFSET() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_JUST_STORE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_TOTAL_HEIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_TOTAL_HEIGHT_SIMPLE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_TOTAL_HEIGHT_WALK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_TOTAL_HEIGHT_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_TOTAL_HEIGHT_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CUR_LINE_HEIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_CUR_LINE_HEIGHT_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEDOTEXT_CONT_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_XTrimMeasure() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_OK_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_PLAIN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_RUN_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CLAMP_END() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_EMPTY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CHAR2PIXEL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_Char2Pixel() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void VCHAR2PIXEL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_REPOS_PEN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_REPOS_RANGE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDOTEXT_REPOS_MOVETO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_RANGE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_RANGE_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_USE_RUN_START() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_CLAMP_RUN_END() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_LAST_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_MEASURE_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_NO_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_VIEWRECT_BOTTOM_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_NO_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_5() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_TO_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_SIMPLE_DIV() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_WALK_HEIGHTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_WALK_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_FOUND_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PIXELY_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_7() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_NO_5() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_NODISP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_BODY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_NO_DISPATCH() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_COMMON() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_PORT_STYLED() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_14() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_OK_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_SEL_OK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_DISPATCHED() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PROLOGUE_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDISPATCH() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XTRIMMEASURE_CONT_15() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ORDER_SEL_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECOPY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECOPY_PUTSCRAP_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_PUT_STYL_SCRAP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECOPY_FIRST_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECOPY_CLAMP_ZERO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECOPY_DISPOSE_TEMP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TECUT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDELETE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_SEL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_SEL_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_STYLES() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_ADJ_FIRST_RUN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_REDUCE_COMPACT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_DELETE_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADJ_RUNS_AFTER_DEL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADJ_RUNS_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_ADJ_RUNS_NEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RECALC_LINES() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_RECALC_LINES_REDRAW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEDELETE_CONT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XFINDWORD() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_SKIP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_CONT_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_CONT_3() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_DONE_2() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDWORD_CONT_4() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEWdBreak() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK_STYLED() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK_RUN_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK_USE_END() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_BREAK_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_FIND_LINE_MEASURE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_TEWDBREAK_NEXT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XFINDLINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDLINE_CONT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEFINDLINE_SKIP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void XFINDLINE_NEXT_WORD() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // textedit
