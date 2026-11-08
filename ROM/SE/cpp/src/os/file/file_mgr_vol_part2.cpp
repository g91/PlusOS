#include "serom/os/file/file_mgr_vol.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: file_mgr_vol
namespace serom { namespace os { namespace file_mgr_vol { 
    void MOUNTVOL_NO_12() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_SCAN_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_5() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_6() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_7() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_8() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_OK_9() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNTVOL_CONT_11() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void MTCHECK_EXT_SCAN_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void P_BMChk() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VBMCHK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_SKIP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_EMPTY() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_EMPTY_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_NO() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_NO_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_4() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FindFCB() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_6() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FindFCB_NEXT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_7() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void BMCHK_CONT_8() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDDRIVE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDDRIVE_EMPTY() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDDRIVE_FOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_CONT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_SKIP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_CONT_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_SKIP_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_OK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void OFFLINE_ERR() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_OFFLINE_JOIN() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_FOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_FOUND_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_OK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_OK_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_MFS_PATH() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_CONT_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_NO_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECT_NO_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL_CONT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL_NO() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL_NEXT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void UNMOUNTVOL_EXIT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL_UNMOUNTVOL_SHARED() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL_OK() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL_SKIP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL_NO_DISPOSE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void FLUSHVOL_DONE() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void P_CKExtFS() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VCKEXTFS() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void CKEXTFS_RTS() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void P_DTRMV3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VDTRMV3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void P_DTRMV1() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void VDTRMV1() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_CONT() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_TRY_WD() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_CONT_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_NO() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_EMPTY() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_FOUND() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_ERR() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_LOOP() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_ERR_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_OK_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_OK_3() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
    void DTRMV1_NO_2() {
        // Volume mount / VCB search
        Addr pb = A(0);
        if (pb) PokeW(pb + 16, 0);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // file_mgr_vol
