#pragma once
#ifndef SEROM_OS_EVENT_MGR_HPP
#define SEROM_OS_EVENT_MGR_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/event_mgr.s  (20 entry points)
namespace serom { namespace os { namespace event_mgr { 
    void EVENTS();
    void POSTEVENT();
    void POSTEVENT_ENABLED();
    void POSTEVENT_SCAN_LOOP();
    void POSTEVENT_FILL();
    void FILL_EVENT_FIELDS();
    void OSEVENTAVAIL();
    void OSEVTAVAIL_WALK();
    void OSEVTAVAIL_CHK_AUTOKEY();
    void OSEVTAVAIL_FOUND();
    void OSEVTAVAIL_COPY_LOOP();
    void OSEVTAVAIL_NO_EVENT();
    void GETOSEVENT();
    void ADB_DEQUEUE_RTS();
    void ADB_DEQUEUE_INVALIDATE();
    void FLUSHEVENTS();
    void FLUSH_WALK_LOOP();
    void FLUSH_NEXT_ENTRY();
    void FLUSH_QUEUE_EMPTY();
    void FLUSH_EXIT();
}}}  // namespace serom::os::event_mgr
#endif  // SEROM_OS_EVENT_MGR_HPP
