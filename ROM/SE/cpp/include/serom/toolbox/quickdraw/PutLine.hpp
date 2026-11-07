#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_PUTLINE_HPP
#define SEROM_TOOLBOX_QUICKDRAW_PUTLINE_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/PutLine.s  (13 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace PutLine { 
    void PUTLINE();
    void DHPOS();
    void DVPOS();
    void DVPOS_OK();
    void DVPOS_NO();
    void PUTLINE_SLANTED();
    void NOSWAP();
    void NOSWAP_OK();
    void NOSWAP_NO();
    void LOOP();
    void NOCHNG();
    void NOCHNG_EXIT();
    void PUTLINE_GOHOME();
}}}}  // namespace serom::toolbox::quickdraw::PutLine
#endif  // SEROM_TOOLBOX_QUICKDRAW_PUTLINE_HPP
