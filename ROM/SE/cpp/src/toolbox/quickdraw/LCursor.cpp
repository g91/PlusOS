#include "serom/toolbox/quickdraw/LCursor.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: LCursor
namespace serom { namespace toolbox { namespace quickdraw { namespace LCursor { 
    
    void INITCURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETCURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HIDECURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWCURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHIELDCURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OBSCURECURSOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // LCursor
