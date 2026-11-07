#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_OVALS_HPP
#define SEROM_TOOLBOX_QUICKDRAW_OVALS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/Ovals.s  (12 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace Ovals { 
    void STDOVAL();
    void STDOVAL_NOTPIC();
    void STDOVAL_NOTRGN();
    void NOTFR();
    void STDOVAL_DOIT();
    void FRAMEOVAL();
    void PAINTOVAL();
    void ERASEOVAL();
    void INVERTOVAL();
    void FILLOVAL();
    void CALLOVAL();
    void CALLOVAL_USESTD();
}}}}  // namespace serom::toolbox::quickdraw::Ovals
#endif  // SEROM_TOOLBOX_QUICKDRAW_OVALS_HPP
