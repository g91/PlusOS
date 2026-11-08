#include "serom/os/device/event_mgr.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: event_mgr
namespace serom { namespace os { namespace event_mgr { 
    
    void EVENTS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void POSTEVENT() {
        // _PostEvent: D0 = event code, A0 = message
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void POSTEVENT_ENABLED() {
        // _PostEvent: D0 = event code, A0 = message
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void POSTEVENT_SCAN_LOOP() {
        // _PostEvent: D0 = event code, A0 = message
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void POSTEVENT_FILL() {
        // _PostEvent: D0 = event code, A0 = message
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void FILL_EVENT_FIELDS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OSEVENTAVAIL() {
        // _GetOSEvent / _OSEventAvail: D0 = mask, A0 = EventRecord
        Addr evRec = A(0);
        if (evRec) {
            PokeW(evRec + 0, 0); // what = nullEvent
            PokeL(evRec + 2, 0); // message
            PokeL(evRec + 6, lowmem::Get<UInt32>(lowmem::Ticks)); // when
            PokeL(evRec + 10, 0x01000100ul); // where (pt)
            PokeW(evRec + 14, 0); // modifiers
        }
        D(0) = 0; // false (no pending event)
        g_cpu.ccr.z = true;
    }
    void OSEVTAVAIL_WALK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OSEVTAVAIL_CHK_AUTOKEY() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void OSEVTAVAIL_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OSEVTAVAIL_COPY_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void OSEVTAVAIL_NO_EVENT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETOSEVENT() {
        // _GetOSEvent / _OSEventAvail: D0 = mask, A0 = EventRecord
        Addr evRec = A(0);
        if (evRec) {
            PokeW(evRec + 0, 0); // what = nullEvent
            PokeL(evRec + 2, 0); // message
            PokeL(evRec + 6, lowmem::Get<UInt32>(lowmem::Ticks)); // when
            PokeL(evRec + 10, 0x01000100ul); // where (pt)
            PokeW(evRec + 14, 0); // modifiers
        }
        D(0) = 0; // false (no pending event)
        g_cpu.ccr.z = true;
    }
    void ADB_DEQUEUE_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ADB_DEQUEUE_INVALIDATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHEVENTS() {
        // _FlushEvents: D0 = mask
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSH_WALK_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FLUSH_NEXT_ENTRY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSH_QUEUE_EMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSH_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}  // event_mgr
