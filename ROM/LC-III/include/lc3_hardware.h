/* lc3_hardware.h - Macintosh LC III (Sonora ASIC / 68030) Hardware Map
 *
 * Details the physical and 32-bit logical address map of the Macintosh LC III.
 */
#ifndef LC3_HARDWARE_H
#define LC3_HARDWARE_H

#include <stdint.h>

/* Physical Memory & I/O Bases */
#define LC3_RAM_BASE             0x00000000  /* Up to 36 MB (4 MB onboard + 32 MB 72-pin SIMM) */
#define LC3_ROM_BASE             0x40800000  /* 1 MB ROM space (Mirrored at 0x00800000 in 24-bit mode) */
#define LC3_VRAM_BASE            0x50100000  /* Sonora Framebuffer: 512 KB onboard + 256 KB VRAM SIMM */
#define LC3_SONORA_BASE          0x50F00000  /* Sonora ASIC Control Registers */
#define LC3_SCSI_ESP_BASE        0x50F10000  /* NCR 53C96 Enhanced SCSI Processor */
#define LC3_ESP_BASE             LC3_SCSI_ESP_BASE
#define LC3_SCC_RD_BASE          0x50F04000  /* Serial Communications Controller Read */
#define LC3_SCC_WR_BASE          0x50F04002  /* Serial Communications Controller Write */
#define LC3_SCC_BASE             LC3_SCC_RD_BASE
#define LC3_VIA1_BASE            0x50F00000  /* Versatile Interface Adapter (VIA1) */
#define LC3_VIA2_BASE            0x50F02000  /* VIA2 / Sonora Pseudo-VIA2 Registers */
#define LC3_EGRET_BASE           0x50F02100  /* Egret Microcontroller Communication Interface */
#define LC3_SWIM_BASE            0x50F16000  /* Super Woz Integrated Machine (Floppy) */
#define LC3_PDS_SLOT_BASE        0xFC000000  /* LC III Processor Direct Slot */

/* Sonora Video Display Modes (640x480 standard) */
#define LC3_SCREEN_WIDTH         640
#define LC3_SCREEN_HEIGHT        480
#define LC3_ROWBYTES_1BPP        80          /* 80 bytes * 8 = 640 pixels */
#define LC3_ROWBYTES_4BPP        320         /* 320 bytes * 2 = 640 pixels */
#define LC3_ROWBYTES_8BPP        640         /* 640 bytes * 1 = 640 pixels */
#define LC3_ROWBYTES_16BPP       1280        /* 640 pixels * 2 bytes/pixel (requires 768K VRAM) */

/* Sonora ASIC Registers */
#define SONORA_VBL_CTL           (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x0012))
#define SONORA_INTERRUPT_MASK    (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x0018))
#define SONORA_INTERRUPT_STATUS  (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x001A))
#define SONORA_SCSI_DACK         (*(volatile uint32_t *)(LC3_SONORA_BASE + 0x0020))
#define SONORA_VIDEO_MODE        (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x0030))
#define SONORA_CLUT_ADDR         (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x0034))
#define SONORA_CLUT_DATA         (*(volatile uint8_t *)(LC3_SONORA_BASE + 0x0036))

/* NCR 53C96 ESP (Enhanced SCSI Processor) Registers */
struct NCR53C96 {
    volatile uint8_t xfer_count_lsb;     /* +0x00 Read/Write Transfer Counter (Low) */
    uint8_t pad0[15];
    volatile uint8_t xfer_count_msb;     /* +0x10 Read/Write Transfer Counter (High) */
    uint8_t pad1[15];
    volatile uint8_t fifo_data;          /* +0x20 Read/Write 16-Byte FIFO Data */
    uint8_t pad2[15];
    volatile uint8_t command;            /* +0x30 Read/Write Command Register */
    uint8_t pad3[15];
    volatile uint8_t status;             /* +0x40 Read Status Register */
    uint8_t pad4[15];
    volatile uint8_t interrupt;          /* +0x50 Read Interrupt Register (Clears INT) */
    uint8_t pad5[15];
    volatile uint8_t seq_step;           /* +0x60 Read Sequence Step Register */
    uint8_t pad6[15];
    volatile uint8_t fifo_flags;         /* +0x70 Read FIFO Flags */
    uint8_t pad7[15];
    volatile uint8_t config1;            /* +0x80 Read/Write Configuration Register 1 */
    uint8_t pad8[15];
    volatile uint8_t clock_factor;       /* +0x90 Clock Conversion Factor */
    uint8_t pad9[15];
    volatile uint8_t test;               /* +0xA0 Test Mode Register */
    uint8_t pad10[15];
    volatile uint8_t config2;            /* +0xB0 Read/Write Configuration Register 2 */
    uint8_t pad11[15];
    volatile uint8_t config3;            /* +0xC0 Read/Write Configuration Register 3 */
    uint8_t pad12[15];
};

#define LC3_ESP ((volatile struct NCR53C96 *)LC3_SCSI_ESP_BASE)

/* 53C96 Command Register Constants */
#define ESP_CMD_DMA              0x80
#define ESP_CMD_NOP              0x00
#define ESP_CMD_FLUSH_FIFO       0x01
#define ESP_CMD_RESET_CHIP       0x02
#define ESP_CMD_RESET_BUS        0x03
#define ESP_CMD_XFER_INFO        0x10
#define ESP_CMD_INIT_CMD_COMP    0x11
#define ESP_CMD_MSG_ACCEPTED     0x12
#define ESP_CMD_TRANSFER_PAD     0x18
#define ESP_CMD_SET_ATN          0x1A
#define ESP_CMD_SELECT_NO_ATN    0x41
#define ESP_CMD_SELECT_ATN       0x42
#define ESP_CMD_SELECT_ATN_STOP  0x43

#endif /* LC3_HARDWARE_H */
