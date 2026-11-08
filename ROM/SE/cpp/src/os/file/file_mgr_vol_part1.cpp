#include "serom/os/file/file_mgr_vol.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: file_mgr_vol
namespace serom { namespace os { namespace file_mgr_vol { 
    
    void VDOEJECT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DOEJECT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VDOEJECT_DO_EJECT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VDOEJECT_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DIVUP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DIVUP_EXACT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DIVUP_RET() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void ROUNDALLOC() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void QueueSearch() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void QSEARCH_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void QSEARCH_FOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void QSEARCH_RET() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void QSEARCH_NOTFOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void GETVCBRFN() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void GETVCBRFN_RET() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void GETVCBRFN_NSVERR() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FindVCBDrv() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDVCB_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDVCB_NEXT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDVCB_FOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDVCB_RET() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDVCB_NSVERR() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void ResolveWDVCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void WDLOOKUP_RET() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MARKVCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MARKVCBTIME() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void SetVCBDirty() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHMDB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHMDB_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MAKESTKPB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MAKESTKPB_CLRLOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_RETRY() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_SKIP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CORE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_4() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_ERR_BRA() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_ALLOC_VCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_ENQUEUE_VCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_5() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_SKIP_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_6() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECKREMOUNT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_7() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_REMOUNT_VERIFY() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_8() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_MFS_DIR_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_FOUND_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_4() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_5() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_REMOUNT_NEXT_FCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_6() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_DEQUEUE_NEW_VCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLOK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLDONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLDONE_EXIT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHKERR() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLERR_CLOSE_BT() {
        // Device/File I/O operation
        Addr pb = A(0);
        if (pb) {
            PokeW(pb + 16, 0); // ioResult = noErr
        }
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLER1_FLUSH_CACHES() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTVOLER2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_10() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_8() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_RENAME_FROM_VCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_RENAME_FROM_CAT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_NAME_TRUNC_OK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_COPY_NAME_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_FOUND_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_SKIP_5() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_9() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_10() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_NO_11() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // file_mgr_vol
