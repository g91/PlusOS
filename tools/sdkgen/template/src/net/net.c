/* net.c - PlusNET: a small TCP/IP stack for PlusOS on the DaynaPORT adapter.
 *
 * Ethernet II, ARP, IPv4 (no fragments), ICMP echo, UDP, DHCP client, DNS
 * resolver, and a stop-and-wait TCP (one connection, one segment in flight,
 * in-order receive, retransmit with backoff). Enough for telnet and IRC.
 * Every field is accessed byte-wise: the 68000 faults on odd-address word
 * access and frames are not always nicely aligned.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusnet/net.h>
#include <plusnet/dayna.h>

#define FRAME_MAX  1518
#define ETH_HDR    14
#define IP_HDR     20
#define ETH_IP     0x0800
#define ETH_ARP    0x0806
#define IPP_ICMP   1
#define IPP_TCP    6
#define IPP_UDP    17
#define IP_BCAST   0xFFFFFFFFu

/* frame layout offsets in txbuf */
#define OFF_IP     ETH_HDR
#define OFF_L4     (ETH_HDR + IP_HDR)
#define OFF_UDPDATA (OFF_L4 + 8)

static uint8_t  g_mac[6];
static uint32_t g_ip, g_mask, g_gw, g_dns;
static int      g_link, g_up, g_scsi_id = 4;
static int      in_poll;
static uint16_t g_ident;
static struct net_stats st;

static uint8_t rxbuf[FRAME_MAX + 16] __attribute__((aligned(2)));
static uint8_t txbuf[FRAME_MAX + 16] __attribute__((aligned(2)));
static uint8_t arpbuf[64]            __attribute__((aligned(2)));

/* ------------------------------------------------------------------------ */
/* byte-order safe accessors (network order == 68000 order)                  */
/* ------------------------------------------------------------------------ */
static inline uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static inline uint32_t rd32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static inline void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static inline void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static uint32_t csum_acc(uint32_t s, const uint8_t *p, int n)
{
    while (n > 1) { s += (uint32_t)((p[0] << 8) | p[1]); p += 2; n -= 2; }
    if (n > 0) s += (uint32_t)(p[0] << 8);
    return s;
}
static uint16_t csum_fin(uint32_t s)
{
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)~s;
}

static uint32_t now(void) { return plusos->ticks(); }

static uint32_t rnd(void)
{
    static uint32_t x;
    if (!x) x = plusos->ticks() * 2654435761u + plusos->seconds();
    x = x * 1103515245u + 12345u;
    return x;
}

/* ------------------------------------------------------------------------ */
/* addresses & registry                                                      */
/* ------------------------------------------------------------------------ */
char *net_ip2str(uint32_t ip, char *b)
{
    snprintf(b, 16, "%d.%d.%d.%d", (int)((ip >> 24) & 255), (int)((ip >> 16) & 255),
             (int)((ip >> 8) & 255), (int)(ip & 255));
    return b;
}

int net_str2ip(const char *s, uint32_t *ip)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        int n = 0, digits = 0;
        while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; digits++; }
        if (!digits || n > 255) return -1;
        v = (v << 8) | (uint32_t)n;
        if (i < 3) { if (*s != '.') return -1; s++; }
    }
    if (*s) return -1;
    *ip = v;
    return 0;
}

static int reg_str(const char *key, char *buf, int size)
{
    struct plusos_reg_args a;
    memset(&a, 0, sizeof a);
    a.key = key; a.vbuf = buf; a.vsize = size;
    buf[0] = 0;
    return plusos_syscall(SYS_regget, &a) > 0 ? 0 : -1;
}

static int reg_ip(const char *key, uint32_t *ip)
{
    char v[32];
    if (reg_str(key, v, sizeof v)) return -1;
    return net_str2ip(v, ip);
}

/* ------------------------------------------------------------------------ */
/* Ethernet + ARP                                                            */
/* ------------------------------------------------------------------------ */
static int eth_out(const uint8_t *dst, uint16_t type, int payload_len)
{
    memcpy(txbuf, dst, 6);
    memcpy(txbuf + 6, g_mac, 6);
    wr16(txbuf + 12, type);
    st.tx_frames++;
    int r = dayna_send(txbuf, ETH_HDR + payload_len);
    if (r) st.tx_errors++;
    return r;
}

struct arpent { uint32_t ip; uint8_t mac[6]; uint8_t valid; };
static struct arpent arpc[8];
static int arp_next;

