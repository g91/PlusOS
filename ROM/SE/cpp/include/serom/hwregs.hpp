#pragma once
#ifndef SEROM_HWREGS_HPP
#define SEROM_HWREGS_HPP
//
// serom/hwregs.hpp -- Hardware registers for Macintosh SE
// Transcribed from F:\mac_rom\include\hwregs.inc
//
#include "serom/machine.hpp"

namespace serom {
namespace hwregs {

// === VIA 1 (6522 Versatile Interface Adapter) ===
enum : Addr {
    VBase       = 0xEFE1FE, // VIA base address
    VIA_PortB   = 0xEFE1FE, // Port B data (RTC, mouse button, sound)
    VIA_PortA   = 0xEFFFFE, // Port A data (sound vol, overlay, disk sel, screen)
    VIA_DDRB    = 0xEFE5FE, // Port B data direction
    VIA_DDRA    = 0xEFE7FE, // Port A data direction
    VIA_T1CL    = 0xEFE9FE, // Timer 1 counter low
    VIA_T1CH    = 0xEFEBFE, // Timer 1 counter high
    VIA_T1LL    = 0xEFEDFE, // Timer 1 latch low
    VIA_T1LH    = 0xEFEFFE, // Timer 1 latch high
    VIA_T2CL    = 0xEFF1FE, // Timer 2 counter low
    VIA_T2CH    = 0xEFF3FE, // Timer 2 counter high
    VIA_SR      = 0xEFF5FE, // Shift register (keyboard / ADB)
    VIA_ACR     = 0xEFF7FE, // Auxiliary control register
    VIA_PCR     = 0xEFF9FE, // Peripheral control register
    VIA_IFR     = 0xEFFBFE, // Interrupt flag register
    VIA_IER     = 0xEFFDFE, // Interrupt enable register
};

// === SCSI (NCR 5380) ===
enum : Addr {
    SCSI_Base   = 0x580000,
    SCSI_Data   = 0x580000, // Current SCSI data / Output data
    SCSI_ICR    = 0x580002, // Initiator Command Register
    SCSI_MR     = 0x580004, // Mode Register
    SCSI_TCR    = 0x580006, // Target Command Register
    SCSI_CSR    = 0x580008, // Current SCSI Bus Status
    SCSI_BSR    = 0x58000A, // Bus and Status Register
    SCSI_IDR    = 0x58000C, // Input Data Register
    SCSI_RST    = 0x58000E, // Reset Parity / Interrupts
};

// === SCC (Zilog 8530 Serial Communications Controller) ===
enum : Addr {
    SCC_Read    = 0x9FFFF8,
    SCC_Write   = 0xBF0004,
    SCC_B_Cmd   = 0x9FFFF8,
    SCC_A_Cmd   = 0x9FFFFC,
    SCC_B_Data  = 0x9FFFFA,
    SCC_A_Data  = 0x9FFFFE,
};

// === Video Framebuffer ===
enum : Addr {
    ScreenBase  = 0x003FA700, // SE 512x342 1-bpp main screen buffer
    ScreenAlt   = 0x003F2700, // SE alternate screen buffer
    ScreenWidth = 512,
    ScreenHeight= 342,
    ScreenRowBytes = 64,      // 512 / 8
};

} // namespace hwregs
} // namespace serom

#endif // SEROM_HWREGS_HPP
