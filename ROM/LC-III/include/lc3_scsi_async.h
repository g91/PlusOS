/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 186: SCSI Manager 4.3 Async I/O Completion & Queue Management (GD)
 */

#ifndef LC3_SCSI_ASYNC_H
#define LC3_SCSI_ASYNC_H

#include <stdint.h>
#include <stdbool.h>

#define LC3_SCSI_MAX_PENDING     16
#define LC3_SCSI_MAX_DEVICES     8

/* SCSI Status codes */
#define LC3_SCSI_STAT_PENDING    0
#define LC3_SCSI_STAT_SUCCESS    1
#define LC3_SCSI_STAT_TIMEOUT    2
#define LC3_SCSI_STAT_ABORTED    3
#define LC3_SCSI_STAT_BUS_ERROR  4

typedef struct lc3_scsi_req {
    uint8_t  target_id;
    uint8_t  lun;
    uint8_t  cdb[16];
    uint8_t  cdb_len;
    void    *buf;
    uint32_t buf_len;
    bool     is_write;
    uint32_t timeout_ticks;
    uint32_t start_tick;
    uint8_t  status;
    void   (*completion)(struct lc3_scsi_req *req);
    struct lc3_scsi_req *next;
} lc3_scsi_req_t;

typedef struct {
    lc3_scsi_req_t *head;
    lc3_scsi_req_t *tail;
    uint16_t count;
    bool bus_busy[LC3_SCSI_MAX_DEVICES];
} lc3_scsi_async_engine_t;

void lc3_scsi_async_init(lc3_scsi_async_engine_t *engine);
int16_t lc3_scsi_async_queue_request(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req);
void lc3_scsi_async_complete_request(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req, uint8_t status);
int16_t lc3_scsi_async_dequeue(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req);
void lc3_scsi_async_poll(lc3_scsi_async_engine_t *engine, uint32_t current_ticks);
int16_t lc3_scsi_async_abort(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req);

#endif /* LC3_SCSI_ASYNC_H */
