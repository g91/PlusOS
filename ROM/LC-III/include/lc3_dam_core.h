/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Database Access Manager (DAM / Pack7) Core Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DAM_CORE_H
#define LC3_DAM_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define DAM_NO_ERR         0
#define DAM_CONN_ERR     -8000
#define DAM_QUERY_ERR    -8001
#define DAM_PARAM_ERR      -50

typedef struct DBConnInfo {
    int32_t connID;
    char serverName[64];
    bool isConnected;
    bool queryInProgress;
    int32_t lastError;
} DBConnInfo;

typedef struct DBColumnDef {
    char colName[32];
    int16_t colType;      /* 1 = int, 2 = float, 3 = string */
    uint16_t maxLen;
} DBColumnDef;

/* DAM API */
int16_t lc3_dam_init(DBConnInfo *conn, const char *serverName);
int16_t lc3_dam_start_query(DBConnInfo *conn, const char *sqlQuery);
int16_t lc3_dam_exec_query(DBConnInfo *conn);
int16_t lc3_dam_get_results(DBConnInfo *conn, DBColumnDef *cols, uint16_t maxCols, uint16_t *actualCols, void *rowBuffer, uint32_t *rowBytes);
int16_t lc3_dam_get_conn_info(const DBConnInfo *conn, char *infoOut, uint16_t maxLen);
int16_t lc3_dam_dispatch(int16_t selector, void *params);

#endif /* LC3_DAM_CORE_H */