static void arp_learn(uint32_t ip, const uint8_t *mac)
{
    if (!ip) return;
    for (int i = 0; i < 8; i++)
        if (arpc[i].valid && arpc[i].ip == ip) { memcpy(arpc[i].mac, mac, 6); return; }
    struct arpent *e = &arpc[arp_next++ & 7];
    e->ip = ip; memcpy(e->mac, mac, 6); e->valid = 1;
}

static int arp_lookup(uint32_t ip, uint8_t *mac)
{
    for (int i = 0; i < 8; i++)
        if (arpc[i].valid && arpc[i].ip == ip) { memcpy(mac, arpc[i].mac, 6); return 0; }
    return -1;
}

/* ARP packets use their own small buffer so they never clobber a frame the
 * caller is still building in txbuf. */
static void arp_send(uint16_t op, uint32_t tip, const uint8_t *tmac, const uint8_t *dst)
{
    uint8_t *p = arpbuf;
    memcpy(p, dst, 6);
    memcpy(p + 6, g_mac, 6);
    wr16(p + 12, ETH_ARP);
    uint8_t *a = p + ETH_HDR;
    wr16(a, 1); wr16(a + 2, ETH_IP); a[4] = 6; a[5] = 4; wr16(a + 6, op);
    memcpy(a + 8, g_mac, 6); wr32(a + 14, g_ip);
    if (tmac) memcpy(a + 18, tmac, 6); else memset(a + 18, 0, 6);
    wr32(a + 24, tip);
    st.tx_frames++;
    if (dayna_send(arpbuf, ETH_HDR + 28)) st.tx_errors++;
}

static void net_poll_rx(void);

/* MAC for an IP: direct if on our subnet, else the gateway's. */
static int arp_resolve(uint32_t ip, uint8_t *mac)
{
    static const uint8_t bcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    if (ip == IP_BCAST || (g_mask && (ip | g_mask) == IP_BCAST && (ip & g_mask) == (g_ip & g_mask))) {
        memcpy(mac, bcast, 6);
        return 0;
    }
    uint32_t nh = ip;
    if (g_gw && g_mask && (ip & g_mask) != (g_ip & g_mask)) nh = g_gw;
    if (arp_lookup(nh, mac) == 0) return 0;
    if (in_poll) return -1;                      /* can't block inside the receive path */
    for (int tries = 0; tries < 3; tries++) {
        st.arp_requests++;
        arp_send(1, nh, NULL, bcast);
        uint32_t t0 = now();
        while (now() - t0 < 60) {
            net_poll_rx();
            if (arp_lookup(nh, mac) == 0) return 0;
        }
    }
    return -1;
}

static void arp_input(const uint8_t *f, int n)
{
    if (n < ETH_HDR + 28) return;
    const uint8_t *a = f + ETH_HDR;
    if (rd16(a) != 1 || rd16(a + 2) != ETH_IP) return;
    uint16_t op = rd16(a + 6);
    uint32_t spa = rd32(a + 14), tpa = rd32(a + 24);
    arp_learn(spa, a + 8);
    if (op == 1 && g_ip && tpa == g_ip) arp_send(2, spa, a + 8, a + 8);
}

/* ------------------------------------------------------------------------ */
/* IPv4                                                                      */
/* ------------------------------------------------------------------------ */
/* Send the L4 payload already placed at txbuf+OFF_L4. */
static int ip_send(uint32_t dst, uint8_t proto, int payload_len)
{
    uint8_t *ih = txbuf + OFF_IP;
    ih[0] = 0x45; ih[1] = 0;
    wr16(ih + 2, (uint16_t)(IP_HDR + payload_len));
    wr16(ih + 4, g_ident++);
    wr16(ih + 6, 0);
    ih[8] = 64; ih[9] = proto;
    wr16(ih + 10, 0);
    wr32(ih + 12, g_ip);
    wr32(ih + 16, dst);
    wr16(ih + 10, csum_fin(csum_acc(0, ih, IP_HDR)));
    uint8_t mac[6];
    if (dst == IP_BCAST) memset(mac, 0xFF, 6);
    else if (arp_resolve(dst, mac) < 0) return -1;
    return eth_out(mac, ETH_IP, IP_HDR + payload_len);
}

/* L4 checksum with the IPv4 pseudo header */
static uint16_t l4_csum(uint32_t src, uint32_t dst, uint8_t proto, const uint8_t *p, int len)
{
    uint8_t ph[12];
    wr32(ph, src); wr32(ph + 4, dst); ph[8] = 0; ph[9] = proto; wr16(ph + 10, (uint16_t)len);
    uint32_t s = csum_acc(0, ph, 12);
    s = csum_acc(s, p, len);
    uint16_t c = csum_fin(s);
    return c ? c : 0xFFFF;
}

