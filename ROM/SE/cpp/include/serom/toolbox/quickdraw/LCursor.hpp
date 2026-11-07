#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_LCURSOR_HPP
#define SEROM_TOOLBOX_QUICKDRAW_LCURSOR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/LCursor.s  (6 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace LCursor { 
    void INITCURSOR();
    void SETCURSOR();
    void HIDECURSOR();
    void SHOWCURSOR();
    void SHIELDCURSOR();
    void OBSCURECURSOR();
}}}}  // namespace serom::toolbox::quickdraw::LCursor
#endif  // SEROM_TOOLBOX_QUICKDRAW_LCURSOR_HPP
