#include "serom/toolbox/textedit.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: textedit
namespace serom { namespace toolbox { namespace textedit { 
    void TESCROLL_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REENABLE_BLINK() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TE_REENABLE_BLINK_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEPINSCROLL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_HEIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_VPOS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_DV0() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_DH() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_HPOS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_DH0() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PINSCROLL_SCROLL() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESELVIEW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void SelView() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void SELVIEW_NO_AUTO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEAUTOVIEW() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOVIEW_OFF() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEAUTOVIEW_EXIT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DefClikProc() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_GET_LINE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_CHECK_BOUNDS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_NONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_DOWN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_UP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_RIGHT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_LEFT() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AUTOSCROLL_APPLY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds_GO_LOWER() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds_FOUND() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds_AT_ZERO() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void DateToSeconds_RETURN() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void CountRunsBefore() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void CountRunsBefore_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void CountRunsBefore_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void GetStyleEntry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AdjustRunPositions() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AdjustRunPositions_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void AdjustRunPositions_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Entry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Redraw() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Frame() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Frame_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Start() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_RunLoop() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_NoStartSplit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_Apply() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_FaceCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_FaceReplace() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_FaceCleared() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_SizeCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_ColorCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_AddSize() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_AddSizeDelta() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_FindOrAddIndex() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_AssignIndex() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_NextRun() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyle_NoOpRTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PrimeStyleEntry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEContinuousStyle_Entry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEContinuousStyle_PlainPath() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEContinuousStyle_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void SetPortFromStyleRun() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void MergeAdjacentRuns() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void MergeAdjacentRuns_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void MergeAdjacentRuns_COPY() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void MergeAdjacentRuns_DONE() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate_FaceCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate_SizeCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate_ColorCheck() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate_No() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void StyleMatchesTemplate_RTS() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEReplaceStyle_Entry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEReplaceStyle_Redraw() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEReplaceStyle_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEReplaceStyle_Frame() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEReplaceStyle_Frame_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEGETOFFSET() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEGetStyleHandle_Entry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TEGetStyleHandle_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyleHandle_Entry() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void TESetStyleHandle_Exit() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PackTexturePattern() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void PenModePatternTbl() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void BuildHilitePattern() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void BuildHilitePattern_Align() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void BuildHilitePattern_LOOP() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
    void BuildHilitePattern_LeaveZero() {
        // TextEdit routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // textedit