/* ------------------------------------------------------------------------ */
/* ICMP                                                                      */
/* ------------------------------------------------------------------------ */
static uint16_t ping_id = 0x504F;   /* 'PO' */
static int      ping_got;
static uint16_t ping_seq_got;

static void icmp_input(uint32_t src, const uint8_t *p, int len)
{
    if (len < 8) return;
    if (p[0] == 8) {                               /* echo request -> reply */
        if (len > FRAME_MAX - OFF_L4) return;
        uint8_t *r = txbuf + OFF_L4;
        memcpy(r, p, (size_t)len);
        r[0] = 0; wr16(r + 2, 0);
        wr16(r + 2, csum_fin(csum_acc(0, r, len)));
        ip_send(src, IPP_ICMP, len);
    } else if (p[0] == 0) {                        /* echo reply */
        if (rd16(p + 4) == ping_id) { ping_got = 1; ping_seq_got = rd16(p + 6); }
    }
}

int net_ping(uint32_t ip, uint16_t seq, int timeout_ticks)
{
    if (!g_up) return -1;
    uint8_t *r = txbuf + OFF_L4;
    int len = 8 + 32;
    r[0] = 8; r[1] = 0; wr16(r + 2, 0);
    wr16(r + 4, ping_id); wr16(r + 6, seq);
    for (int i = 0; i < 32; i++) r[8 + i] = (uint8_t)('a' + (i % 26));
    wr16(r + 2, csum_fin(csum_acc(0, r, len)));
    ping_got = 0;
    uint32_t t0 = now();
    if (ip_send(ip, IPP_ICMP, len) < 0) return -1;
    while (now() - t0 < (uint32_t)timeout_ticks) {
        net_poll();
        if (ping_got && ping_seq_got == seq) return (int)(now() - t0);
    }
    return -1;
}

/* ------------------------------------------------------------------------ */
/* UDP                                                                       */
/* ------------------------------------------------------------------------ */
static uint16_t udp_port;                        /* the application socket */
static uint8_t  udp_rx[1472];
static int      udp_rxlen;
static uint32_t udp_from;
static uint16_t udp_fport;

static void dhcp_input(const uint8_t *d, int len);
static void dns_input(const uint8_t *d, int len);
static int  dhcp_active, dns_active;
static uint16_t dns_lport;

/* data already at txbuf+OFF_UDPDATA */
static int udp_out(uint32_t ip, uint16_t dport, uint16_t sport, int len)
{
    uint8_t *uh = txbuf + OFF_L4;
    wr16(uh, sport); wr16(uh + 2, dport); wr16(uh + 4, (uint16_t)(8 + len)); wr16(uh + 6, 0);
    wr16(uh + 6, l4_csum(g_ip, ip, IPP_UDP, uh, 8 + len));
    return ip_send(ip, IPP_UDP, 8 + len);
}

int net_udp_bind(uint16_t port) { udp_port = port; udp_rxlen = 0; return 0; }

int net_udp_send(uint32_t ip, uint16_t port, const void *data, int len)
{
    if (!g_up || len < 0 || len > 1472) return -1;
    memcpy(txbuf + OFF_UDPDATA, data, (size_t)len);
    return udp_out(ip, port, udp_port ? udp_port : 49999, len);
}

int net_udp_recv(void *buf, int max, uint32_t *from_ip, uint16_t *from_port)
{
    if (!udp_rxlen) return 0;
    int n = udp_rxlen < max ? udp_rxlen : max;
    memcpy(buf, udp_rx, (size_t)n);
    if (from_ip) *from_ip = udp_from;
    if (from_port) *from_port = udp_fport;
    udp_rxlen = 0;
    return n;
}

static void udp_input(uint32_t src, const uint8_t *uh, int len)
{
    if (len < 8) return;
    uint16_t sport = rd16(uh), dport = rd16(uh + 2);
    int dlen = rd16(uh + 4) - 8;
    if (dlen < 0 || dlen > len - 8) dlen = len - 8;
    const uint8_t *d = uh + 8;
    if (dhcp_active && dport == 68) { dhcp_input(d, dlen); return; }
    if (dns_active && dport == dns_lport) { dns_input(d, dlen); return; }
    if (udp_port && dport == udp_port) {
        if (dlen > (int)sizeof udp_rx) dlen = sizeof udp_rx;
        memcpy(udp_rx, d, (size_t)dlen);
        udp_rxlen = dlen; udp_from = src; udp_fport = sport;
    }
}

