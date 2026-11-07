#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_SEEKRGN_HPP
#define SEROM_TOOLBOX_QUICKDRAW_SEEKRGN_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/SeekRgn.s  (16 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace SeekRgn { 
    void INITRGN();
    void INITRGN_CLRLOOP();
    void SEEKRGN();
    void UP();
    void SEEKRGN_CLRLP();
    void DOWN();
    void SEEKRGN_DOWN1();
    void SEEKRGN_NEXTHOR();
    void LOK();
    void ROK();
    void NOTIN1();
    void INVLONG();
    void INVLONG_CONT();
    void ENDWORD();
    void DONE1();
    void SEEKRGN_DONE();
}}}}  // namespace serom::toolbox::quickdraw::SeekRgn
#endif  // SEROM_TOOLBOX_QUICKDRAW_SEEKRGN_HPP
