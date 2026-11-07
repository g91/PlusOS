#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_PUTOVAL_HPP
#define SEROM_TOOLBOX_QUICKDRAW_PUTOVAL_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/PutOval.s  (8 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace PutOval { 
    void PUTOVAL();
    void NXTVERT();
    void PUTOVAL_YESBUMP();
    void PUTOVAL_YESBUMP_OK();
    void PUTOVAL_YESBUMP_SKIP();
    void PUTPT();
    void PUTOVAL_SIZEOK();
    void APPEND();
}}}}  // namespace serom::toolbox::quickdraw::PutOval
#endif  // SEROM_TOOLBOX_QUICKDRAW_PUTOVAL_HPP
