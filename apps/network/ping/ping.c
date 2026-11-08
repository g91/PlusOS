/* ping - ICMP echo: is the network alive?   ping HOST [COUNT] */
#include <plusos/plusos.h>
#include <plusnet/net.h>

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: ping HOST [COUNT]\n"); return 1; }
    int count = 4;
    if (argc > 2) { count = 0; for (const char *p = argv[2]; *p >= '0' && *p <= '9'; p++) count = count * 10 + (*p - '0'); if (count <= 0) count = 4; }
    if (net_init()) return 1;

    uint32_t ip;
    int r = net_resolve(argv[1], &ip);
    if (r) { printf("ping: %s: %s\n", argv[1], r == -2 ? "no such host" : "cannot resolve"); return 1; }
    char s[16];
    printf("PING %s (%s): 32 bytes\n", argv[1], net_ip2str(ip, s));

    int ok = 0, min = 0, max = 0, sum = 0;
    for (int i = 1; i <= count; i++) {
        int t = net_ping(ip, (uint16_t)i, 60 * 3);
        if (t < 0) { printf("seq=%d: timeout\n", i); }
        else {
            int ms = t * 1000 / 60;
            printf("reply from %s: seq=%d time=%d ms\n", s, i, ms);
            if (!ok || ms < min) min = ms;
            if (ms > max) max = ms;
            sum += ms; ok++;
        }
        if (i < count) { uint32_t t0 = plusos->ticks(); while (plusos->ticks() - t0 < 60) net_poll(); }
        if (plusos->key_poll() == 'q') break;
    }
    printf("%d sent, %d received", count, ok);
    if (ok) printf(", min/avg/max = %d/%d/%d ms", min, sum / ok, max);
    printf("\n");
    return ok ? 0 : 1;
}
