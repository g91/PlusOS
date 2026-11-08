#pragma once
#ifndef SEROM_OS_TRAP_DISPATCH_HPP
#define SEROM_OS_TRAP_DISPATCH_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/trap_dispatch.s  (19 entry points)
namespace serom { namespace os { namespace trap_dispatch { 
    void DSPATCH();
    void ATRAP68020();
    void ATRAP68010();
    void EMT1010();
    void TRAP_COMMON();
    void TRAP_DECODE();
    void TRAP_TBOX_DISPATCH();
    void TRAP_OS_PATH();
    void TRAP_OS_RETURN();
    void TRAP_OS_NOSAVEREG();
    void SETTRAPADDRESS();
    void SET_TRAP_STORE();
    void SET_TRAP_OLD();
    void DETECT_TRAP_TABLE();
    void DETECT_OS_TABLE();
    void DETECT_DONE();
    void GET_TRAP_OLD();
    void GETTRAPADDRESS();
    void GET_TRAP_READ();
}}}  // namespace serom::os::trap_dispatch
#endif  // SEROM_OS_TRAP_DISPATCH_HPP
