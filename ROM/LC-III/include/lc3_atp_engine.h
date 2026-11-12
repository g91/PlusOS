/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Transaction Protocol (ATP Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ATP_ENGINE_H
#define LC3_ATP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define ATP_NO_ERR          0
#define ATP_BAD_REQ_CODE  -3101
#define ATP_TOO_MANY_SKTS -3102
#define ATP_NO_DATA_PKT   -3103
#define ATP_REQ_CANCELLED -3104

typedef struct {
    uint16_t aNet;
    uint8_t aNode;
    uint8_t aSocket;
} ATPAddrBlock;

typedef struct {
    int16_t buffSize;
    void *buffPtr;
    int16_t dataSize;
    uint32_t userBytes;
} BDSElement;

typedef struct {
    uint8_t atpSocket;
    uint8_t atpFlags;
    ATPAddrBlock addrBlock;
    int16_t reqLength;
    void *reqPointer;
    BDSElement *bdsPointer;
    uint8_t numOfBuffs;
    uint8_t timeOutVal;
    uint8_t numOfResends;
    uint8_t bitMap;
    uint16_t transID;
} ATPParamBlock;

/* ATP Core API */
int16_t lc3_atp_open_socket(uint8_t requestedSocket, uint8_t *actualSocket);
int16_t lc3_atp_close_socket(uint8_t atpSocket);
int16_t lc3_atp_send_request(ATPParamBlock *pb, bool isAsync);
int16_t lc3_atp_get_request(ATPParamBlock *pb, bool isAsync);
int16_t lc3_atp_send_response(ATPParamBlock *pb, bool isAsync);
int16_t lc3_atp_add_response(ATPParamBlock *pb, int16_t rspNum);
int16_t lc3_atp_req_cancel(uint16_t transID);
int16_t lc3_atp_kill_send_req(uint8_t atpSocket);

#endif /* LC3_ATP_ENGINE_H */
