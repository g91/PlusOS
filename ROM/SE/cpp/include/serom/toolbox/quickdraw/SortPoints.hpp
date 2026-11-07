#pragma once
#ifndef SEROM_TOOLBOX_QUICKDRAW_SORTPOINTS_HPP
#define SEROM_TOOLBOX_QUICKDRAW_SORTPOINTS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/toolbox/quickdraw/SortPoints.s  (19 entry points)
namespace serom { namespace toolbox { namespace quickdraw { namespace SortPoints { 
    void SORTPOINTS();
    void SORTPOINTS_LOOP();
    void SPLIT();
    void SPLIT_RETRY();
    void SPLIT_NEXT();
    void SPLIT_LOOP();
    void THREE();
    void FOUR();
    void SPLIT_LOOP_2();
    void SPLIT_DONE_2();
    void SORTPOINTS_NOSWAP();
    void SORTPOINTS_RIGHTOK();
    void SORTPOINTS_GOHOME();
    void CULLPOINTS();
    void CULLPOINTS_DELETE();
    void CULLPOINTS_MORE1();
    void CULLPOINTS_WHILE1();
    void CULLPOINTS_DONE();
    void CULLPOINTS_GOHOME();
}}}}  // namespace serom::toolbox::quickdraw::SortPoints
#endif  // SEROM_TOOLBOX_QUICKDRAW_SORTPOINTS_HPP
