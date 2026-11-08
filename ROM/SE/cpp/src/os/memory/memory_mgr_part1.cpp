#include "serom/os/memory/memory_mgr.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: memory_mgr
namespace serom { namespace os { namespace memory_mgr { 
    
    void HEAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPLBASE() {
        // SetApplBase: sets application memory base
        Addr base = A(0) ? A(0) : 0x2800;
        lowmem::SetApplZone(base);
        lowmem::SetTheZone(base);
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPBASE_SKIP() {
        // SetApplBase: sets application memory base
        Addr base = A(0) ? A(0) : 0x2800;
        lowmem::SetApplZone(base);
        lowmem::SetTheZone(base);
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPBASE_CONT() {
        // SetApplBase: sets application memory base
        Addr base = A(0) ? A(0) : 0x2800;
        lowmem::SetApplZone(base);
        lowmem::SetTheZone(base);
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPBASE_OK() {
        // SetApplBase: sets application memory base
        Addr base = A(0) ? A(0) : 0x2800;
        lowmem::SetApplZone(base);
        lowmem::SetTheZone(base);
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE_SKIP() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE_LOOP() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE_LOOP_2() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE_SKIP_2() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITAPPLZONE_DONE() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITZONE() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITZONE_LOOP() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void INITZONE_ERR() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void MEMINIT_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void INITZONE_EXIT() {
        // _InitZone / _InitApplZone
        Addr start = A(0) ? A(0) : 0x20000;
        Addr limit = 0x60000;
        lowmem::SetApplZone(start);
        lowmem::SetTheZone(start);
        lowmem::SetHeapEnd(limit);
        PokeL(start + 0, limit);       // bkLim
        PokeL(start + 8, start + 52);  // hFstFree
        PokeL(start + 12, limit - start - 52); // zcbFree
        D(0) = 0;
        SetNZ(0);
    }
    void GETZONE() {
        // _GetZone: A0 = current zone
        A(0) = lowmem::GetTheZone();
        D(0) = 0;
        SetNZ(0);
    }
    void SETZONE() {
        // _SetZone: A0 = new zone
        lowmem::SetTheZone(A(0));
        D(0) = 0;
        SetNZ(0);
    }
    void MAXBLOCK() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void COMPACTMEM() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void MAXBLOCK_OR_COMPACT() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void COMPACTMEM_NO() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void MAXBLOCK_DO_COMPACT() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void PURGEMEM() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void PURGEMEM_CONT() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void PURGEMEM_EXIT() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void PURGEMEM_EXIT_2() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void PURGEMEM_ERR() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void PURGESPACE() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void FREEMEM() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void RESRVMEM() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void RESRVMEM_MAKESPC() {
        // PurgeMem / PurgeSpace / ResrvMem
        D(0) = 0;
        A(0) = 0x40000;
        SetNZ(0);
    }
    void MAXMEM() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void MAXMEM_CONT() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void MAXMEM_DONE() {
        // FreeMem / MaxMem / CompactMem
        Addr zone = lowmem::GetTheZone();
        UInt32 freeBytes = zone ? PeekL(zone + 12) : 0x40000;
        if (freeBytes == 0) freeBytes = 0x40000;
        D(0) = freeBytes;
        A(0) = freeBytes;
        SetNZ(D(0));
    }
    void SETGROWZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPLLIMIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETAPPLLIMIT_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETAPPLLIMIT_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STACKSPACE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MAXAPPLZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MAXAPPLZONE_NO_ROOM() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void NEWPTR() {
        // _NewPtr: D0 = size, A0 = returned pointer
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr ptr = heapEnd - byteCount - 16;
        lowmem::SetHeapEnd(ptr);
        PokeL(ptr - 8, byteCount + 8); // block size
        PokeL(ptr - 4, zone);          // owner zone
        A(0) = ptr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void NEWPTR_EXIT() {
        // _NewPtr: D0 = size, A0 = returned pointer
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr ptr = heapEnd - byteCount - 16;
        lowmem::SetHeapEnd(ptr);
        PokeL(ptr - 8, byteCount + 8); // block size
        PokeL(ptr - 4, zone);          // owner zone
        A(0) = ptr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void MM_PTR_EPILOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEWPTR_ERR_2() {
        // _NewPtr: D0 = size, A0 = returned pointer
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr ptr = heapEnd - byteCount - 16;
        lowmem::SetHeapEnd(ptr);
        PokeL(ptr - 8, byteCount + 8); // block size
        PokeL(ptr - 4, zone);          // owner zone
        A(0) = ptr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void DISPOSEPTR() {
        // _DisposePtr: A0 = pointer
        Addr ptr = A(0);
        if (ptr) {
            PokeL(ptr - 8, 0); // mark free
        }
        D(0) = 0;
        SetNZ(0);
    }
    void DISPOSPTR_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DISPOSPTR_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETPTRSIZE() {
        // _GetPtrSize: A0 = pointer, D0 = returned size
        Addr ptr = A(0);
        if (ptr) {
            D(0) = PeekL(ptr - 8) - 8;
        } else {
            D(0) = 0;
        }
        SetNZ(D(0));
    }
    void SETPTRSIZE() {
        // _SetPtrSize: A0 = pointer, D0 = new size
        Addr ptr = A(0);
        if (ptr) {
            PokeL(ptr - 8, ((D(0) + 3) & ~3ul) + 8);
        }
        D(0) = 0;
        SetNZ(0);
    }
    void PTRZONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PTRZONE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PTRZONE_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MM_CLEAR_BLOCK_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NEWEMPTYHANDLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NWHANDLE() {
        // _NewHandle: D0 = size, A0 = returned handle
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr dataPtr = heapEnd - byteCount - 16;
        Addr masterPtr = dataPtr - 8;
        lowmem::SetHeapEnd(masterPtr);
        PokeL(masterPtr, dataPtr & 0x00FFFFFFul); // master pointer -> data
        PokeL(dataPtr - 8, byteCount + 8);        // block header size
        PokeL(dataPtr - 4, masterPtr);            // master pointer backref
        A(0) = masterPtr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void NEWHANDLE_EMPTY() {
        // _NewHandle: D0 = size, A0 = returned handle
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr dataPtr = heapEnd - byteCount - 16;
        Addr masterPtr = dataPtr - 8;
        lowmem::SetHeapEnd(masterPtr);
        PokeL(masterPtr, dataPtr & 0x00FFFFFFul); // master pointer -> data
        PokeL(dataPtr - 8, byteCount + 8);        // block header size
        PokeL(dataPtr - 4, masterPtr);            // master pointer backref
        A(0) = masterPtr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void NEWHANDLE_ALLOC() {
        // _NewHandle: D0 = size, A0 = returned handle
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr dataPtr = heapEnd - byteCount - 16;
        Addr masterPtr = dataPtr - 8;
        lowmem::SetHeapEnd(masterPtr);
        PokeL(masterPtr, dataPtr & 0x00FFFFFFul); // master pointer -> data
        PokeL(dataPtr - 8, byteCount + 8);        // block header size
        PokeL(dataPtr - 4, masterPtr);            // master pointer backref
        A(0) = masterPtr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void NEWHANDLE_CONT() {
        // _NewHandle: D0 = size, A0 = returned handle
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr dataPtr = heapEnd - byteCount - 16;
        Addr masterPtr = dataPtr - 8;
        lowmem::SetHeapEnd(masterPtr);
        PokeL(masterPtr, dataPtr & 0x00FFFFFFul); // master pointer -> data
        PokeL(dataPtr - 8, byteCount + 8);        // block header size
        PokeL(dataPtr - 4, masterPtr);            // master pointer backref
        A(0) = masterPtr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
    void MM_HANDLE_NOERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void MM_HANDLE_EPILOGUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEWHANDLE_2() {
        // _NewHandle: D0 = size, A0 = returned handle
        UInt32 byteCount = (D(0) + 3) & ~3ul;
        Addr zone = lowmem::GetTheZone();
        if (!zone) zone = lowmem::GetSysZone();
        Addr heapEnd = lowmem::GetHeapEnd();
        if (!heapEnd) heapEnd = 0x80000;
        Addr dataPtr = heapEnd - byteCount - 16;
        Addr masterPtr = dataPtr - 8;
        lowmem::SetHeapEnd(masterPtr);
        PokeL(masterPtr, dataPtr & 0x00FFFFFFul); // master pointer -> data
        PokeL(dataPtr - 8, byteCount + 8);        // block header size
        PokeL(dataPtr - 4, masterPtr);            // master pointer backref
        A(0) = masterPtr;
        D(0) = 0; // noErr
        SetNZ(0);
    }
}}}  // memory_mgr
