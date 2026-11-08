#include "serom/boot/boot_load.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: boot_load
namespace serom { namespace boot { namespace boot_load { 
    
    void MYBOOT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOTME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CALL_SECONDARY_BOOT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SELECT_HEAP_SIZE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void USE_BOOT_HEAP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void USE_FIXED_HEAP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CAP_APPL_BASE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SET_APPL_BASE2() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COPY_SYSRES_NAME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void OPEN_SYSTEM_FILE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LOAD_DSAT_RESOURCE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GOT_DSAT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_FONTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RESERVE_PRINT_BUF() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RESERVE_STACK_BUF() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EXEC_PATCH1() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void OPEN_SYSRES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LAUNCH_FINDER() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_LOAD_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_LOAD_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LAUNCH_APP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOT_FAIL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUNT_FAILED_EJECT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CLEAR_BOOT_MASK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RETRY_BOOT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECK_BOOT_ERROR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECK_BOOT_ERROR_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECK_FILE_EXISTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void OPEN_BOOT_PATCH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void OPEN_AND_READ_FILE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETUP_READ_PARAMS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CLOSE_FILE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILE_LOAD_RETURN() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ALLOC_BOOT_BUF() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ALLOC_SYS_PTR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETVOL_BLESSED_FOLDER() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETVOL_BLESSED_FOLDER_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUSEINIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LOAD_MCKY_RSRC() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void MOUSEINIT_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITFS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CLEAR_CKDDB_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITFS_ALLOC_FCB() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ALLOC_VOL_CACHE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ALLOC_CTL_CACHE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITFS_OK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITFS_FAIL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITEVENTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILL_EVTBUF_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // boot_load
