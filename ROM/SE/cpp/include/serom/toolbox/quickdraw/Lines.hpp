#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_LINES_HPP
#define SEROM_TOOLBOX_QUICKDRAW_LINES_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/Lines.s  (30 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace Lines { 
    void STDLINE();
    void STDLINE_NO();
    void PUTOP();
    void STDLINE_STARTOK();
    void DHDV();
    void UPDATE();
    void STDLINE_NOTPIC();
    void LINETO();
    void LINETO_USESTD();
    void LINE();
    void MOVETO();
    void MOVE();
    void MOOV();
    void DOLINE();
    void DOLINE_NO();
    void SIZEOK();
    void NOTPOLY();
    void DOLINE_NOTRGN();
    void HIDEPEN();
    void SHOWPEN();
    void SHOWPEN_CONT();
    void GETPENSTATE();
    void SETPENSTATE();
    void SETPENSTATE_CONT();
    void NOTSET();
    void GETPEN();
    void PENSIZE();
    void PENMODE();
    void PENPAT();
    void PENNORMAL();
}}}}  // namespace serom::toolbox::quickdraw::Lines
#endif  // SEROM_TOOLBOX_QUICKDRAW_LINES_HPP
