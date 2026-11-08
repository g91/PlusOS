/* irc - a small IRC client.   irc SERVER NICK [#CHANNEL] [PORT]
 *
 * Type to talk in the current channel. Commands:
 *   /join #chan   /part   /msg nick text   /nick name   /raw IRC-LINE   /quit
 * Cmd-Q also quits.
 */
#include <plusos/plusos.h>
#include <plusnet/net.h>

static char nick[32] = "plusos";
static char chan[64];
static char line[400];          /* the line being typed */
static int  llen;
static char inbuf[1024];        /* partial line from the server */
static int  inlen;

static void prompt(void)
{
    printf("%s> ", chan[0] ? chan : "");
    if (llen) plusos->con_write(line, (size_t)llen);
}

static void say(const char *fmt, const char *a, const char *b, const char *c)
{
    /* interrupt the input line, print, restore the prompt */
    if (llen || chan[0]) printf("\n");
    printf(fmt, a, b, c);
    printf("\n");
    prompt();
}

static int sendf(const char *a, const char *b, const char *c, const char *d)
{
    char out[520];
    int n = snprintf(out, sizeof out, "%s%s%s%s\r\n", a, b ? b : "", c ? c : "", d ? d : "");
    if (n >= (int)sizeof out) n = (int)sizeof out - 1;
    return net_tcp_send(out, n);
}

/* ":prefix CMD params :trailing" */
static void handle(char *msg)
{
    char *prefix = NULL, *from = NULL, *p = msg;
    if (*p == ':') { prefix = p + 1; while (*p && *p != ' ') p++; if (*p) *p++ = 0; }
    char *cmd = p; while (*p && *p != ' ') p++; if (*p) *p++ = 0;
    char *params = p, *trail = NULL;
    char *t = params;
    while (*t) {
        if (t[0] == ':' && (t == params || t[-1] == ' ')) { *t = 0; trail = t + 1; break; }
        t++;
    }
    /* first param (target) */
    char *target = params; char *q = params; while (*q && *q != ' ') q++; if (*q) *q = 0;
    if (prefix) { from = prefix; for (char *e = prefix; *e; e++) if (*e == '!') { *e = 0; break; } }

    if (!strcmp(cmd, "PING")) { sendf("PONG :", trail ? trail : target, NULL, NULL); return; }
    if (!strcmp(cmd, "PRIVMSG") || !strcmp(cmd, "NOTICE")) {
        const char *txt = trail ? trail : "";
        if (!strncmp(txt, "\001ACTION ", 8)) { say("* %s %s%s", from ? from : "?", txt + 8, ""); return; }
        if (!strcmp(target, nick)) say("*%s* %s%s", from ? from : "?", txt, "");
        else if (chan[0] && !strcmp(target, chan)) say("<%s> %s%s", from ? from : "?", txt, "");
        else say("[%s] <%s> %s", target, from ? from : "?", txt);
        return;
    }
    if (!strcmp(cmd, "JOIN"))  { say("-> %s joined %s%s", from ? from : "?", trail ? trail : target, ""); return; }
    if (!strcmp(cmd, "PART"))  { say("<- %s left %s%s", from ? from : "?", target, ""); return; }
    if (!strcmp(cmd, "QUIT"))  { say("<- %s quit (%s)%s", from ? from : "?", trail ? trail : "", ""); return; }
    if (!strcmp(cmd, "NICK"))  { say("%s is now %s%s", from ? from : "?", trail ? trail : target, ""); if (from && !strcmp(from, nick) && trail) { strncpy(nick, trail, sizeof nick - 1); } return; }
    if (!strcmp(cmd, "KICK"))  { say("%s kicked from %s%s", params, target, ""); return; }
    if (!strcmp(cmd, "001") && chan[0]) { sendf("JOIN ", chan, NULL, NULL); }
    if (cmd[0] >= '0' && cmd[0] <= '9') {       /* numerics: show the text */
        if (!strcmp(cmd, "433")) say("nick in use - try /nick other%s%s", "", "", "");
        if (trail) say("%s%s%s", trail, "", "");
        else { char *rest = params + strlen(params) + 1; say("%s %s%s", cmd, rest, ""); }
        return;
    }
    say("%s %s %s", cmd, params, trail ? trail : "");
}

