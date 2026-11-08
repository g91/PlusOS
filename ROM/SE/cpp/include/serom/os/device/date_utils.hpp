#pragma once
#ifndef SEROM_OS_DATE_UTILS_HPP
#define SEROM_OS_DATE_UTILS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/date_utils.s  (8 entry points)
namespace serom { namespace os { namespace date_utils { 
    void PACKMGREND();
    void SEXYDATE();
    void SECS2DATE();
    void S2D_NOT_LEAP();
    void S2D_PAST_FEB();
    void DATE2SECS();
    void D2S_NOT_LEAP();
    void D2S_IN_JAN_FEB();
}}}  // namespace serom::os::date_utils
#endif  // SEROM_OS_DATE_UTILS_HPP
