/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 186: SCSI Manager 4.3 Async I/O Completion & Queue Management (GD)
 */

#include "lc3_scsi_async.h"
#include <string.h>

void lc3_scsi_async_init(lc3_scsi_async_engine_t *engine)
{
    if (!engine) return;
    memset(engine, 0, sizeof(lc3_scsi_async_engine_t));
}

int16_t lc3_scsi_async_queue_request(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req)
{
    if (!engine || !req) return -1;
    if (engine->count >= LC3_SCSI_MAX_PENDING) return -2; /* queue full */
    if (req->target_id >= LC3_SCSI_MAX_DEVICES) return -3; /* invalid target */

    req->status = LC3_SCSI_STAT_PENDING;
    req->next = 0;

    if (!engine->head) {
        engine->head = req;
        engine->tail = req;
    } else {
        engine->tail->next = req;
        engine->tail = req;
    }

    engine->count++;
    engine->bus_busy[req->target_id] = true;
    return 0;
}

void lc3_scsi_async_complete_request(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req, uint8_t status)
{
    if (!engine || !req) return;

    req->status = status;
    if (req->target_id < LC3_SCSI_MAX_DEVICES) {
        engine->bus_busy[req->target_id] = false;
    }

    /* Dequeue */
    lc3_scsi_async_dequeue(engine, req);

    /* Invoke user completion routine */
    if (req->completion) {
        req->completion(req);
    }
}

int16_t lc3_scsi_async_dequeue(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req)
{
    if (!engine || !req || !engine->head) return -1;

    lc3_scsi_req_t *curr = engine->head;
    lc3_scsi_req_t *prev = 0;

    while (curr) {
        if (curr == req) {
            if (prev) {
                prev->next = curr->next;
            } else {
                engine->head = curr->next;
            }

            if (curr == engine->tail) {
                engine->tail = prev;
            }

            curr->next = 0;
            if (engine->count > 0) engine->count--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -2; /* Not found */
}

void lc3_scsi_async_poll(lc3_scsi_async_engine_t *engine, uint32_t current_ticks)
{
    if (!engine) return;

    lc3_scsi_req_t *curr = engine->head;
    while (curr) {
        lc3_scsi_req_t *next = curr->next;
        if (curr->status == LC3_SCSI_STAT_PENDING && curr->timeout_ticks > 0) {
            if (current_ticks - curr->start_tick >= curr->timeout_ticks) {
                lc3_scsi_async_complete_request(engine, curr, LC3_SCSI_STAT_TIMEOUT);
            }
        }
        curr = next;
    }
}

int16_t lc3_scsi_async_abort(lc3_scsi_async_engine_t *engine, lc3_scsi_req_t *req)
{
    if (!engine || !req) return -1;

    if (req->status == LC3_SCSI_STAT_PENDING) {
        lc3_scsi_async_complete_request(engine, req, LC3_SCSI_STAT_ABORTED);
        return 0;
    }

    return -2; /* Not active or already finished */
}
