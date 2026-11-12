/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk DDP (Datagram Delivery) & NBP (Name Binding Protocol)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLETALK_NBP_H
#define LC3_APPLETALK_NBP_H

#include <stdint.h>
#include <stdbool.h>

#define ATALK_NO_ERR           0
#define ATALK_SOCKET_ERR     -91
#define ATALK_COLLISION_ERR -1026
#define ATALK_PARAM_ERR       -50

typedef struct {
    uint16_t net;
    uint8_t  node;
    uint8_t  socket;
} AddrBlock;

typedef struct {
    AddrBlock addr;
    uint8_t   enumerator;
    char      entityName[100]; // Object:Type@Zone
} NBPTuple;

typedef struct {
    uint16_t ddpLength;
    uint16_t ddpChecksum;
    uint16_t dstNet;
    uint16_t srcNet;
    uint8_t  dstNode;
    uint8_t  srcNode;
    uint8_t  dstSocket;
    uint8_t  srcSocket;
    uint8_t  ddpType;
} DDPHeader;

/* DDP API */
int16_t lc3_ddp_open_socket(uint8_t *socketNum, void *listenerProc);
int16_t lc3_ddp_close_socket(uint8_t socketNum);
int16_t lc3_ddp_write(uint8_t srcSocket, const AddrBlock *dstAddr, uint8_t ddpType, const void *data, uint16_t len);

/* NBP API */
int16_t lc3_nbp_register(NBPTuple *tuple, uint8_t socketNum);
int16_t lc3_nbp_remove(const NBPTuple *tuple);
int16_t lc3_nbp_lookup(const char *entityPattern, NBPTuple *resultList, int16_t maxTuples, int16_t *tuplesFound);
int16_t lc3_nbp_confirm(const NBPTuple *tuple, const AddrBlock *addr);

#endif /* LC3_APPLETALK_NBP_H */
