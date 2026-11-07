#pragma once
#ifndef SEROM_BOOT_SAD_MAC_HPP
#define SEROM_BOOT_SAD_MAC_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/boot/sad_mac.s  (24 entry points)
namespace serom { namespace boot { namespace sad_mac { 
    void CRITERR();
    void CRITERR_ERASE();
    void CRITERR_DRAW_LOW();
    void CRITERR_SHOW_ICON();
    void CRITERR_DEADLOOP();
    void HEX_DIGIT_LOOP();
    void GLYPH_DRAW_LOOP();
    void DT_HEX_FONT();
    void HAPPYMAC();
    void SADMAC();
    void DRAW_MAC_ICON();
    void DRAW_MAC_FACE();
    void DRAW_MAC_RETURN();
    void PUTSYMBOL();
    void PUTICON();
    void PUTICON_ROW();
    void PUTICON_BYTE();
    void PUTICON_COPY();
    void PUTICON_ADVANCE();
    void PUTICON_NEXTROW();
    void ICON_XOR_PROPAGATE();
    void ICON_MAC_OUTLINE();
    void DT_mHappyMac();
    void DT_mSadMac();
}}}  // namespace serom::boot::sad_mac
#endif  // SEROM_BOOT_SAD_MAC_HPP
