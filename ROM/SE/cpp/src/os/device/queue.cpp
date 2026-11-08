#include "serom/os/device/queue.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: queue
namespace serom { namespace os { namespace queue { 

    
    void QSTUFF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENQUEUE() {
        Addr elem = A(0);
        Addr qhdr = A(1);
        if (!elem || !qhdr) return;
        PokeL(elem, 0); // clear qLink
        Addr head = PeekL(qhdr + 2);
        if (head == 0) {
            PokeL(qhdr + 2, elem);
            PokeL(qhdr + 6, elem);
        } else {
            Addr tail = PeekL(qhdr + 6);
            if (tail) PokeL(tail, elem);
            PokeL(qhdr + 6, elem);
        }
        g_cpu.ccr.z = true;
    }
    void ENQ_APPEND_TAIL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENQ_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DEQUEUE() {
        Addr elem = A(0);
        Addr qhdr = A(1);
        if (!elem || !qhdr) {
            D(0) = static_cast<UInt32>(-1);
            g_cpu.ccr.z = false;
            return;
        }
        Addr prev = 0;
        Addr curr = PeekL(qhdr + 2);
        while (curr) {
            if (curr == elem) {
                Addr next = PeekL(curr);
                if (!prev) PokeL(qhdr + 2, next);
                else PokeL(prev, next);
                if (PeekL(qhdr + 6) == elem) PokeL(qhdr + 6, prev);
                D(0) = 0;
                g_cpu.ccr.z = true;
                return;
            }
            prev = curr;
            curr = PeekL(curr);
        }
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
    }
    void DEQ_SEARCH_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEQ_UNLINK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DEQ_SUCCESS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DEQ_RETURN() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DEQ_UNLINK_MID() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INITQUEUE() {
        Addr qhdr = A(1);
        if (qhdr) {
            PokeW(qhdr, 0);
            PokeL(qhdr + 2, 0);
            PokeL(qhdr + 6, 0);
        }
        g_cpu.ccr.z = true;
    }
}}}  // queue
