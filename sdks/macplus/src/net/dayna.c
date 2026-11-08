/* dayna.c - DaynaPORT SCSI/Link (BlueSCSI V2 WiFi) Ethernet driver.
 *
 * Protocol (Dayna SLINKCMD.TXT, cross-checked against the BlueSCSI firmware
 * and a working host driver):
 *   INQUIRY   12 00 00 00 24 00        -> 36 bytes, "Dayna" at offset 8
 *   ENABLE    0E 00 00 00 00 80        (80 = enable, 00 = disable); then wait 0.5 s
 *   STATS     09 00 00 00 12 00        -> 18 bytes: MAC at 0..5, 3 counters
 *   READ      08 00 00 05 FC 80        -> 6-byte header + frame
 *               header: [len:2 BE][flags:4]; len counts the frame + 4 CRC
 *               bytes; len 0 = nothing waiting; flags FFFFFFFF = a packet was
 *               dropped (reset the interface). Byte 5 = 80 is single-record
 *               polled mode (C0 would make BlueSCSI return several records).
 *   WRITE     0A 00 00 LL LL 80        LL = frame length + 8; data-out is
 *               [len:2 BE][00 00][frame][00 00 00 00]
 *
 * The ROM SCSI Manager transfers with Transfer Instruction Blocks. When no
 * packet is waiting, BlueSCSI sends only the 6-byte header and goes to the
 * status phase, so a single 1532-byte read would end in scPhaseErr. We read
 * the header with one TIB, then the frame with a second, exactly-sized TIB.
 */
#include <plusos/plusos.h>
#include <plusnet/scsimgr.h>
#include <plusnet/dayna.h>

static int g_id = 4;
static int g_ok;
static int g_err;
static const char *g_step = "";

static uint8_t g_hdr[8]   __attribute__((aligned(2)));
static uint8_t g_wbuf[DAYNA_FRAME_MAX + 16] __attribute__((aligned(2)));
static uint8_t g_cdb[6]   __attribute__((aligned(2)));
static uint8_t g_small[64] __attribute__((aligned(2)));

int         dayna_last_error(void) { return g_err; }
const char *dayna_last_step(void)  { return g_step; }

static void fail(const char *step, int e) { g_step = step; g_err = e; }

/* Arbitrate for the bus and select the adapter. 0 ok, -1 busy, -2 no device. */
static int open_target(void)
{
    int16_t e = 0;
    for (int tries = 0; tries < 30; tries++) {
        e = SCSIGet();
        if (e == 0) break;
        plusos->sleep_ticks(1);
    }
    if (e) { fail("get", e); return -1; }
    e = SCSISelect((int16_t)g_id);
    if (e) { fail("select", e); return -2; }
    return 0;
}

static int send_cdb(const uint8_t *cdb)
{
    memcpy(g_cdb, cdb, 6);
    int16_t e = SCSICmd(g_cdb, 6);
    if (e) { fail("cmd", e); return -3; }
    return 0;
}

/* One TIB transfer. scPhaseErr means the target ended the data phase early
 * (fewer bytes than asked); callers that expect that treat it as success. */
static int tib_xfer(void *data, int len, int out, int tolerate_short)
{
    SCSIInstr tib[2];
    tib[0].scOpcode = scInc;  tib[0].scParam1 = (uint32_t)(uintptr_t)data; tib[0].scParam2 = (uint32_t)len;
    tib[1].scOpcode = scStop; tib[1].scParam1 = 0; tib[1].scParam2 = 0;
    int16_t e = out ? SCSIWrite(tib) : SCSIRead(tib);
    if (e == 0) return 0;
    if (e == scPhaseErr && tolerate_short) return 0;
    fail(out ? "write" : "read", e);
    return -4;
}

static int complete(void)
{
    int16_t stat = 0, msg = 0;
    int16_t e = SCSIComplete(&stat, &msg, 60 * 5);     /* 5 s */
    if (e) { fail("complete", e); return -5; }
    if (stat != 0) { fail("status", stat); return -6; }
    return 0;
}

