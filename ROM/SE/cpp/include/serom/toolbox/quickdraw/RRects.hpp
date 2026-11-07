#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_RRECTS_HPP
#define SEROM_TOOLBOX_QUICKDRAW_RRECTS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/RRects.s  (12 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace RRects { 
    void STDRRECT();
    void OVALOK();
    void STDRRECT_NOTPIC();
    void STDRRECT_NOTRGN();
    void STDRRECT_DOIT();
    void FRAMEROUNDRECT();
    void PAINTROUNDRECT();
    void ERASEROUNDRECT();
    void INVERTROUNDRECT();
    void FILLROUNDRECT();
    void CALLRRECT();
    void CALLRRECT_USESTD();
}}}}  // namespace serom::toolbox::quickdraw::RRects
#endif  // SEROM_TOOLBOX_QUICKDRAW_RRECTS_HPP
