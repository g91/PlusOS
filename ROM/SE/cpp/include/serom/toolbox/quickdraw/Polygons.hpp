#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_POLYGONS_HPP
#define SEROM_TOOLBOX_QUICKDRAW_POLYGONS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/Polygons.s  (33 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace Polygons { 
    void STDPOLY();
    void STDPOLY_NOTPIC();
    void STDPOLY_NOTFR();
    void STDPOLY_GOHOME();
    void FRAMEPOLY();
    void PAINTPOLY();
    void ERASEPOLY();
    void INVERTPOLY();
    void FILLPOLY();
    void CALLPOLY();
    void CALLPOLY_USESTD();
    void OPENPOLY();
    void CLOSEPOLY();
    void CLOSEPOLY_NEXTPT();
    void CLOSEPOLY_TOPOK();
    void CLOSEPOLY_BOTOK();
    void CLOSEPOLY_LEFTOK();
    void RIGHTOK();
    void EMPTY();
    void KILLPOLY();
    void OFFSETPOLY();
    void OFFSETPOLY_NEXTPT();
    void MAPPOLY();
    void MAPPOLY_NOTSAME();
    void MAPPOLY_NEXTPT();
    void MAPPOLY_START();
    void MAPPOLY_DONE();
    void FRPOLY();
    void FRPOLY_NEXTPT();
    void FRPOLY_START();
    void FRPOLY_DONE();
    void DRAWPOLY();
    void DRAWPOLY_DONE();
}}}}  // namespace serom::toolbox::quickdraw::Polygons
#endif  // SEROM_TOOLBOX_QUICKDRAW_POLYGONS_HPP
