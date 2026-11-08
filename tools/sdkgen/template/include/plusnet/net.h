/* plusnet/net.h - PlusNET: TCP/IP for PlusOS over the BlueSCSI DaynaPORT.
 *
 *   net_init()   bring the adapter up and configure IP (registry or DHCP)
 *   net_poll()   service the network: call it often (every loop iteration)
 *   then: net_ping / net_resolve / UDP / one TCP connection at a time.
 *
 * Registry keys (set with the `net` tool or `reg`):
 *   net.scsi   SCSI ID of the DaynaPORT (default 4)
 *   net.dhcp   1 = configure with DHCP (default when net.ip is not set)
 *   net.ip  net.mask  net.gw  net.dns   static configuration
 * All multi-byte network fields are big-endian - the 68000's own byte order,
 * so no swapping is ever needed.
 */
#ifndef PLUSNET_NET_H
#define PLUSNET_NET_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- interface ---- */
int  net_init(void);                 /* 0 ok, -1 no adapter, -2 no IP config */
void net_poll(void);                 /* receive + dispatch frames, run timers */
int  net_link(void);                 /* adapter initialised */
int  net_up(void);                   /* IP configured */
const uint8_t *net_mac(void);
uint32_t net_ip(void);
uint32_t net_mask(void);
uint32_t net_gw(void);
uint32_t net_dns(void);
int  net_dhcp(void);                 /* (re)configure with DHCP: 0 ok */

struct net_stats {
    uint32_t rx_frames, tx_frames, rx_errors, tx_errors, arp_requests, tcp_retransmits;
};
const struct net_stats *net_get_stats(void);

/* ---- addresses ---- */
char *net_ip2str(uint32_t ip, char *buf);        /* buf >= 16 bytes */
int   net_str2ip(const char *s, uint32_t *ip);   /* "a.b.c.d" -> 0 ok */
int   net_resolve(const char *host, uint32_t *ip); /* dotted quad or DNS A lookup */

/* ---- ICMP ---- */
int   net_ping(uint32_t ip, uint16_t seq, int timeout_ticks);   /* round trip in ticks, or -1 */

/* ---- UDP (one application socket) ---- */
int   net_udp_bind(uint16_t local_port);
int   net_udp_send(uint32_t ip, uint16_t port, const void *data, int len);
int   net_udp_recv(void *buf, int max, uint32_t *from_ip, uint16_t *from_port);  /* 0 = none */

/* ---- TCP (one connection) ---- */
enum { NET_TCP_CLOSED = 0, NET_TCP_CONNECTING = 1, NET_TCP_OPEN = 2, NET_TCP_PEER_CLOSED = 3 };
int   net_tcp_connect(uint32_t ip, uint16_t port, int timeout_ticks);  /* 0 ok */
int   net_tcp_send(const void *data, int len);   /* queues (waits for the previous segment); <0 lost */
int   net_tcp_recv(void *buf, int max);          /* bytes, 0 none, -1 connection finished */
int   net_tcp_state(void);
void  net_tcp_close(void);

#ifdef __cplusplus
}
#endif
#endif /* PLUSNET_NET_H */
