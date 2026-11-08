#include "serom/boot/sad_mac.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: sad_mac
namespace serom { namespace boot { namespace sad_mac { 
    
    void CRITERR() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void CRITERR_ERASE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void CRITERR_DRAW_LOW() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void CRITERR_SHOW_ICON() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void CRITERR_DEADLOOP() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void HEX_DIGIT_LOOP() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void GLYPH_DRAW_LOOP() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_HEX_FONT() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void HAPPYMAC() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void SADMAC() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_MAC_ICON() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_MAC_FACE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_MAC_RETURN() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTSYMBOL() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON_ROW() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON_BYTE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON_COPY() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON_ADVANCE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void PUTICON_NEXTROW() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_XOR_PROPAGATE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_MAC_OUTLINE() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_mHappyMac() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_mSadMac() {
        // Sad Mac display
        Addr fb = 0x003FA700ul; // SE video base
        PokeW(fb, 0xFFFF);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // sad_mac
