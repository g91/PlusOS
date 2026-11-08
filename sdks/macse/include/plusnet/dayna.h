/* plusnet/dayna.h - driver for the DaynaPORT SCSI/Link Ethernet adapter, as
 * emulated by BlueSCSI V2 (WiFi) at a SCSI ID. Sends and receives raw
 * Ethernet frames; the TCP/IP stack in net.c sits on top. */
#ifndef PLUSNET_DAYNA_H
#define PLUSNET_DAYNA_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAYNA_FRAME_MAX 1518          /* Ethernet frame incl. header, excl. CRC */

/* Bring the adapter up: INQUIRY, enable the interface, read the MAC.
 * Returns 0, or -1 (no device at that SCSI ID), -2 (device is not a
 * DaynaPORT), -3 (SCSI error - see dayna_last_error). */
int dayna_init(int scsi_id, uint8_t mac[6]);

/* Send one Ethernet frame (header + payload, no CRC). 0 or <0. */
int dayna_send(const uint8_t *frame, int len);

/* Receive one frame into buf (capacity max). Returns its length with the
 * CRC stripped, 0 if nothing is waiting, <0 on a SCSI error. */
int dayna_recv(uint8_t *buf, int max);

/* Disable then re-enable the interface (recovers a "dropped packets" state). */
int dayna_reset(void);

/* Diagnostics: the SCSI Manager error of the last failed step, and which
 * step ("get", "select", "cmd", "read", "write", "complete", "status"). */
int         dayna_last_error(void);
const char *dayna_last_step(void);

#ifdef __cplusplus
}
#endif
#endif /* PLUSNET_DAYNA_H */