/* ------------------------------------------------------------------------ */
/* DHCP client                                                               */
/* ------------------------------------------------------------------------ */
static int      dh_state;          /* 1 discover sent, 2 offer in, 3 request sent, 4 bound */
static uint32_t dh_xid, dh_yiaddr, dh_server, dh_mask, dh_gw, dh_dns;

static void dhcp_send(int type)
{
    uint8_t *d = txbuf + OFF_UDPDATA;
    memset(d, 0, 300);
    d[0] = 1; d[1] = 1; d[2] = 6; d[3] = 0;
    wr32(d + 4, dh_xid);
    wr16(d + 10, 0x8000);                             /* broadcast reply, please */
    memcpy(d + 28, g_mac, 6);
    d[236] = 0x63; d[237] = 0x82; d[238] = 0x53; d[239] = 0x63;
    int o = 240;
    d[o++] = 53; d[o++] = 1; d[o++] = (uint8_t)type;
    if (type == 3) {
        d[o++] = 50; d[o++] = 4; wr32(d + o, dh_yiaddr); o += 4;
        d[o++] = 54; d[o++] = 4; wr32(d + o, dh_server); o += 4;
    }
    d[o++] = 55; d[o++] = 3; d[o++] = 1; d[o++] = 3; d[o++] = 6;
    d[o++] = 12; d[o++] = 6; memcpy(d + o, "plusos", 6); o += 6;
    d[o++] = 255;
    int len = o < 300 ? 300 : o;
    udp_out(IP_BCAST, 67, 68, len);
}

static void dhcp_input(const uint8_t *d, int len)
{
    if (len < 240 || d[0] != 2 || rd32(d + 4) != dh_xid) return;
    if (d[236] != 0x63 || d[237] != 0x82 || d[238] != 0x53 || d[239] != 0x63) return;
    int type = 0; uint32_t mask = 0, gw = 0, dns = 0, server = 0;
    int o = 240;
    while (o + 1 < len) {
        int opt = d[o], ol;
        if (opt == 255) break;
        if (opt == 0) { o++; continue; }
        ol = d[o + 1];
        if (o + 2 + ol > len) break;
        const uint8_t *v = d + o + 2;
        switch (opt) {
        case 53: type = v[0]; break;
        case 1:  if (ol >= 4) mask = rd32(v); break;
        case 3:  if (ol >= 4) gw = rd32(v); break;
        case 6:  if (ol >= 4) dns = rd32(v); break;
        case 54: if (ol >= 4) server = rd32(v); break;
        }
        o += 2 + ol;
    }
    if (type == 2 && dh_state == 1) {
        dh_yiaddr = rd32(d + 16); dh_server = server; dh_mask = mask; dh_gw = gw; dh_dns = dns;
        dh_state = 2;
    } else if (type == 5 && dh_state == 3) {
        dh_yiaddr = rd32(d + 16);
        if (mask) dh_mask = mask;
        if (gw) dh_gw = gw;
        if (dns) dh_dns = dns;
        dh_state = 4;
    } else if (type == 6 && dh_state == 3) {
        dh_state = 0;                                  /* NAK */
    }
}

static int dhcp_wait(int want, int ticks)
{
    uint32_t t0 = now();
    while (now() - t0 < (uint32_t)ticks) {
        net_poll();
        if (dh_state == want) return 0;
        if (dh_state == 0) return -1;
    }
    return -1;
}

int net_dhcp(void)
{
    if (!g_link) return -1;
    uint32_t save_ip = g_ip;
    g_ip = 0; g_up = 0;
    dhcp_active = 1;
    int ok = -1;
    for (int tries = 0; tries < 3 && ok; tries++) {
        dh_xid = rnd();
        dh_state = 1;
        dhcp_send(1);
        if (dhcp_wait(2, 60 * 4)) continue;
        dh_state = 3;
        dhcp_send(3);
        if (dhcp_wait(4, 60 * 4)) continue;
        ok = 0;
    }
    dhcp_active = 0;
    if (ok) { g_ip = save_ip; g_up = save_ip != 0; return -1; }
    g_ip = dh_yiaddr;
    g_mask = dh_mask ? dh_mask : 0xFFFFFF00u;
    g_gw = dh_gw;
    g_dns = dh_dns ? dh_dns : dh_gw;
    g_up = 1;
    return 0;
}

/* ------------------------------------------------------------------------ */
/* DNS resolver                                                              */
/* ------------------------------------------------------------------------ */
static uint16_t dns_id;
static int      dns_state;           /* 1 waiting, 2 answered, 3 no such name */
static uint32_t dns_result;

