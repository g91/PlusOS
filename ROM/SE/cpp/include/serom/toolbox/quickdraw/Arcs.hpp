#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_ARCS_HPP
#define SEROM_TOOLBOX_QUICKDRAW_ARCS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/Arcs.s  (10 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace Arcs { 
    void STDARC();
    void NOTPIC();
    void DOIT();
    void FRAMEARC();
    void PAINTARC();
    void ERASEARC();
    void INVERTARC();
    void FILLARC();
    void CALLARC();
    void USESTD();
}}}}  // namespace serom::toolbox::quickdraw::Arcs
#endif  // SEROM_TOOLBOX_QUICKDRAW_ARCS_HPP
