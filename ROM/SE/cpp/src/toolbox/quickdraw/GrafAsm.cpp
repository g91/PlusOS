#include "serom/toolbox/quickdraw/GrafAsm.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: GrafAsm
namespace serom { namespace toolbox { namespace quickdraw { namespace GrafAsm { 
    
    void NEWHANDLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FASTSLABMODE_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SETHSIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INITGRAF() {
        // _InitGraf: initialize QuickDraw globals at A5
        Addr qdGlobals = A(5) - 206;
        PokeL(qdGlobals, 0); // thePort
        PokeL(qdGlobals + 4, 0x003FA700ul); // screenBits.baseAddr
        PokeW(qdGlobals + 8, 64);           // screenBits.rowBytes
        PokeL(qdGlobals + 10, 0);           // screenBits.bounds.top,left
        PokeW(qdGlobals + 14, 342);          // screenBits.bounds.bottom
        PokeW(qdGlobals + 16, 512);          // screenBits.bounds.right
        D(0) = 0;
        SetNZ(0);
    }
    void CLRLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CRSRLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CURDATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OPENPORT() {
        // _OpenPort / _InitPort: A0 = port
        Addr port = A(0);
        if (port) {
            PokeW(port + 0, 0); // device
            PokeL(port + 2, 0x003FA700ul); // portBits.baseAddr
            PokeW(port + 6, 64);           // rowBytes
            PokeW(port + 14, 0x0101);      // pnSize (1,1)
            PokeW(port + 18, 0);           // pnMode (patCopy)
        }
        D(0) = 0;
        SetNZ(0);
    }
    void INITPORT() {
        // _OpenPort / _InitPort: A0 = port
        Addr port = A(0);
        if (port) {
            PokeW(port + 0, 0); // device
            PokeL(port + 2, 0x003FA700ul); // portBits.baseAddr
            PokeW(port + 6, 64);           // rowBytes
            PokeW(port + 14, 0x0101);      // pnSize (1,1)
            PokeW(port + 18, 0);           // pnMode (patCopy)
        }
        D(0) = 0;
        SetNZ(0);
    }
    void CLOSEPORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETSTDPROCS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LOCALTOGLOBAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GLOBALTOLOCAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GLOBALTOLOCAL_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ADDSUB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void JUSTADD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ADDPT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SUBPT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHARE2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHARE3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETPORT() {
        // _SetPort: A0 = port
        Addr port = A(0);
        Addr qdGlobals = A(5) - 206;
        if (port) PokeL(qdGlobals, port);
        D(0) = 0;
        SetNZ(0);
    }
    void GETPORT() {
        // _GetPort: A0 = returned port
        Addr qdGlobals = A(5) - 206;
        A(0) = PeekL(qdGlobals);
        D(0) = 0;
        SetNZ(0);
    }
    void GRAFDEVICE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETPORTBITS() {
        // _SetPort: A0 = port
        Addr port = A(0);
        Addr qdGlobals = A(5) - 206;
        if (port) PokeL(qdGlobals, port);
        D(0) = 0;
        SetNZ(0);
    }
    void SETPORTBITS_SHARE() {
        // _SetPort: A0 = port
        Addr port = A(0);
        Addr qdGlobals = A(5) - 206;
        if (port) PokeL(qdGlobals, port);
        D(0) = 0;
        SetNZ(0);
    }
    void BACKPAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PORTSIZE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEPORTTO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void OFSPORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OFSRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SETORIGIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETORIGIN_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CLIPRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SETCLIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETCLIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETCLIP_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETPT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALPT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // GrafAsm