static int dns_skip_name(const uint8_t *d, int len, int p)
{
    while (p < len) {
        int l = d[p];
        if (l == 0) return p + 1;
        if ((l & 0xC0) == 0xC0) return p + 2;
        p += 1 + l;
    }
    return len;
}

static void dns_input(const uint8_t *d, int len)
{
    if (len < 12 || rd16(d) != dns_id) return;
    if ((d[3] & 0x0F) == 3) { dns_state = 3; return; }  /* NXDOMAIN */
    int qd = rd16(d + 4), an = rd16(d + 6);
    int p = 12;
    for (int i = 0; i < qd; i++) { p = dns_skip_name(d, len, p); p += 4; }
    for (int i = 0; i < an && p + 10 <= len; i++) {
        p = dns_skip_name(d, len, p);
        if (p + 10 > len) break;
        uint16_t type = rd16(d + p), rdlen = rd16(d + p + 8);
        if (type == 1 && rdlen == 4 && p + 14 <= len) { dns_result = rd32(d + p + 10); dns_state = 2; return; }
        p += 10 + rdlen;
    }
}

int net_resolve(const char *host, uint32_t *ip)
{
    if (net_str2ip(host, ip) == 0) return 0;
    if (!g_up || !g_dns) return -1;
    int hl = (int)strlen(host);
    if (hl < 1 || hl > 200) return -1;
    for (int tries = 0; tries < 3; tries++) {
        uint8_t *d = txbuf + OFF_UDPDATA;
        dns_id = (uint16_t)rnd();
        wr16(d, dns_id); wr16(d + 2, 0x0100); wr16(d + 4, 1); wr16(d + 6, 0); wr16(d + 8, 0); wr16(d + 10, 0);
        int o = 12, start = 0;
        for (int i = 0; i <= hl; i++) {
            if (host[i] == '.' || host[i] == 0) {
                int l = i - start;
                if (l <= 0 || l > 63) return -1;
                d[o++] = (uint8_t)l;
                memcpy(d + o, host + start, (size_t)l); o += l;
                start = i + 1;
            }
        }
        d[o++] = 0;
        wr16(d + o, 1); wr16(d + o + 2, 1); o += 4;
        dns_lport = (uint16_t)(0xC000 | (rnd() & 0x0FFF));
        dns_active = 1; dns_state = 1;
        if (udp_out(g_dns, 53, dns_lport, o) < 0) { dns_active = 0; return -1; }
        uint32_t t0 = now();
        while (now() - t0 < 60 * 3) {
            net_poll();
            if (dns_state == 2) { dns_active = 0; *ip = dns_result; return 0; }
            if (dns_state == 3) { dns_active = 0; return -2; }
        }
        dns_active = 0;
    }
    return -1;
}

/* ------------------------------------------------------------------------ */
/* TCP (one connection, stop-and-wait)                                       */
/* ------------------------------------------------------------------------ */
#define TF_FIN 0x01
#define TF_SYN 0x02
#define TF_RST 0x04
#define TF_PSH 0x08
#define TF_ACK 0x10
#define TCP_MSS   536
#define TCP_RXBUF 4096
#define TCP_RTO0  60

enum { TS_CLOSED, TS_SYN_SENT, TS_ESTAB, TS_FIN_WAIT1, TS_FIN_WAIT2, TS_CLOSE_WAIT, TS_LAST_ACK };

static struct {
    int state;
    uint32_t rip; uint16_t rport, lport;
    uint32_t snd_una, snd_nxt, rcv_nxt;
    uint16_t snd_wnd;
    uint8_t  pend_flags; uint8_t pend[TCP_MSS]; int pend_len; int pend_active; int pend_tries;
    uint32_t pend_t, rto;
    uint8_t  rx[TCP_RXBUF]; int rx_r, rx_n;
    int err;
} tc;

static int32_t seqdiff(uint32_t a, uint32_t b) { return (int32_t)(a - b); }

static void tcp_emit(uint8_t flags, uint32_t seq, const uint8_t *data, int len, int mss_opt)
{
    uint8_t *th = txbuf + OFF_L4;
    int hl = mss_opt ? 24 : 20;
    wr16(th, tc.lport); wr16(th + 2, tc.rport);
    wr32(th + 4, seq); wr32(th + 8, tc.rcv_nxt);
    th[12] = (uint8_t)((hl / 4) << 4); th[13] = flags;
    wr16(th + 14, (uint16_t)(TCP_RXBUF - tc.rx_n));
    wr16(th + 16, 0); wr16(th + 18, 0);
    if (mss_opt) { th[20] = 2; th[21] = 4; wr16(th + 22, 1460); }
    if (len) memcpy(th + hl, data, (size_t)len);
    wr16(th + 16, l4_csum(g_ip, tc.rip, IPP_TCP, th, hl + len));
    ip_send(tc.rip, IPP_TCP, hl + len);
}

