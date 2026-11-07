#include "serom/hw/interrupts.hpp"
// Scaffold for SE ROM module: se/hw/interrupts.s
namespace serom { namespace hw { namespace interrupts { 
    void INTHND() { serom::serom_unimplemented("hw::interrupts::INTHND"); }
    void LVL1INT() { serom::serom_unimplemented("hw::interrupts::LVL1INT"); }
    void VIA_SCAN_NEXT() { serom::serom_unimplemented("hw::interrupts::VIA_SCAN_NEXT"); }
    void LVL2INT() { serom::serom_unimplemented("hw::interrupts::LVL2INT"); }
    void SCC_DISPATCH_JMP() { serom::serom_unimplemented("hw::interrupts::SCC_DISPATCH_JMP"); }
    void SPURIOUS() { serom::serom_unimplemented("hw::interrupts::SPURIOUS"); }
    void EXTBINT() { serom::serom_unimplemented("hw::interrupts::EXTBINT"); }
    void EXTAINT() { serom::serom_unimplemented("hw::interrupts::EXTAINT"); }
    void EXTSTATUS_DISPATCH() { serom::serom_unimplemented("hw::interrupts::EXTSTATUS_DISPATCH"); }
    void EXTSTATUS_RESET() { serom::serom_unimplemented("hw::interrupts::EXTSTATUS_RESET"); }
    void ONESECINT() { serom::serom_unimplemented("hw::interrupts::ONESECINT"); }
    void LVL1RTS() { serom::serom_unimplemented("hw::interrupts::LVL1RTS"); }
    void LVL2RTS() { serom::serom_unimplemented("hw::interrupts::LVL2RTS"); }
    void VBLINT() { serom::serom_unimplemented("hw::interrupts::VBLINT"); }
    void VBL_CHECK_HEAP() { serom::serom_unimplemented("hw::interrupts::VBL_CHECK_HEAP"); }
    void VBL_CURSOR_TASK() { serom::serom_unimplemented("hw::interrupts::VBL_CURSOR_TASK"); }
    void VBL_TASK_QUEUE() { serom::serom_unimplemented("hw::interrupts::VBL_TASK_QUEUE"); }
    void VBL_TASK_LOOP() { serom::serom_unimplemented("hw::interrupts::VBL_TASK_LOOP"); }
    void VBL_TASK_NEXT() { serom::serom_unimplemented("hw::interrupts::VBL_TASK_NEXT"); }
    void VBL_TASKS_DONE() { serom::serom_unimplemented("hw::interrupts::VBL_TASKS_DONE"); }
}}}  // namespace serom::hw::interrupts
