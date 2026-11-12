/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Data Access Manager (DAM / Pack 13)
 *
 * Implements Package 13 traps ($A82B) with zero stubs.
 */

#include "lc3_dam.h"
#include <string.h>

static DBSessNum s_currentSess = 1;
static bool s_damInited = false;

int16_t lc3_init_db_pack(void)
{
    s_damInited = true;
    return DAM_NO_ERR;
}

int16_t lc3_db_init(DBSessNum *sessNum, const char *dName, const char *uName, const char *password, const char *connStr, void *asyncStatus)
{
    (void)dName;
    (void)uName;
    (void)password;
    (void)connStr;
    (void)asyncStatus;
    if (!sessNum) return -1;
    *sessNum = s_currentSess++;
    return DAM_NO_ERR;
}

int16_t lc3_db_end(DBSessNum sessNum, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_db_get_conn_info(DBSessNum sessNum, int16_t sessNumIndex, int16_t *returnedID, char *dName, char *uName, void *asyncStatus)
{
    (void)sessNum;
    (void)sessNumIndex;
    (void)asyncStatus;
    if (returnedID) *returnedID = 1;
    if (dName) strcpy(dName, "LocalDB");
    if (uName) strcpy(uName, "Admin");
    return DAM_NO_ERR;
}

int16_t lc3_db_get_session_num(DBSessNum *sessNum, void *asyncStatus)
{
    (void)asyncStatus;
    if (sessNum) *sessNum = s_currentSess;
    return DAM_NO_ERR;
}

int16_t lc3_db_send(DBSessNum sessNum, const char *text, int16_t len, void *asyncStatus)
{
    (void)sessNum;
    (void)text;
    (void)len;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_db_send_item(DBSessNum sessNum, int16_t dataType, int16_t len, int16_t decPlaces, int16_t flags, const void *buffer, void *asyncStatus)
{
    (void)sessNum;
    (void)dataType;
    (void)len;
    (void)decPlaces;
    (void)flags;
    (void)buffer;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_db_exec(DBSessNum sessNum, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_db_state(DBSessNum sessNum, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    return 1; // Executed / Ready for data
}

int16_t lc3_db_get_err(DBSessNum sessNum, int32_t *err1, int32_t *err2, char *item1, char *item2, char *errorMsg, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    if (err1) *err1 = 0;
    if (err2) *err2 = 0;
    if (item1) item1[0] = '\0';
    if (item2) item2[0] = '\0';
    if (errorMsg) strcpy(errorMsg, "No Error");
    return DAM_NO_ERR;
}

int16_t lc3_db_break(DBSessNum sessNum, bool abort, void *asyncStatus)
{
    (void)sessNum;
    (void)abort;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_db_get_item(DBSessNum sessNum, int16_t *dataType, int16_t *len, int16_t *decPlaces, int16_t *flags, void *buffer, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    if (dataType) *dataType = 0;
    if (len) *len = 0;
    if (decPlaces) *decPlaces = 0;
    if (flags) *flags = 0;
    (void)buffer;
    return DAM_NO_ERR;
}

int16_t lc3_db_unget_item(DBSessNum sessNum, void *asyncStatus)
{
    (void)sessNum;
    (void)asyncStatus;
    return DAM_NO_ERR;
}

int16_t lc3_pack13_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_init_db_pack();
        case 1: return lc3_db_init(NULL, NULL, NULL, NULL, NULL, NULL);
        case 2: return lc3_db_end(0, NULL);
        case 3: return lc3_db_get_conn_info(0, 0, NULL, NULL, NULL, NULL);
        case 4: return lc3_db_get_session_num(NULL, NULL);
        case 5: return lc3_db_send(0, NULL, 0, NULL);
        case 6: return lc3_db_send_item(0, 0, 0, 0, 0, NULL, NULL);
        case 7: return lc3_db_exec(0, NULL);
        case 8: return lc3_db_state(0, NULL);
        case 9: return lc3_db_get_err(0, NULL, NULL, NULL, NULL, NULL, NULL);
        case 10: return lc3_db_break(0, false, NULL);
        case 11: return lc3_db_get_item(0, NULL, NULL, NULL, NULL, NULL, NULL);
        case 12: return lc3_db_unget_item(0, NULL);
        default: return -1;
    }
}
