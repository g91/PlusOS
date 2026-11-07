#pragma once
#ifndef SEROM_OS_TIME_MGR_HPP
#define SEROM_OS_TIME_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/time_mgr.s  (21 entry points)
namespace serom { namespace os { namespace time_mgr { 
    void TIMEMGR();
    void INSTIME();
    void RMVTIME();
    void RMVTIME_DONE();
    void PRIMETIME();
    void PRIME_NO_ACTIVE();
    void PRIME_PEEL();
    void PRIME_RESTORE_SR();
    void SCAN_TIMER_QUEUE();
    void SCAN_NEXT_TASK();
    void SCAN_SKIP_TASK();
    void SCAN_DONE();
    void PROGRAM_T2();
    void T2_DELAY_OK();
    void T2_NO_TASK();
    void P_VIAint5_Timer2();
    void T2ISR_NEXT_TASK();
    void T2ISR_SKIP_TASK();
    void T2ISR_DISPATCH();
    void T2ISR_RESTORE();
    void T2ISR_DONE();
}}}  // namespace serom::os::time_mgr
#endif  // SEROM_OS_TIME_MGR_HPP
