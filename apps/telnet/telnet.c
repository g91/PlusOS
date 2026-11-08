/* telnet - a plain telnet client.   telnet HOST [PORT]
 * Cmd-Q or any function key disconnects. Server-side echo is assumed (the
 * usual case); ANSI escape sequences are stripped, since the PlusOS console
 * has no cursor addressing. */
#include <plusos/plusos.h>
#include <plusnet/net.h>

#define IAC  255
#define DONT 254
#define DO   253
#define WONT 252
#define WILL 251
#define SB   250
#define SE   240

static int ia_state, ia_cmd, ansi;

static void send_bytes(const uint8_t *b, int n) { net_tcp_send(b, n); }

static void telnet_in(int c)
{
    switch (ia_state) {
    case 1:                                  /* after IAC */
        if (c == IAC) { ia_state = 0; plusos->con_putc(c); return; }
        if (c == DO || c == DONT || c == WILL || c == WONT) { ia_cmd = c; ia_state = 2; return; }
        if (c == SB) { ia_state = 3; return; }
        ia_state = 0;                        /* NOP, GA, ... */
        return;
    case 2: {                                /* option negotiation */
        uint8_t r[3] = { IAC, 0, (uint8_t)c };
        if (ia_cmd == DO)        r[1] = (c == 3) ? WILL : WONT;   /* we only do suppress-go-ahead */
        else if (ia_cmd == WILL) r[1] = (c == 1 || c == 3) ? DO : DONT;  /* let the server echo */
        else { ia_state = 0; return; }       /* DONT / WONT: nothing to say */
        send_bytes(r, 3);
        ia_state = 0;
        return;
    }
    case 3: if (c == IAC) ia_state = 4; return;          /* inside SB ... */
    case 4: ia_state = (c == SE) ? 0 : 3; return;        /* ... until IAC SE */
    }
    if (c == IAC) { ia_state = 1; return; }
    if (ansi) {                              /* strip ESC [ ... final */
        if (ansi == 1) { ansi = (c == '[') ? 2 : 0; return; }
        if (c >= 0x40 && c <= 0x7E) ansi = 0;
        return;
    }
    if (c == 0x1B) { ansi = 1; return; }
    if (c == '\r' || c == 0 || c == 7) return;
    if (c == '\n' || c == '\t' || c == 8 || (c >= 32 && c < 127)) plusos->con_putc(c);
}

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: telnet HOST [PORT]\n"); return 1; }
    int port = 23;
    if (argc > 2) { port = 0; for (const char *p = argv[2]; *p >= '0' && *p <= '9'; p++) port = port * 10 + (*p - '0'); }
    if (net_init()) return 1;

    uint32_t ip;
    if (net_resolve(argv[1], &ip)) { printf("telnet: cannot resolve %s\n", argv[1]); return 1; }
    char s[16];
    printf("Connecting to %s:%d...\n", net_ip2str(ip, s), port);
    if (net_tcp_connect(ip, (uint16_t)port, 60 * 15)) { printf("telnet: connection failed\n"); return 1; }
    printf("Connected. Cmd-Q or an F-key disconnects.\n");

    static uint8_t buf[512];
    for (;;) {
        net_poll();
        int n = net_tcp_recv(buf, sizeof buf);
        if (n < 0) { printf("\n[connection closed]\n"); break; }
        for (int i = 0; i < n; i++) telnet_in(buf[i]);

        int ch = plusos->key_poll();
        if (ch) {
            if (ch == KEY_FKEY || ((ch == 'q' || ch == 'Q') && (plusos->key_mods() & 0x0100))) break;
            uint8_t o[4]; int on = 0;
            switch (ch) {
            case KEY_RETURN: case KEY_ENTER: o[on++] = '\r'; o[on++] = '\n'; break;
            case KEY_BACKSPACE: o[on++] = 8; break;
            case KEY_DELETE: o[on++] = 127; break;
            case KEY_TAB: o[on++] = 9; break;
            case KEY_ESC: o[on++] = 27; break;
            case KEY_UP:    o[on++] = 27; o[on++] = '['; o[on++] = 'A'; break;
            case KEY_DOWN:  o[on++] = 27; o[on++] = '['; o[on++] = 'B'; break;
            case KEY_RIGHT: o[on++] = 27; o[on++] = '['; o[on++] = 'C'; break;
            case KEY_LEFT:  o[on++] = 27; o[on++] = '['; o[on++] = 'D'; break;
            default: if (ch >= 32 && ch < 127) o[on++] = (uint8_t)ch; break;
            }
            if (on && net_tcp_send(o, on) < 0) { printf("\n[send failed]\n"); break; }
        }
        if (n == 0 && !ch) plusos->sleep_ticks(1);
    }
    net_tcp_close();
    return 0;
}