static void tcp_send_pending(void)
{
    uint8_t f = tc.pend_flags;
    int syn = (f & TF_SYN) != 0;
    if (!syn) f |= TF_ACK;
    tcp_emit(f, tc.snd_una, tc.pend, tc.pend_len, syn);
    tc.pend_t = now();
}

static void tcp_set_pending(uint8_t flags, const uint8_t *data, int len)
{
    tc.pend_flags = flags;
    tc.pend_len = len;
    if (len) memcpy(tc.pend, data, (size_t)len);
    tc.pend_active = 1; tc.pend_tries = 0; tc.rto = TCP_RTO0;
    tc.snd_nxt = tc.snd_una + (uint32_t)len + ((flags & (TF_SYN | TF_FIN)) ? 1u : 0u);
    tcp_send_pending();
}

static void tcp_timer(void)
{
    if (!tc.pend_active || tc.state == TS_CLOSED) return;
    if (now() - tc.pend_t < tc.rto) return;
    if (++tc.pend_tries > 6) { tc.state = TS_CLOSED; tc.pend_active = 0; tc.err = 1; return; }
    tc.rto = tc.rto < 480 ? tc.rto * 2 : 480;
    st.tcp_retransmits++;
    tcp_send_pending();
}

static void rx_put(const uint8_t *d, int n)
{
    int w = (tc.rx_r + tc.rx_n) % TCP_RXBUF;
    for (int i = 0; i < n; i++) { tc.rx[w] = d[i]; w = (w + 1) % TCP_RXBUF; }
    tc.rx_n += n;
}

static void tcp_input(uint32_t src, const uint8_t *th, int tlen)
{
    if (tlen < 20 || tc.state == TS_CLOSED) return;
    if (rd16(th) != tc.rport || rd16(th + 2) != tc.lport || src != tc.rip) return;
    int hl = (th[12] >> 4) * 4;
    if (hl < 20 || hl > tlen) return;
    const uint8_t *data = th + hl;
    int dlen = tlen - hl;
    uint32_t seq = rd32(th + 4), ack = rd32(th + 8);
    uint8_t flags = th[13];

    if (flags & TF_RST) { tc.state = TS_CLOSED; tc.pend_active = 0; tc.err = 2; return; }

    if (tc.state == TS_SYN_SENT) {
        if ((flags & TF_SYN) && (flags & TF_ACK) && ack == tc.snd_nxt) {
            tc.rcv_nxt = seq + 1;
            tc.snd_una = ack;
            tc.pend_active = 0;
            tc.snd_wnd = rd16(th + 14);
            tc.state = TS_ESTAB;
            tcp_emit(TF_ACK, tc.snd_nxt, NULL, 0, 0);
        }
        return;
    }

    if (flags & TF_ACK) {
        if (seqdiff(ack, tc.snd_una) > 0 && seqdiff(ack, tc.snd_nxt) <= 0) {
            if (ack == tc.snd_nxt) {
                tc.snd_una = ack;
                if (tc.pend_active) {
                    tc.pend_active = 0;
                    if (tc.pend_flags & TF_FIN) {
                        if (tc.state == TS_FIN_WAIT1) tc.state = TS_FIN_WAIT2;
                        else if (tc.state == TS_LAST_ACK) tc.state = TS_CLOSED;
                    }
                }
            } else {
                /* partial ack of our segment: shift and resend the rest */
                int done = (int)seqdiff(ack, tc.snd_una);
                if (done > 0 && done < tc.pend_len) {
                    memmove(tc.pend, tc.pend + done, (size_t)(tc.pend_len - done));
                    tc.pend_len -= done;
                }
                tc.snd_una = ack;
                tcp_send_pending();
            }
        }
        tc.snd_wnd = rd16(th + 14);
    }

    int accepted = 0;
    if (dlen > 0) {
        int32_t off = seqdiff(seq, tc.rcv_nxt);
        if (off == 0) {
            int room = TCP_RXBUF - tc.rx_n;
            int n = dlen < room ? dlen : room;
            if (n > 0) { rx_put(data, n); tc.rcv_nxt += (uint32_t)n; accepted = n; }
        } else if (off < 0 && off + dlen > 0) {        /* overlap: take the new tail */
            int skip = (int)(-off);
            int room = TCP_RXBUF - tc.rx_n;
            int n = (dlen - skip) < room ? (dlen - skip) : room;
            if (n > 0) { rx_put(data + skip, n); tc.rcv_nxt += (uint32_t)n; accepted = n + skip; }
        }
    }
    int fin_in_order = (flags & TF_FIN) && seqdiff(seq + (uint32_t)dlen, tc.rcv_nxt) == 0 && (dlen == 0 || accepted == dlen);
    if (fin_in_order) {
        tc.rcv_nxt++;
        if (tc.state == TS_ESTAB) tc.state = TS_CLOSE_WAIT;
        else if (tc.state == TS_FIN_WAIT2 || tc.state == TS_FIN_WAIT1) { tc.state = TS_CLOSED; tc.pend_active = 0; }
        tcp_emit(TF_ACK, tc.snd_nxt, NULL, 0, 0);
        return;
    }
    if (dlen > 0) tcp_emit(TF_ACK, tc.snd_nxt, NULL, 0, 0);
}