/* A whole command with at most one data phase. */
static int command(const uint8_t *cdb, void *data, int len, int out, int tolerate_short)
{
    int r = open_target();
    if (r) return r;
    if ((r = send_cdb(cdb)) != 0) { complete(); return r; }
    if (len > 0 && data) {
        r = tib_xfer(data, len, out, tolerate_short);
        int c = complete();
        return r ? r : c;
    }
    return complete();
}

int dayna_init(int scsi_id, uint8_t mac[6])
{
    static const uint8_t cdb_inq[6]    = { 0x12, 0, 0, 0, 36, 0 };
    static const uint8_t cdb_enable[6] = { 0x0E, 0, 0, 0, 0, 0x80 };
    static const uint8_t cdb_stats[6]  = { 0x09, 0, 0, 0, 0x12, 0 };
    g_id = scsi_id;
    g_ok = 0;

    memset(g_small, 0, sizeof g_small);
    int r = command(cdb_inq, g_small, 36, 0, 1);
    if (r == -2) return -1;                              /* nothing at that ID */
    if (r) return -3;
    if (memcmp(g_small + 8, "Dayna", 5) != 0) return -2;

    if (command(cdb_enable, NULL, 0, 0, 0)) return -3;
    plusos->sleep_ticks(32);                             /* the spec: ~0.5 s after enable */

    memset(g_small, 0, sizeof g_small);
    if (command(cdb_stats, g_small, 18, 0, 1)) return -3;
    memcpy(mac, g_small, 6);
    g_ok = 1;
    return 0;
}

int dayna_reset(void)
{
    static const uint8_t cdb_disable[6] = { 0x0E, 0, 0, 0, 0, 0x00 };
    static const uint8_t cdb_enable[6]  = { 0x0E, 0, 0, 0, 0, 0x80 };
    command(cdb_disable, NULL, 0, 0, 0);
    plusos->sleep_ticks(6);
    int r = command(cdb_enable, NULL, 0, 0, 0);
    plusos->sleep_ticks(32);
    return r;
}

int dayna_send(const uint8_t *frame, int len)
{
    if (!g_ok) return -1;
    if (len <= 0 || len > DAYNA_FRAME_MAX) return -1;
    int total = len + 8;
    g_wbuf[0] = (uint8_t)(len >> 8);
    g_wbuf[1] = (uint8_t)len;
    g_wbuf[2] = 0;
    g_wbuf[3] = 0;
    memcpy(g_wbuf + 4, frame, (size_t)len);
    memset(g_wbuf + 4 + len, 0, 4);
    uint8_t cdb[6] = { 0x0A, 0, 0, (uint8_t)(total >> 8), (uint8_t)total, 0x80 };
    return command(cdb, g_wbuf, total, 1, 0);
}

int dayna_recv(uint8_t *buf, int max)
{
    static const uint8_t cdb_read[6] = { 0x08, 0, 0, 0x05, 0xFC, 0x80 };
    if (!g_ok) return -1;

    int r = open_target();
    if (r) return r;
    if ((r = send_cdb(cdb_read)) != 0) { complete(); return r; }

    memset(g_hdr, 0, sizeof g_hdr);
    r = tib_xfer(g_hdr, 6, 0, 1);
    if (r) { complete(); return r; }

    int len = (g_hdr[0] << 8) | g_hdr[1];
    uint32_t flags = ((uint32_t)g_hdr[2] << 24) | ((uint32_t)g_hdr[3] << 16) |
                     ((uint32_t)g_hdr[4] << 8) | g_hdr[5];

    if (flags == 0xFFFFFFFFu) {                          /* packets were dropped */
        complete();
        dayna_reset();
        return 0;
    }
    int n = 0;
    if (len > 0) {
        int want = len > max ? max : len;
        r = tib_xfer(buf, want, 0, 1);
        if (r == 0) n = want;
    }
    int c = complete();
    if (r) return r;
    if (c && n == 0) return c;
    if (n >= 4) n -= 4;                                  /* strip the CRC */
    else n = 0;
    return n;
}
