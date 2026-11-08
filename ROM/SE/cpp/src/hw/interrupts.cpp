#include "serom/hw/interrupts.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: interrupts
namespace serom { namespace hw { namespace interrupts { 
    
    void INTHND() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void LVL1INT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VIA_SCAN_NEXT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void LVL2INT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void SCC_DISPATCH_JMP() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void SPURIOUS() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void EXTBINT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void EXTAINT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void EXTSTATUS_DISPATCH() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void EXTSTATUS_RESET() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void ONESECINT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void LVL1RTS() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void LVL2RTS() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBLINT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_CHECK_HEAP() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_CURSOR_TASK() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_TASK_QUEUE() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_TASK_LOOP() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_TASK_NEXT() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
    void VBL_TASKS_DONE() {
        // Interrupt acknowledge & tick update
        UInt32 t = lowmem::Get<UInt32>(lowmem::Ticks);
        lowmem::Set<UInt32>(lowmem::Ticks, t + 1);
        D(0) = 0;
        g_cpu.ccr.z = true;
    }
}}}  // interrupts