static void command(char *s)
{
    if (s[0] != '/') {
        if (!chan[0]) { say("not in a channel: /join #chan%s%s", "", "", ""); return; }
        sendf("PRIVMSG ", chan, " :", s);
        say("<%s> %s%s", nick, s, "");
        return;
    }
    char *arg = s + 1; while (*arg && *arg != ' ') arg++; if (*arg) *arg++ = 0;
    const char *c = s + 1;
    if (!strcmp(c, "join") && *arg) { strncpy(chan, arg, sizeof chan - 1); sendf("JOIN ", arg, NULL, NULL); }
    else if (!strcmp(c, "part")) { if (chan[0]) sendf("PART ", chan, NULL, NULL); chan[0] = 0; }
    else if (!strcmp(c, "msg")) {
        char *to = arg; char *txt = arg; while (*txt && *txt != ' ') txt++; if (*txt) *txt++ = 0;
        sendf("PRIVMSG ", to, " :", txt);
        say("-> *%s* %s%s", to, txt, "");
    }
    else if (!strcmp(c, "nick") && *arg) { sendf("NICK ", arg, NULL, NULL); strncpy(nick, arg, sizeof nick - 1); }
    else if (!strcmp(c, "raw")) sendf(arg, NULL, NULL, NULL);
    else if (!strcmp(c, "quit")) { sendf("QUIT :PlusOS", NULL, NULL, NULL); }
    else say("commands: /join #c  /part  /msg n t  /nick n  /raw l  /quit%s%s", "", "", "");
}

int main(int argc, char **argv)
{
    if (argc < 3) { printf("usage: irc SERVER NICK [#CHANNEL] [PORT]\n"); return 1; }
    strncpy(nick, argv[2], sizeof nick - 1);
    if (argc > 3 && argv[3][0] == '#') strncpy(chan, argv[3], sizeof chan - 1);
    int port = 6667;
    int pi = (argc > 3 && argv[3][0] != '#') ? 3 : 4;
    if (argc > pi) { port = 0; for (const char *p = argv[pi]; *p >= '0' && *p <= '9'; p++) port = port * 10 + (*p - '0'); }
    if (net_init()) return 1;

    uint32_t ip;
    if (net_resolve(argv[1], &ip)) { printf("irc: cannot resolve %s\n", argv[1]); return 1; }
    char s[16];
    printf("Connecting to %s:%d...\n", net_ip2str(ip, s), port);
    if (net_tcp_connect(ip, (uint16_t)port, 60 * 15)) { printf("irc: connection failed\n"); return 1; }
    sendf("NICK ", nick, NULL, NULL);
    sendf("USER ", nick, " 0 * :PlusOS on a Macintosh", NULL);
    printf("Connected as %s. /quit or Cmd-Q leaves.\n", nick);
    prompt();

    static uint8_t buf[512];
    int quitting = 0;
    for (;;) {
        net_poll();
        int n = net_tcp_recv(buf, sizeof buf);
        if (n < 0) { printf("\n[disconnected]\n"); break; }
        for (int i = 0; i < n; i++) {
            char c = (char)buf[i];
            if (c == '\n') { inbuf[inlen] = 0; if (inlen) handle(inbuf); inlen = 0; }
            else if (c != '\r' && inlen < (int)sizeof inbuf - 1) inbuf[inlen++] = c;
        }
        if (quitting && net_tcp_state() != NET_TCP_OPEN) break;

        int ch = plusos->key_poll();
        if (ch) {
            if (ch == KEY_FKEY || ((ch == 'q' || ch == 'Q') && (plusos->key_mods() & 0x0100))) {
                sendf("QUIT :PlusOS", NULL, NULL, NULL); quitting = 1;
            } else if (ch == KEY_RETURN || ch == KEY_ENTER) {
                printf("\n");
                line[llen] = 0;
                if (llen) { int was = llen; llen = 0; char tmp[400]; memcpy(tmp, line, (size_t)was + 1); command(tmp); if (!strncmp(tmp, "/quit", 5)) quitting = 1; }
                prompt();
            } else if (ch == KEY_BACKSPACE || ch == KEY_DELETE) {
                if (llen) { llen--; plusos->con_write("\b \b", 3); }
            } else if (ch >= 32 && ch < 127 && llen < (int)sizeof line - 1) {
                line[llen++] = (char)ch; plusos->con_putc(ch);
            }
        }
        if (n == 0 && !ch) plusos->sleep_ticks(1);
    }
    net_tcp_close();
    return 0;
}
