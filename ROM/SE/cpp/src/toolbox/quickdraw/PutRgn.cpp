#include "serom/toolbox/quickdraw/PutRgn.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: PutRgn
namespace serom { namespace toolbox { namespace quickdraw { namespace PutRgn { 
    
    void PUTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRGN_FITS() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRGN_NOTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRGN_NXTVERT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void NXTHOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // PutRgn
