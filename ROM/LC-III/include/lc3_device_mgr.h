/* lc3_device_mgr.h - Macintosh LC III Device Manager & DCE / Unit Table Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00B5C2 - 0x00BCC6
 */
#ifndef LC3_DEVICE_MGR_H
#define LC3_DEVICE_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define UNIT_TABLE_MAX_ENTRIES 64

/* Device Control Entry (DCE) Flags */
#define D_NEED_GOODBYE       0x0200
#define D_NEED_TIME          0x0400
#define D_NEED_LOCK          0x4000
#define D_RAM_BASED          0x0040
#define D_OPENED             0x0020
#define D_READ_ENABLE        0x0001
#define D_WRITE_ENABLE       0x0002
#define D_CONTROL_ENABLE     0x0004
#define D_STATUS_ENABLE      0x0008

/* Device Control Entry (DCE) structure - 32-bit clean */
typedef struct LC3AuxDCE {
    void    *dCtlDriver;        /* Driver code handle or pointer */
    int16_t  dCtlFlags;         /* DCE flags */
    void    *dCtlQueue;         /* I/O request queue head */
    uint32_t dCtlQHead;
    uint32_t dCtlQTail;
    uint32_t dCtlPosition;      /* Current byte offset */
    void   **dCtlStorage;       /* Driver private storage handle */
    int16_t  dCtlRefNum;        /* Driver refNum (-1 - unitNum) */
    int16_t  dCtlCurTicks;      /* Periodic action tick counter */
    int32_t  dCtlWindow;        /* Desk accessory window pointer */
    int16_t  dCtlDelay;         /* Periodic action frequency */
    int16_t  dCtlEMask;         /* Desk accessory event mask */
    int16_t  dCtlMenu;          /* Menu ID associated with driver */
    int8_t   dCtlSlot;          /* Slot number if NuBus/PDS card */
    int8_t   dCtlSlotId;
    int32_t  dCtlDevBase;       /* Base hardware address */
    int32_t  reserved;
} LC3AuxDCE;

/* I/O Parameter Block */
typedef struct LC3IOParam {
    struct LC3IOParam *qLink;
    uint16_t           qType;
    int16_t            ioTrap;
    void              *ioCmdAddr;
    void             (*ioCompletion)(struct LC3IOParam *pb);
    int16_t            ioResult;
    char              *ioNamePtr;
    int16_t            ioVRefNum;
    int16_t            ioRefNum;
    int8_t             ioVersNum;
    int8_t             ioPermssn;
    void              *ioBuffer;
    uint32_t           ioReqCount;
    uint32_t           ioActCount;
    int16_t            ioPosMode;
    int32_t            ioPosOffset;
} LC3IOParam;

/* Device Manager Low-Memory Globals */
#define LM_UTABLEBASE        (*(volatile LC3AuxDCE ***)0x011C)
#define LM_UNITTABLESZ       (*(volatile int16_t   *)0x01D2)

/* Device Manager Interface */
void        lc3_devmgr_init(void);
LC3AuxDCE **lc3_devmgr_get_dce(int16_t refNum);
int16_t     lc3_devmgr_lookup_unit(const char *name);
int16_t     lc3_devmgr_open(LC3IOParam *pb);
int16_t     lc3_devmgr_close(LC3IOParam *pb);
int16_t     lc3_devmgr_read(LC3IOParam *pb);
int16_t     lc3_devmgr_write(LC3IOParam *pb);
int16_t     lc3_devmgr_control(LC3IOParam *pb);
int16_t     lc3_devmgr_status(LC3IOParam *pb);
int16_t     lc3_devmgr_kill_io(LC3IOParam *pb);
int16_t     lc3_devmgr_install(int16_t refNum, LC3AuxDCE **dceHandle);
int16_t     lc3_devmgr_remove(int16_t refNum);
void        lc3_devmgr_enqueue(LC3AuxDCE *dce, LC3IOParam *pb);
LC3IOParam *lc3_devmgr_dequeue(LC3AuxDCE *dce);
void        lc3_devmgr_dispatch_next(LC3AuxDCE *dce);

#endif /* LC3_DEVICE_MGR_H */
