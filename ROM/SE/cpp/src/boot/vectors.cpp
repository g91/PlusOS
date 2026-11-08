#include "serom/boot/vectors.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: vectors
namespace serom { namespace boot { namespace vectors { 
    
    void MYFIRSTPROC() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void MYROM() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void BASEOFROM() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void STARTPC() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void ROMVERSION() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void STBOOT() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void BADDISK() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void ROMRSRC() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void DISPOFF() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void CRITICAL() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
    void RESETENTRY() {
        // Exception vector table setup
        for (Addr a = 0; a < 0x400; a += 4) {
            PokeL(a, 0x00400000ul); // default vector handler
        }
        PokeL(0x0004, 0x00400000ul); // Reset PC
        D(0) = 0;
        SetNZ(0);
    }
}}}  // vectors
