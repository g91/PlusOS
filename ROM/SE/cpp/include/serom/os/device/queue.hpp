#pragma once
#ifndef SEROM_OS_QUEUE_HPP
#define SEROM_OS_QUEUE_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/queue.s  (11 entry points)
namespace serom { namespace os { namespace queue { 
    void QSTUFF();
    void ENQUEUE();
    void ENQ_APPEND_TAIL();
    void ENQ_DONE();
    void DEQUEUE();
    void DEQ_SEARCH_LOOP();
    void DEQ_UNLINK();
    void DEQ_SUCCESS();
    void DEQ_RETURN();
    void DEQ_UNLINK_MID();
    void INITQUEUE();
}}}  // namespace serom::os::queue
#endif  // SEROM_OS_QUEUE_HPP
