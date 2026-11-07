#include "serom/boot/vectors.hpp"
// Scaffold for SE ROM module: se/boot/vectors.s
namespace serom { namespace boot { namespace vectors { 
    void MYFIRSTPROC() { serom::serom_unimplemented("boot::vectors::MYFIRSTPROC"); }
    void MYROM() { serom::serom_unimplemented("boot::vectors::MYROM"); }
    void BASEOFROM() { serom::serom_unimplemented("boot::vectors::BASEOFROM"); }
    void STARTPC() { serom::serom_unimplemented("boot::vectors::STARTPC"); }
    void ROMVERSION() { serom::serom_unimplemented("boot::vectors::ROMVERSION"); }
    void STBOOT() { serom::serom_unimplemented("boot::vectors::STBOOT"); }
    void BADDISK() { serom::serom_unimplemented("boot::vectors::BADDISK"); }
    void ROMRSRC() { serom::serom_unimplemented("boot::vectors::ROMRSRC"); }
    void DISPOFF() { serom::serom_unimplemented("boot::vectors::DISPOFF"); }
    void CRITICAL() { serom::serom_unimplemented("boot::vectors::CRITICAL"); }
    void RESETENTRY() { serom::serom_unimplemented("boot::vectors::RESETENTRY"); }
}}}  // namespace serom::boot::vectors
