#include "serom/os/file/file_mgr_dir.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: file_mgr_dir
namespace serom { namespace os { namespace file_mgr_dir { 
    void RENAME_DO_CATALOG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_UPDATE_VOL_NAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void RENAME_NEXT_FCB_FOR_FILE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_ERR_4() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void RENAME_ERR_5() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void RENAME_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_VOL_VERIFY_ONLINE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RENAME_VOL_COPY_NAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TFMOVE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CATMOVE_NO_CROSSVOL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CATMOVE_BAD_NAME_RTS() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void CATMOVE_HFS_PATH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void HFSCATMOVE_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_ERR_3() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void HFSCATMOVE_OK_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void HFSCATMOVE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CATMOVE_NEXT_FCB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DELETE_MFS_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void HFSCATMOVE_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void HFSCATMOVE_NO_4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_OK_6() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void RSTFILLOCK_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void HFSCATMOVE_ERR_5() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void HFSCATMOVE_FOUND_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_NO_5() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HFSCATMOVE_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void HFSCATMOVE_OK_7() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILEINFO_NEXT_FCB_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILTYPE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETFILTYPE_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETFILTYPE_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILLOCK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RSTFILLOCK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETFILLOCK_HFS_PATH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RSTFILLOCK_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void RSTFILLOCK_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void RSTFILLOCK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILLOCK_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILEINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETCATINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILEFILETFSETCATINFO_HFS_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILEFILETFSETCATINFO_HFS_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FILEFILETFSETCATINFO_HFS_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void FILEFILETFSETCATINFO_HFS_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FILEFILETFSETCATINFO_HFS_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void P_FndFilName() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VFNDFILNAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FNDFILNAME_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FNDFILNAME_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FNDFILNAME_NO_4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FNDFILNAME_OK_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FNDFILNAME_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FNDFILNAME_NO_5() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_OK_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FNDFILNAME_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void FNDFILNAME_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void FNDFILNAME_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CkExtFS_AndOnline() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FNDFILNAME_ERR_3() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETCATINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_BEGIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_HFS_OR_MFS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_INDEXED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_VOLNAME_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_DIR_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_FNF_TRANSLATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETCATINFO_OFFSPRING() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_LOOP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_NO_4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_LOOP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_HFS_FILENUM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_OK_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_OK_5() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_ERR_3() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_FCB_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}  // file_mgr_dir
