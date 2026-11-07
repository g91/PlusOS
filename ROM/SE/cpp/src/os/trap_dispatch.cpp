#include "serom/os/trap_dispatch.hpp"
// Scaffold for SE ROM module: se/os/trap_dispatch.s
namespace serom { namespace os { namespace trap_dispatch { 
    void DSPATCH() { serom::serom_unimplemented("os::trap_dispatch::DSPATCH"); }
    void ATRAP68020() { serom::serom_unimplemented("os::trap_dispatch::ATRAP68020"); }
    void ATRAP68010() { serom::serom_unimplemented("os::trap_dispatch::ATRAP68010"); }
    void EMT1010() { serom::serom_unimplemented("os::trap_dispatch::EMT1010"); }
    void TRAP_COMMON() { serom::serom_unimplemented("os::trap_dispatch::TRAP_COMMON"); }
    void TRAP_DECODE() { serom::serom_unimplemented("os::trap_dispatch::TRAP_DECODE"); }
    void TRAP_TBOX_DISPATCH() { serom::serom_unimplemented("os::trap_dispatch::TRAP_TBOX_DISPATCH"); }
    void TRAP_OS_PATH() { serom::serom_unimplemented("os::trap_dispatch::TRAP_OS_PATH"); }
    void TRAP_OS_RETURN() { serom::serom_unimplemented("os::trap_dispatch::TRAP_OS_RETURN"); }
    void TRAP_OS_NOSAVEREG() { serom::serom_unimplemented("os::trap_dispatch::TRAP_OS_NOSAVEREG"); }
    void SETTRAPADDRESS() { serom::serom_unimplemented("os::trap_dispatch::SETTRAPADDRESS"); }
    void SET_TRAP_STORE() { serom::serom_unimplemented("os::trap_dispatch::SET_TRAP_STORE"); }
    void SET_TRAP_OLD() { serom::serom_unimplemented("os::trap_dispatch::SET_TRAP_OLD"); }
    void DETECT_TRAP_TABLE() { serom::serom_unimplemented("os::trap_dispatch::DETECT_TRAP_TABLE"); }
    void DETECT_OS_TABLE() { serom::serom_unimplemented("os::trap_dispatch::DETECT_OS_TABLE"); }
    void DETECT_DONE() { serom::serom_unimplemented("os::trap_dispatch::DETECT_DONE"); }
    void GET_TRAP_OLD() { serom::serom_unimplemented("os::trap_dispatch::GET_TRAP_OLD"); }
    void GETTRAPADDRESS() { serom::serom_unimplemented("os::trap_dispatch::GETTRAPADDRESS"); }
    void GET_TRAP_READ() { serom::serom_unimplemented("os::trap_dispatch::GET_TRAP_READ"); }
}}}  // namespace serom::os::trap_dispatch
