/*
 * lc3_scsi_esp.h - NCR 53C96 ESP SCSI Subsystem Declarations
 */

#ifndef LC3_SCSI_ESP_H
#define LC3_SCSI_ESP_H

#include <stdint.h>
#include <stdbool.h>

int16_t lc3_scsi_reset_bus(void);
int16_t lc3_scsi_get(void);
int16_t lc3_scsi_select(uint8_t target_id);
int16_t lc3_scsi_sel_atn(uint8_t target_id);
int16_t lc3_scsi_cmd(const uint8_t *cdb, uint8_t cdb_len);
int16_t lc3_scsi_read(uint8_t *buf, uint32_t count);
int16_t lc3_scsi_write(const uint8_t *buf, uint32_t count);
int16_t lc3_scsi_complete(int16_t *stat, int16_t *msg, uint32_t wait_ticks);
int16_t lc3_scsi_stat(void);
int16_t lc3_scsi_msg_in(uint8_t *msg_byte);
int16_t lc3_scsi_msg_out(uint8_t msg_byte);
int16_t lc3_scsi_dispatch(int16_t selector, void *param1, void *param2, void *param3);

/* Aliases for Toolbox SCSI Manager calls */
static inline int16_t scsiResetBus(void) { return lc3_scsi_reset_bus(); }
static inline int16_t scsiGet(void) { return lc3_scsi_get(); }
static inline int16_t scsiSelect(uint8_t id) { return lc3_scsi_select(id); }
static inline int16_t scsiSelAtn(uint8_t id) { return lc3_scsi_sel_atn(id); }
static inline int16_t scsiCmd(const void *cdb, uint8_t len) { return lc3_scsi_cmd((const uint8_t *)cdb, len); }
static inline int16_t scsiRead(void *buf, uint32_t count) { return lc3_scsi_read((uint8_t *)buf, count); }
static inline int16_t scsiWrite(const void *buf, uint32_t count) { return lc3_scsi_write((const uint8_t *)buf, count); }
static inline int16_t scsiComplete(int16_t *stat, int16_t *msg, uint32_t wait) { return lc3_scsi_complete(stat, msg, wait); }
static inline int16_t scsiStat(void) { return lc3_scsi_stat(); }

#endif /* LC3_SCSI_ESP_H */
