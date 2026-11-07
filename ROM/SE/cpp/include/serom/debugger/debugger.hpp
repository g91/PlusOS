#pragma once
#ifndef SEROM_DEBUGGER_DEBUGGER_HPP
#define SEROM_DEBUGGER_DEBUGGER_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/debugger/debugger.s  (27 entry points)
namespace serom { namespace debugger { namespace debugger { 
    void SYSERRINIT();
    void VECLOOP_UNIQUE();
    void VECLOOP_UNASSIGNED();
    void VECLOOP_IRQ();
    void VECLOOP_TRAP();
    void CLEAR_SEREGS();
    void DEBUGGER();
    void DBG_NO_DEBUGGER();
    void DBG_CLEANUP();
    void DEBUGPROLOG();
    void TODEEPSHIT();
    void EXCPT_REGS_SAVED();
    void EXCPT_STD_FRAME();
    void EXCPT_DISCARD_FRAME();
    void EXCPT_TO_SYSERR();
    void GENEXCPS();
    void EXCPT_ADDR_ERR();
    void EXCPT_ILLEGAL();
    void EXCPT_ZERO_DIV();
    void EXCPT_CHK();
    void EXCPT_TRAPV();
    void EXCPT_PRIVILEGE();
    void EXCPT_TRACE();
    void EXCPT_LINE_1010();
    void EXCPT_LINE_1111();
    void EXCPT_UNASSIGNED();
    void IRQEXCEPTION();
}}}  // namespace serom::debugger::debugger
#endif  // SEROM_DEBUGGER_DEBUGGER_HPP
