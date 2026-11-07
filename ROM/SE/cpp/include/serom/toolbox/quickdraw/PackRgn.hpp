#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_PACKRGN_HPP
#define SEROM_TOOLBOX_QUICKDRAW_PACKRGN_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/PackRgn.s  (10 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace PackRgn { 
    void PACKRGN();
    void PACKRGN_NO();
    void PACKRGN_LOOP();
    void PACKRGN_NO_2();
    void ENDSCAN();
    void NEXTPT();
    void PACKRGN_START();
    void VSAME();
    void PACKRGN_DONE();
    void PACKRGN_SIZEOK();
}}}}  // namespace serom::toolbox::quickdraw::PackRgn
#endif  // SEROM_TOOLBOX_QUICKDRAW_PACKRGN_HPP
