/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ASP (AppleTalk Session Protocol) Session Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ASP_SESSION_H
#define LC3_ASP_SESSION_H

#include <stdint.h>
#include <stdbool.h>

#define ASP_NO_ERR              0
#define ASP_BAD_VERSION       -1066
#define ASP_BUFFER_TOO_SMALL  -1067
#define ASP_NO_MORE_SESSIONS  -1068
#define ASP_NO_SERVERS        -1069
#define ASP_PARAM_ERR         -1070
#define ASP_SERVER_BUSY       -1071
#define ASP_SESSION_CLOSED    -1072
#define ASP_SIZE_ERR          -1073
#define ASP_TOO_MANY_CLIENTS  -1074
#define ASP_NO_ACK            -1075

/* ASP Function Codes */
#define ASP_FUNC_CLOSE_SESSION   1
#define ASP_FUNC_COMMAND         2
#define ASP_FUNC_GET_STATUS      3
#define ASP_FUNC_OPEN_SESSION    4
#define ASP_FUNC_TICKLE          5
#define ASP_FUNC_WRITE           6

#define ASP_MAX_SESSIONS         8
#define ASP_MAX_CMD_SIZE         578

typedef struct ASPSession {
    bool active;
    uint8_t sessionID;
    uint16_t remoteNet;
    uint8_t remoteNode;
    uint8_t remoteSocket;
    uint8_t localSocket;
    uint16_t lastSeqNum;
    uint32_t lastTickleTime;
} ASPSession;

/* Public API */
int16_t lc3_asp_open_session(uint16_t net, uint8_t node, uint8_t slsSocket, uint8_t *sessionID);
int16_t lc3_asp_close_session(uint8_t sessionID);
int16_t lc3_asp_command(uint8_t sessionID, const uint8_t *cmdBlock, uint16_t cmdLen, uint8_t *replyBuffer, uint16_t *replyLen);
int16_t lc3_asp_write(uint8_t sessionID, const uint8_t *cmdBlock, uint16_t cmdLen, const uint8_t *writeData, uint16_t writeLen, uint8_t *replyBuffer, uint16_t *replyLen);
int16_t lc3_asp_get_status(uint16_t net, uint8_t node, uint8_t slsSocket, uint8_t *statusBuffer, uint16_t *statusLen);
int16_t lc3_asp_tickle(uint8_t sessionID);

#endif /* LC3_ASP_SESSION_H */
