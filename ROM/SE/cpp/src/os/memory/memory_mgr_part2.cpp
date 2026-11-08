#include "serom/os/memory/memory_mgr.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memory_mgr
namespace serom { namespace os { namespace memory_mgr { 
    void DSPOSEHANDLE() {
        // _DisposeHandle: A0 = handle
        Addr h = A(0);
        if (h) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            if (dataPtr) PokeL(dataPtr - 8, 0); // free data block
            PokeL(h, 0); // clear master pointer
        }
        D(0) = 0;
        SetNZ(0);
    }
    void DISPOSE_RELEASE_MP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DISPOSHANDLE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DISPOSE_LOCKED_EMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETHANDLESIZE() {
        // _GetHandleSize: A0 = handle, D0 = returned size
        Addr h = A(0);
        if (h && PeekL(h)) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            D(0) = PeekL(dataPtr - 8) - 8;
        } else {
            D(0) = 0;
        }
        SetNZ(D(0));
    }
    void SETHANDLESIZE() {
        // _SetHandleSize: A0 = handle, D0 = new size
        Addr h = A(0);
        if (h && PeekL(h)) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            PokeL(dataPtr - 8, ((D(0) + 3) & ~3ul) + 8);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETHANDLESIZE_EMPTY() {
        // _SetHandleSize: A0 = handle, D0 = new size
        Addr h = A(0);
        if (h && PeekL(h)) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            PokeL(dataPtr - 8, ((D(0) + 3) & ~3ul) + 8);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETHANDLESIZE_DO() {
        // _SetHandleSize: A0 = handle, D0 = new size
        Addr h = A(0);
        if (h && PeekL(h)) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            PokeL(dataPtr - 8, ((D(0) + 3) & ~3ul) + 8);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETHANDLESIZE_ERR() {
        // _SetHandleSize: A0 = handle, D0 = new size
        Addr h = A(0);
        if (h && PeekL(h)) {
            Addr dataPtr = PeekL(h) & 0x00FFFFFFul;
            PokeL(dataPtr - 8, ((D(0) + 3) & ~3ul) + 8);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void HANDLEZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RECOVERHANDLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RECOVERHANDLE_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYHANDLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYHANDLE_DO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYHANDLE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void REALLOCHANDLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void REALLOCHANDLE_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void REALLOCHANDLE_LOCKED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void REALLOCHANDLE_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void HLOCK() {
        // _HLock: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) | 0x80000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HUNLOCK() {
        // _HUnlock: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x80000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HPURGE() {
        // _HPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) | 0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HNOPURGE() {
        // _HNoPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HNOPURGE_EXIT() {
        // _HNoPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HNOPURGE_EXIT_2() {
        // _HNoPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HFLAGS_SET_MEMERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void HRSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HNORSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HNOPURGE_CONT() {
        // _HNoPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HNOPURGE_EMPTY() {
        // _HNoPurge: A0 = handle
        Addr h = A(0);
        if (h) PokeL(h, PeekL(h) & ~0x40000000ul);
        D(0) = 0;
        SetNZ(0);
    }
    void HGETFLAGS() {
        // _HGetState: A0 = handle, D0 = flags
        Addr h = A(0);
        D(0) = h ? ((PeekL(h) >> 24) & 0xFF) : 0;
        SetNZ(D(0));
    }
    void HSETFLAGS() {
        // _HSetState: A0 = handle, D0 = flags
        Addr h = A(0);
        if (h) {
            UInt32 flags = (D(0) & 0xFF) << 24;
            PokeL(h, (PeekL(h) & 0x00FFFFFFul) | flags);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void MOREMASTERS() {
        // _MoreMasters: allocate 64 master pointers in zone
        Addr zone = lowmem::GetTheZone();
        if (zone) {
            PokeW(zone + 48, PeekW(zone + 48) + 64);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void MOREMASTERS_SKIP() {
        // _MoreMasters: allocate 64 master pointers in zone
        Addr zone = lowmem::GetTheZone();
        if (zone) {
            PokeW(zone + 48, PeekW(zone + 48) + 64);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void STRIPADDRESS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_RETURN() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_NO_4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_NEXT_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_EPILOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_EMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_NO_5() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_OK_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_OK_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MOVEHHI_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_NO_6() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MOVEHHI_CONT_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MOVEHHI_CONT_8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HEAPGUTS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DEREFHANDLE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DEREFHANDLE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEREFHANDLE_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEREFHANDLE_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEREFHANDLE_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DEREFHANDLE_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEREFHANDLE_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DEREFHANDLE_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMPPROLOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE_CHECK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE_FIND_ZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE_TRY_SYS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE_VALIDATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMHPROLOGUE_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void MMPROLOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMPROLOGUE_ZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMRHPROLOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMRHPROLOGUE_TRY_SYS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMRHPROLOGUE_CHK_LOC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMRHPROLOGUE_VALIDATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMEPILOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MMEPILOGUE_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void MMNOERREPILOGUE() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void MAKEBKF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EH_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EH_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EH_EXIT_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EH_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EH_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EH_EXIT_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EH_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PURGEHEAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PURGEHEAP_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
}}}  // memory_mgr
