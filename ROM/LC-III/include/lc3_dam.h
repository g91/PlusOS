/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Data Access Manager (DAM / Pack 13)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DAM_H
#define LC3_DAM_H

#include <stdint.h>
#include <stdbool.h>

#define DAM_NO_ERR          0
#define DAM_NOT_INIT       -1
#define DAM_BAD_SESS_NUM   -2

typedef int32_t DBSessNum;

typedef struct {
    int16_t status;
    int32_t result;
} DBAsyncStatus;

/* Data Access Manager API */
int16_t lc3_init_db_pack(void);
int16_t lc3_db_init(DBSessNum *sessNum, const char *dName, const char *uName, const char *password, const char *connStr, void *asyncStatus);
int16_t lc3_db_end(DBSessNum sessNum, void *asyncStatus);
int16_t lc3_db_get_conn_info(DBSessNum sessNum, int16_t sessNumIndex, int16_t *returnedID, char *dName, char *uName, void *asyncStatus);
int16_t lc3_db_get_session_num(DBSessNum *sessNum, void *asyncStatus);
int16_t lc3_db_send(DBSessNum sessNum, const char *text, int16_t len, void *asyncStatus);
int16_t lc3_db_send_item(DBSessNum sessNum, int16_t dataType, int16_t len, int16_t decPlaces, int16_t flags, const void *buffer, void *asyncStatus);
int16_t lc3_db_exec(DBSessNum sessNum, void *asyncStatus);
int16_t lc3_db_state(DBSessNum sessNum, void *asyncStatus);
int16_t lc3_db_get_err(DBSessNum sessNum, int32_t *err1, int32_t *err2, char *item1, char *item2, char *errorMsg, void *asyncStatus);
int16_t lc3_db_break(DBSessNum sessNum, bool abort, void *asyncStatus);
int16_t lc3_db_get_item(DBSessNum sessNum, int16_t *dataType, int16_t *len, int16_t *decPlaces, int16_t *flags, void *buffer, void *asyncStatus);
int16_t lc3_db_unget_item(DBSessNum sessNum, void *asyncStatus);

/* Package 13 Dispatcher ($A82B) */
int16_t lc3_pack13_dispatch(int16_t selector, void *params);

#endif /* LC3_DAM_H */
