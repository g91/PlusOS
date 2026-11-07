#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_PUTRGN_HPP
#define SEROM_TOOLBOX_QUICKDRAW_PUTRGN_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/PutRgn.s  (6 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace PutRgn { 
    void PUTRGN();
    void PUTRGN_FITS();
    void PUTRGN_NOTRECT();
    void PUTRGN_NXTVERT();
    void NXTHOR();
    void PUTRGN_DONE();
}}}}  // namespace serom::toolbox::quickdraw::PutRgn
#endif  // SEROM_TOOLBOX_QUICKDRAW_PUTRGN_HPP