int net_tcp_connect(uint32_t ip, uint16_t port, int timeout_ticks)
{
    if (!g_up) return -1;
    if (tc.state != TS_CLOSED) net_tcp_close();
    memset(&tc, 0, sizeof tc);
    tc.rip = ip; tc.rport = port;
    tc.lport = (uint16_t)(49152 + (rnd() & 0x3FFF));
    tc.snd_una = rnd();
    tc.snd_wnd = 1460;
    tc.state = TS_SYN_SENT;
    tcp_set_pending(TF_SYN, NULL, 0);
    uint32_t t0 = now();
    while (now() - t0 < (uint32_t)timeout_ticks) {
        net_poll();
        if (tc.state == TS_ESTAB) return 0;
        if (tc.state == TS_CLOSED) return -2;
    }
    tc.state = TS_CLOSED; tc.pend_active = 0;
    return -3;
}

static int tcp_wait_clear(int ticks)
{
    uint32_t t0 = now();
    while (tc.pend_active) {
        net_poll();
        if (tc.state == TS_CLOSED) return -1;
        if (now() - t0 > (uint32_t)ticks) return -1;
    }
    return 0;
}

int net_tcp_send(const void *data, int len)
{
    const uint8_t *p = data;
    if (tc.state != TS_ESTAB && tc.state != TS_CLOSE_WAIT) return -1;
    while (len > 0) {
        if (tcp_wait_clear(60 * 30)) return -1;
        if (tc.state != TS_ESTAB && tc.state != TS_CLOSE_WAIT) return -1;
        int n = len < TCP_MSS ? len : TCP_MSS;
        if (tc.snd_wnd && n > tc.snd_wnd) n = tc.snd_wnd;
        if (n <= 0) n = 1;
        tcp_set_pending(TF_PSH, p, n);
        p += n; len -= n;
    }
    return 0;
}

int net_tcp_recv(void *buf, int max)
{
    uint8_t *b = buf;
    int before = tc.rx_n;
    int n = tc.rx_n < max ? tc.rx_n : max;
    for (int i = 0; i < n; i++) { b[i] = tc.rx[tc.rx_r]; tc.rx_r = (tc.rx_r + 1) % TCP_RXBUF; }
    tc.rx_n -= n;
    if (n > 0 && (TCP_RXBUF - before) < 1460 && tc.state == TS_ESTAB)
        tcp_emit(TF_ACK, tc.snd_nxt, NULL, 0, 0);         /* window update */
    if (n == 0 && (tc.state == TS_CLOSED || tc.state == TS_CLOSE_WAIT)) return -1;
    return n;
}

int net_tcp_state(void)
{
    switch (tc.state) {
    case TS_CLOSED:   return NET_TCP_CLOSED;
    case TS_SYN_SENT: return NET_TCP_CONNECTING;
    case TS_ESTAB:    return NET_TCP_OPEN;
    default:          return NET_TCP_PEER_CLOSED;
    }
}

void net_tcp_close(void)
{
    if (tc.state == TS_ESTAB || tc.state == TS_CLOSE_WAIT) {
        tcp_wait_clear(60 * 2);
        if (tc.state == TS_ESTAB || tc.state == TS_CLOSE_WAIT) {
            tc.state = tc.state == TS_ESTAB ? TS_FIN_WAIT1 : TS_LAST_ACK;
            tcp_set_pending(TF_FIN, NULL, 0);
            uint32_t t0 = now();
            while (tc.state != TS_CLOSED && now() - t0 < 60 * 3) net_poll();
        }
    }
    tc.state = TS_CLOSED; tc.pend_active = 0;
}

