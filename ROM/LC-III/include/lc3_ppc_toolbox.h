/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Program-to-Program Communications (PPC Toolbox)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PPC_TOOLBOX_H
#define LC3_PPC_TOOLBOX_H

#include <stdint.h>
#include <stdbool.h>

#define PPC_NO_ERR            0
#define PPC_NOT_INIT_ERR   -900
#define PPC_PORT_NOT_FOUND -903
#define PPC_SESS_CLOSED    -906
#define PPC_BAD_PORT_NAME  -907

typedef uint32_t PPCSessID;

typedef struct {
    uint8_t nameScript;
    uint8_t name[33];
    uint8_t portKindSelector;
    union {
        uint8_t portTypeStr[33];
        struct {
            uint32_t creator;
            uint32_t type;
        } port;
    } u;
} PPCPortRec;

typedef struct {
    int16_t locationKindSelector;
    union {
        uint8_t nbpType[33];
        struct {
            uint16_t net;
            uint8_t node;
            uint8_t socket;
        } nbp;
    } u;
} LocationNameRec;

typedef struct {
    PPCSessID sessRefNum;
    PPCPortRec *name;
    LocationNameRec *location;
    bool authRequired;
    uint8_t userData[4];
} PPCParamBlock;

/* PPC Toolbox API */
int16_t lc3_ppc_init(void);
int16_t lc3_ppc_open(PPCPortRec *portName, LocationNameRec *locName, bool authRequired, uint32_t *portRefNum);
int16_t lc3_ppc_close(uint32_t portRefNum);
int16_t lc3_ppc_start(uint32_t portRefNum, PPCPortRec *targetPort, LocationNameRec *targetLoc, PPCSessID *newSessID);
int16_t lc3_ppc_inform(uint32_t portRefNum, PPCSessID *incomingSessID);
int16_t lc3_ppc_accept(PPCSessID sessID);
int16_t lc3_ppc_reject(PPCSessID sessID);
int16_t lc3_ppc_write(PPCSessID sessID, const void *buffer, uint32_t reqCount, uint32_t *actCount);
int16_t lc3_ppc_read(PPCSessID sessID, void *buffer, uint32_t reqCount, uint32_t *actCount);
int16_t lc3_ppc_end(PPCSessID sessID);

/* PPC Dispatcher ($A0DD) */
int16_t lc3_ppc_dispatch(int16_t selector, PPCParamBlock *pb);

#endif /* LC3_PPC_TOOLBOX_H */
