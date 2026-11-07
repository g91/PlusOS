#pragma once
#ifndef SEROM_HW_INTERRUPTS_HPP
#define SEROM_HW_INTERRUPTS_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/hw/interrupts.s  (20 entry points)
namespace serom { namespace hw { namespace interrupts { 
    void INTHND();
    void LVL1INT();
    void VIA_SCAN_NEXT();
    void LVL2INT();
    void SCC_DISPATCH_JMP();
    void SPURIOUS();
    void EXTBINT();
    void EXTAINT();
    void EXTSTATUS_DISPATCH();
    void EXTSTATUS_RESET();
    void ONESECINT();
    void LVL1RTS();
    void LVL2RTS();
    void VBLINT();
    void VBL_CHECK_HEAP();
    void VBL_CURSOR_TASK();
    void VBL_TASK_QUEUE();
    void VBL_TASK_LOOP();
    void VBL_TASK_NEXT();
    void VBL_TASKS_DONE();
}}}  // namespace serom::hw::interrupts
#endif  // SEROM_HW_INTERRUPTS_HPP
