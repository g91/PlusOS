/* net - network status and configuration (DaynaPORT over BlueSCSI WiFi).
 *
 *   net                         bring the adapter up, show MAC, IP, gateway, DNS
 *   net dhcp                    use DHCP from now on (and configure now)
 *   net ip A.B.C.D [MASK [GW [DNS]]]   static configuration, saved to the registry
 *   net id N                    SCSI ID of the DaynaPORT (BlueSCSI: the NEx.hda file)
 *   net stats                   frame counters
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusnet/net.h>
#include <plusnet/dayna.h>

static int reg_set(const char *key, const char *value)
{
    struct plusos_reg_args a;
    memset(&a, 0, sizeof a);
    a.key = key; a.value = value;
    return plusos_syscall(SYS_regset, &a) == 0 ? 0 : -1;
}

static void show(void)
{
    char s[16];
    const uint8_t *m = net_mac();
    printf("MAC      %02x:%02x:%02x:%02x:%02x:%02x\n", m[0], m[1], m[2], m[3], m[4], m[5]);
    printf("IP       %s\n", net_ip2str(net_ip(), s));
    printf("mask     %s\n", net_ip2str(net_mask(), s));
    printf("gateway  %s\n", net_ip2str(net_gw(), s));
    printf("DNS      %s\n", net_ip2str(net_dns(), s));
}

static void stats(void)
{
    const struct net_stats *t = net_get_stats();
    printf("frames in %lu  out %lu  rx errors %lu  tx errors %lu\n",
           (unsigned long)t->rx_frames, (unsigned long)t->tx_frames,
           (unsigned long)t->rx_errors, (unsigned long)t->tx_errors);
    printf("ARP requests %lu  TCP retransmits %lu\n",
           (unsigned long)t->arp_requests, (unsigned long)t->tcp_retransmits);
    if (dayna_last_error())
        printf("last SCSI error: %d at '%s'\n", dayna_last_error(), dayna_last_step());
}

int main(int argc, char **argv)
{
    if (argc >= 2 && !strcmp(argv[1], "ip")) {
        if (argc < 3) { printf("usage: net ip A.B.C.D [MASK [GW [DNS]]]\n"); return 1; }
        uint32_t t;
        if (net_str2ip(argv[2], &t)) { printf("bad address: %s\n", argv[2]); return 1; }
        reg_set("net.ip", argv[2]);
        reg_set("net.mask", argc > 3 ? argv[3] : "");
        reg_set("net.gw",   argc > 4 ? argv[4] : "");
        reg_set("net.dns",  argc > 5 ? argv[5] : "");
        reg_set("net.dhcp", "0");
        printf("saved. static IP %s\n", argv[2]);
        return 0;
    }
    if (argc >= 2 && !strcmp(argv[1], "id")) {
        if (argc < 3 || argv[2][0] < '0' || argv[2][0] > '7' || argv[2][1]) { printf("usage: net id 0-7\n"); return 1; }
        reg_set("net.scsi", argv[2]);
        printf("DaynaPORT SCSI ID set to %s\n", argv[2]);
        return 0;
    }
    if (argc >= 2 && !strcmp(argv[1], "dhcp")) {
        reg_set("net.dhcp", "1");
        printf("DHCP enabled.\n");
    }
    int r = net_init();
    if (r == -1) {
        printf("No DaynaPORT. On BlueSCSI: WiFi in bluescsi.ini + an empty NE4.hda (SCSI ID 4);\n"
               "if it uses another ID: net id N.  Not available in the emulator.\n");
        return 1;
    }
    if (r == 0) show();
    if (argc >= 2 && !strcmp(argv[1], "stats")) stats();
    return r == 0 ? 0 : 1;
}