/* ------------------------------------------------------------------------ */
/* receive path                                                              */
/* ------------------------------------------------------------------------ */
static void ip_input(const uint8_t *f, int n)
{
    if (n < ETH_HDR + IP_HDR) return;
    const uint8_t *ih = f + ETH_HDR;
    if ((ih[0] >> 4) != 4) return;
    int ihl = (ih[0] & 15) * 4;
    int tot = rd16(ih + 2);
    if (ihl < 20 || tot < ihl) return;
    if (tot > n - ETH_HDR) tot = n - ETH_HDR;             /* the adapter pads short frames */
    if (rd16(ih + 6) & 0x3FFF) return;                     /* fragments: not supported */
    uint32_t src = rd32(ih + 12), dst = rd32(ih + 16);
    int for_us = (g_ip && dst == g_ip) || dst == IP_BCAST ||
                 (g_ip && g_mask && dst == (g_ip | ~g_mask)) || (!g_ip && dhcp_active);
    if (!for_us) return;
    if (g_ip && dst == g_ip) arp_learn(src, f + 6);
    const uint8_t *p = ih + ihl;
    int plen = tot - ihl;
    switch (ih[9]) {
    case IPP_ICMP: icmp_input(src, p, plen); break;
    case IPP_UDP:  udp_input(src, p, plen); break;
    case IPP_TCP:  tcp_input(src, p, plen); break;
    }
}

static void eth_input(const uint8_t *f, int n)
{
    if (n < ETH_HDR) return;
    uint16_t type = rd16(f + 12);
    if (type == ETH_ARP) arp_input(f, n);
    else if (type == ETH_IP) ip_input(f, n);
}

static void net_poll_rx(void)
{
    if (!g_link || in_poll) return;
    in_poll = 1;
    for (int k = 0; k < 8; k++) {
        int n = dayna_recv(rxbuf, FRAME_MAX);
        if (n <= 0) { if (n < 0) st.rx_errors++; break; }
        st.rx_frames++;
        eth_input(rxbuf, n);
    }
    in_poll = 0;
}

void net_poll(void)
{
    net_poll_rx();
    tcp_timer();
}

/* ------------------------------------------------------------------------ */
/* bring-up                                                                  */
/* ------------------------------------------------------------------------ */
int  net_link(void) { return g_link; }
int  net_up(void)   { return g_up; }
const uint8_t *net_mac(void) { return g_mac; }
uint32_t net_ip(void)   { return g_ip; }
uint32_t net_mask(void) { return g_mask; }
uint32_t net_gw(void)   { return g_gw; }
uint32_t net_dns(void)  { return g_dns; }
const struct net_stats *net_get_stats(void) { return &st; }

int net_init(void)
{
    char v[40], s[16];
    if (reg_str("net.scsi", v, sizeof v) == 0 && v[0] >= '0' && v[0] <= '7') g_scsi_id = v[0] - '0';

    if (!g_link) {
        printf("net: DaynaPORT at SCSI ID %d... ", g_scsi_id);
        int r = dayna_init(g_scsi_id, g_mac);
        if (r) {
            if (r == -1)      printf("no device\n");
            else if (r == -2) printf("not a DaynaPORT\n");
            else              printf("SCSI error %d at %s\n", dayna_last_error(), dayna_last_step());
            return -1;
        }
        g_link = 1;
        printf("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
    }
    if (g_up) return 0;

    int want_dhcp = reg_str("net.dhcp", v, sizeof v) == 0 && v[0] == '1';
    uint32_t ip = 0;
    if (!want_dhcp && reg_ip("net.ip", &ip) == 0 && ip) {
        g_ip = ip;
        if (reg_ip("net.mask", &g_mask)) g_mask = 0xFFFFFF00u;
        if (reg_ip("net.gw", &g_gw))     g_gw = (ip & g_mask) | 1;
        if (reg_ip("net.dns", &g_dns))   g_dns = g_gw;
        g_up = 1;
        printf("net: static IP %s", net_ip2str(g_ip, s));
        printf(" mask %s", net_ip2str(g_mask, s));
        printf(" gw %s", net_ip2str(g_gw, s));
        printf(" dns %s\n", net_ip2str(g_dns, s));
        return 0;
    }
    printf("net: DHCP... ");
    if (net_dhcp()) { printf("no reply (set a static IP with: net ip A.B.C.D MASK GW DNS)\n"); return -2; }
    printf("IP %s", net_ip2str(g_ip, s));
    printf(" mask %s", net_ip2str(g_mask, s));
    printf(" gw %s", net_ip2str(g_gw, s));
    printf(" dns %s\n", net_ip2str(g_dns, s));
    return 0;
}
