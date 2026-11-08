#pragma once
#ifndef SEROM_OS_SCSI_BOOT_HPP
#define SEROM_OS_SCSI_BOOT_HPP
#include "serom/support.hpp"
// Scaffold for SE ROM module: se/os/scsi_boot.s  (25 entry points)
namespace serom { namespace os { namespace scsi_boot { 
    void SCSIBOOTS();
    void SCSILOAD();
    void BOOT_SCSI_TRY_DRIVE();
    void BOOT_SCSI_DRIVE_OK();
    void BOOT_SCSI_NO_DRIVE();
    void BOOT_SCSI_SCAN_PMAP();
    void SCSI_FIND_DRVR();
    void SCSI_LOAD_DRVR();
    void BOOT_SCSI_NO_PMAP();
    void BOOT_SCSI_RUN_DRIVER();
    void BOOT_SCSI_DISPOSE_BUF();
    void BOOT_SCSI_RTS_ERR();
    void BOOT_SCSI_RTS();
    void BOOT_SCSI_PMAP_LOOP();
    void BOOT_SCSI_NEXT_PM_ENTRY();
    void BOOT_SCSI_RESCAN();
    void BOOT_SCSI_FIND_HFS();
    void SCSIREADBLOCKS();
    void SCSIREADBLOCKS_RETRY();
    void BOOT_SCSI_HFS_FOUND();
    void BOOT_SCSI_RDBLOCKS();
    void BOOT_SCSI_DRVR_REFNUM();
    void BOOT_SCSI_DRVR_INSTALL();
    void BOOT_SCSI_DRVR_OK();
    void BOOT_SCSI_DRVR_RTS();
}}}  // namespace serom::os::scsi_boot
#endif  // SEROM_OS_SCSI_BOOT_HPP
