#include "serom/toolbox/font_mgr.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: font_mgr
namespace serom { namespace toolbox { namespace font_mgr { 
    
    void FONTMANAGER() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS_INSTALL_VECTOR() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS_ERR_2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS_CONT_2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS_LIST_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITFONTS_SKIP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void WidthTab_InvalidateAll() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void WidthTab_DerefPrimary() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveFontFamSize() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveFontFamSize_DefSize() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveFontFamSize_GotSize() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveFontFamSize_Pack() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveFontFamSize_Done() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_TRY_FAST() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FAST_CMP1() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FAST_CMP2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FALLBACK() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_RETRY_DEC() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_EXIT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_13() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMLoadFont_Maybe() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMLoadFont_Force() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_16() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_ABORT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_BODY() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_NORMALIZE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_DEVICE_OK() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_RETRY_OK() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_12() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_INVALIDATE_ALL() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_BUILD_FROM_SCRATCH() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_14() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_CACHE_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_CACHE_CMP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_SKIP_5() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_CACHE_PURGED() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NEXT_2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_CACHE_MISS() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_CACHE_LRU() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_16() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_LRU_ENTER() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_LRU_SHIFT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_LRU_DEC() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_REFRESH_PRIMARY() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_18() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_21() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_22() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_OK_9() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_SKIP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_EXTRA_INIT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_OK_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_USE_FOND_WIDTHS() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_STYLE_INIT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_STYLE_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_STYLE_ADD() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_STYLE_NEXT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_SKIP_4() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_BUILD_WIDTH() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_OK_5() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_LOOP_2() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_LOOP_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL16_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_5() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL16_STORE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_TRY_FRACTIONAL() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_LOOP_6() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_6() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_7() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_SCALE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_SCALE_RTS() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL8_LOOP() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_4() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL8_STORE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL_TAIL() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL_DEFAULT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_FILL_DBF() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_PUBLISH_EXTRA() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_NO_PORT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_8() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_AFTER_CONTROL() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_TEAR_DOWN() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSWAPFONT_UNLINK() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMBuildFont() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSEARCH_FOND_LOADED() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void GETFNAME_OK() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_34() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSEARCH_AFTER_TRY() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSEARCH_NORMALIZE_SIZE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_SKIP_6() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_35() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NEXT_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_FOUND_3() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_36() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_37() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_18() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_19() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_OK_12() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSCALE_STORE_NUMER() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSCALE_STORE_VFACT() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void FMSCALE_AFTER_STORE() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_CONT_38() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPFONT_NO_22() {
        // Font Manager routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // font_mgr
