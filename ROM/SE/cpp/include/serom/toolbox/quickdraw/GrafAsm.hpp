#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_GRAFASM_HPP
#define SEROM_TOOLBOX_QUICKDRAW_GRAFASM_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/GrafAsm.s  (38 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace GrafAsm { 
    void NEWHANDLE();
    void FASTSLABMODE_ERR();
    void SETHSIZE();
    void INITGRAF();
    void CLRLP();
    void CRSRLP();
    void CURDATA();
    void OPENPORT();
    void INITPORT();
    void CLOSEPORT();
    void SETSTDPROCS();
    void LOCALTOGLOBAL();
    void GLOBALTOLOCAL();
    void GLOBALTOLOCAL_CONT();
    void ADDSUB();
    void JUSTADD();
    void ADDPT();
    void SUBPT();
    void SHARE2();
    void SHARE3();
    void SETPORT();
    void GETPORT();
    void GRAFDEVICE();
    void SETPORTBITS();
    void SETPORTBITS_SHARE();
    void BACKPAT();
    void PORTSIZE();
    void MOVEPORTTO();
    void OFSPORT();
    void OFSRECT();
    void SETORIGIN();
    void SETORIGIN_DONE();
    void CLIPRECT();
    void SETCLIP();
    void GETCLIP();
    void GETCLIP_SHARE();
    void SETPT();
    void EQUALPT();
}}}}  // namespace serom::toolbox::quickdraw::GrafAsm
#endif  // SEROM_TOOLBOX_QUICKDRAW_GRAFASM_HPP
