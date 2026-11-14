/* ikanet - the IKANET chat client for PlusOS.
 *
 * A native 1-bit Macintosh application that chats over PlusNET (TCP/IP) with
 * the IKANET server, drawn to match the IKANET design:
 *
 *   +-------------------------------------------------------------+
 *   | * File Edit Channel Font DarkMode                       [] |  menu bar
 *   +-------------------------------------------------------------+
 *   | [ic] user  (Online)  o Appearing As Offline     Nice Ping! |  status row
 *   +-------------------------------------------------------------+
 *   | [ic]|user| message text .....                            ^  |
 *   | [ DoodleBop was timed out for 24 hours ]                 #  |  messages
 *   | [ic]|user| ...                                           v  |
 *   +-------------------------------------------------------------+
 *   | [ic]| type here...                        | DL | UP | SEND  |  composer
 *   +-------------------------------------------------------------+
 *   | LONG LIVE THE APPLE III      SWIMMING, SWIMMING   IKANET    |  status strip
 *   +-------------------------------------------------------------+
 *
 * Two display modes
 *   Shell mode    launched from the text shell (no desktop running): takes over
 *                 the whole screen. The layout is sized to the real display
 *                 (512x342 Plus, 640x480 LC III, ...) and anything larger than
 *                 1024x768 is centred. A software mouse pointer is drawn.
 *   Window mode   launched from the desktop: a normal desktop window you can
 *                 drag around. The [] box at the right of the menu bar (or
 *                 Cmd-M) maximizes / restores it.
 *   Force either with -f (full screen) or -w (window).
 *
 * Transport: plain WebSocket (RFC 6455) to ws://HOST/ws - the SAME endpoint the
 * browser client uses, so it rides one port behind Cloudflare (TLS terminates at
 * the edge, never on the 68000). JSON messages, matching the server hub:
 *   -> {"t":"auth","mode":"login"|"join","user":..,"pass":..[,"icon":..]}
 *   -> {"t":"subscribe","channel_id":N,"split":1,"limit":40}
 *   -> {"t":"say","channel_id":N,"body":".."}   {"t":"ping","n":T}
 *   <- {"t":"welcome",...}  {"t":"history_begin"} {"t":"msg",...}* {"t":"history_end"}
 *   <- {"t":"msg","channel_id":N,"user":"..","icon":"..","body":".."}
 * History is requested "split" (one small frame per message) because a 68000
 * cannot buffer a 25 KB history frame; oversized frames are skipped, never
 * allowed to stall the connection.
 *
 * Usage:  ikanet [-f|-w] [HOST] [PORT] [PATH]   (defaults: ikanet.d0a.net 80 /ws)
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusos/syscall.h>
#include <plusrom/QuickDraw.h>
#include <plusnet/net.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ======================================================================== *
 *  Geometry - everything is laid out against the canvas size UIW x UIH, so
 *  the same code fills a 512x342 Plus, a 640x480 LC III, or a big window.
 * ======================================================================== */
#define MIN_W    480
#define MIN_H    320
#define MAX_W    1024                    /* larger screens: centred, not stretched */
#define MAX_H    768

static int UIW = MIN_W, UIH = MIN_H;     /* current canvas size */
static int g_lineh = 11;                 /* message line height (Compact font = 9) */

#define WINW     UIW
#define WINH     UIH
#define MENU_B   16                      /* menu bar [0,16)                     */
#define TITLE_T  MENU_B
#define TITLE_B  (MENU_B + 18)           /* status row                          */
#define MSG_T    TITLE_B
#define COMP_H   18
#define STRIP_H  14
#define COMP_T   (WINH - STRIP_H - COMP_H)
#define MSG_B    COMP_T
#define STRIP_T  (WINH - STRIP_H)
#define SBW      15
#define SBX      (WINW - SBW)            /* message scrollbar x                 */
#define LINEH    g_lineh
#define VISLINES ((MSG_B - MSG_T - 4) / LINEH)
#define SEND_W   40
#define TOOL_W   22
#define SEND_X   (WINW - SEND_W)
#define UP_X     (SEND_X - TOOL_W)
#define DL_X     (UP_X - TOOL_W)
#define COMP_ICON 18
#define ZOOM_L   (WINW - 16)             /* window-mode zoom box in the menu bar */

/* the login screen and icon editor are a 480x320 design centred on the canvas */
#define LX       ((UIW - MIN_W) / 2)
#define LY       ((UIH - MIN_H) / 2)

/* ======================================================================== *
 *  State
 * ======================================================================== */
static GWindow  win;
static GSurface canvas;                  /* everything is drawn here            */
static int      canvas_is_bb;            /* canvas == the screen back buffer    */
static int      g_fullscreen;            /* shell mode                          */
static int      g_maximized;             /* window mode: zoomed to screen size  */
static int      origin_x, origin_y;      /* canvas origin on screen (mouse map) */
static int      g_dirty = 1;             /* the frame needs redrawing           */

#define NMSG     200
#define MSGLEN   200
#define ICONBITS 256                     /* 16x16 mono, chars '0'/'1'           */
typedef struct { char from[24]; char text[MSGLEN]; char sys; char icon[ICONBITS+1]; } Msg;
static Msg  msgs[NMSG];
static int  nmsg;
static int  scroll;                      /* index of first visible message      */
static int  stick_bottom = 1;            /* follow new messages                 */

static char g_user[24];
static char g_pass[24];                  /* kept for automatic reconnects       */
static char g_icon[ICONBITS+1];          /* the logged-in user's own 16x16 icon */
static char g_status[64];                /* connection status on the login screen */
static char g_chan[40] = "#general";
static int  g_chan_id = -1;
static char g_quote[48] = "LONG LIVE THE APPLE III";
static char g_activity[32];
static uint32_t g_activity_until;
static int  g_appear_off;                /* "Appearing As Offline"              */
static int  g_dark = 1;                  /* DarkMode is the default             */
static int  g_font;                      /* 0 Chicago 1 Times 2 Arial 3 Compact */
static int  g_authed;                    /* logged in at least once             */
static int  g_ping = -1;
static char g_clip[200];                 /* Edit > Copy / Paste                 */

static char input[200];
static int  ilen;

/* ======================================================================== *
 *  Tiny helpers
 * ======================================================================== */
static int  in_rect(int x,int y,int l,int t,int r,int b){ return x>=l&&x<r&&y>=t&&y<b; }
static int  raw_mx(void){ return *(volatile int16_t*)0x832; }
static int  raw_my(void){ return *(volatile int16_t*)0x830; }
static int  mouse_x(void){ return raw_mx() - origin_x; }
static int  mouse_y(void){ return raw_my() - origin_y; }
static int  mouse_down(void){ return (*(volatile uint8_t*)0x172 & 0x80) == 0; }
static int  cmd_down(void){ return (plusos->key_mods() & 0x0100) != 0; }
static void scopy(char*d,const char*s,int n){ int i=0; for(;s[i]&&i<n-1;i++) d[i]=s[i]; d[i]=0; }
static uint32_t now(void){ return plusos->ticks(); }
static void set_activity(const char *s, int ticks)
{
    scopy(g_activity, s, sizeof g_activity);
    g_activity_until = s[0] ? now() + (uint32_t)ticks : 0;
    g_dirty = 1;
}

/* A desktop window is only visible while the desktop compositor runs. */
static int under_desktop(void)
{
    struct plusos_procinfo p[16];
    struct plusos_proclist_args a = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    for (int i = 0; i < n; i++)
        if (p[i].state != 4 && !strncmp(p[i].name, "desktop", 7)) return 1;
    return 0;
}

/* ======================================================================== *
 *  Display: canvas, present, software cursor, events
 * ======================================================================== */
static GSurface cur_data, cur_mask;
static int      cur_ready;

static void cursor_init(void)
{
    QDGlobals *q = (QDGlobals *)plusos->qd;
    if (!q) return;
    gs_wrap(&cur_data, q->arrow.data, 2, 16, 16);
    gs_wrap(&cur_mask, q->arrow.mask, 2, 16, 16);
    cur_ready = 1;
}

/* Shell mode: size the canvas to the real display; centre it if the display is
 * bigger than MAX_W x MAX_H. Returns 0 on success. */
static int setup_fullscreen(void)
{
    GSurface *bb = gfx_backbuffer();
    int sw = bb->w, sh = bb->h;
    UIW = sw > MAX_W ? MAX_W : sw;
    UIH = sh > MAX_H ? MAX_H : sh;
    if (UIW < MIN_W) UIW = MIN_W;
    if (UIH < MIN_H) UIH = MIN_H;
    UIW &= ~7;                               /* byte-aligned blits */
    if (UIW >= sw && UIH >= sh) {
        canvas = *bb;                        /* draw straight into the back buffer */
        canvas_is_bb = 1;
        origin_x = origin_y = 0;
    } else {
        if (gs_init(&canvas, UIW, UIH)) return -1;
        canvas_is_bb = 0;
        origin_x = ((sw - UIW) / 2) & ~7;
        origin_y = (sh - UIH) / 2;
    }
    plusos->con_clear();
    cursor_init();
    return 0;
}

/* Window mode: open (or re-open, when zooming) the desktop window. */
static int open_window(int w, int h)
{
    char title[48];
    snprintf(title, sizeof title, "IKANET - %s", g_chan);
    if (gwin_open(&win, title, w, h)) return -1;
    UIW = w; UIH = h;
    canvas = win.surf;
    canvas_is_bb = 0;
    g_dirty = 1;
    return 0;
}

static void toggle_maximize(void)
{
    if (g_fullscreen) return;
    GSurface *bb = gfx_backbuffer();
    int nw, nh;
    if (!g_maximized) {
        nw = (bb->w - 24) & ~7;              /* leave room for the desktop frame,  */
        nh = bb->h - 76;                     /* menu bar and taskbar               */
        if (nw > MAX_W) nw = MAX_W;
        if (nh > MAX_H) nh = MAX_H;
    } else { nw = MIN_W; nh = MIN_H; }
    if (nw < MIN_W) nw = MIN_W;
    if (nh < MIN_H) nh = MIN_H;
    int ow = UIW, oh = UIH;
    gwin_close(&win);
    if (open_window(nw, nh)) { open_window(ow, oh); return; }
    g_maximized = !g_maximized;
}

static void ui_target(void){ gfx_target(&canvas); gfx_noclip(); }

/* Show the finished frame. Shell mode composites the canvas onto the screen and
 * draws the mouse pointer on top (then restores what was under it). */
static void ui_present(void)
{
    if (!g_fullscreen) { gwin_update(&win); return; }

    GSurface *bb = gfx_backbuffer();
    if (!canvas_is_bb) {
        gfx_target(bb); gfx_noclip();
        gfx_fill(0, 0, bb->w, bb->h, g_dark ? 1 : 0);          /* border colour */
        gfx_blit(&canvas, 0, 0, UIW, UIH, origin_x, origin_y, GFX_COPY);
    }
    QDGlobals *q = (QDGlobals *)plusos->qd;
    static uint8_t save[16][3];
    int cx = -100, cy = -100, x0 = 0, rb = bb->rowbytes;
    if (cur_ready && q) {
        cx = raw_mx() - q->arrow.hotSpot.h;
        cy = raw_my() - q->arrow.hotSpot.v;
        x0 = cx >> 3;
        for (int r = 0; r < 16; r++) { int y = cy + r; for (int c = 0; c < 3; c++)
            save[r][c] = (y>=0 && y<bb->h && x0+c>=0 && x0+c<rb) ? bb->bits[y*rb + x0+c] : 0; }
        gfx_target(bb); gfx_noclip();
        gfx_blit_masked(&cur_data, &cur_mask, 0, 0, 16, 16, cx, cy);
    }
    gfx_present();
    if (cur_ready && q) {
        for (int r = 0; r < 16; r++) { int y = cy + r; for (int c = 0; c < 3; c++)
            if (y>=0 && y<bb->h && x0+c>=0 && x0+c<rb) bb->bits[y*rb + x0+c] = save[r][c]; }
    }
}

/* Unified events. 1 = event, 0 = none, -1 = window closed.
 * Window mode: the desktop delivers content coordinates; we also learn where
 * the window sits on screen so drag-tracking loops can map the raw mouse.
 * Shell mode: synthesized from key_poll + the mouse low-memory globals. */
static int fs_prev_down;
static int fs_last_mx = -1, fs_last_my = -1;

static void track_cursor(void)          /* shell mode: move the pointer */
{
    if (!g_fullscreen) return;
    int mx = raw_mx(), my = raw_my();
    if (mx != fs_last_mx || my != fs_last_my) { fs_last_mx = mx; fs_last_my = my; ui_present(); }
}

static int ui_event(GEvent *ev, int wait)
{
    if (!g_fullscreen) {
        int r = gwin_event(&win, ev, wait);
        if (r > 0 && ev->type == GEV_MOUSEDOWN) { origin_x = raw_mx() - ev->x; origin_y = raw_my() - ev->y; }
        return r;
    }
    for (;;) {
        int down = mouse_down();
        if (down && !fs_prev_down) {
            fs_prev_down = 1;
            ev->type = GEV_MOUSEDOWN; ev->x = mouse_x(); ev->y = mouse_y(); ev->key = 0;
            return 1;
        }
        if (!down) fs_prev_down = 0;
        int k = plusos->key_poll();
        if (k) { ev->type = GEV_KEY; ev->key = k; ev->x = ev->y = 0; return 1; }
        if (!wait) return 0;
        track_cursor();
        net_poll();
        plusos->sleep_ticks(1);
    }
}

/* ======================================================================== *
 *  WebSocket client (RFC 6455) over PlusNET TCP
 * ======================================================================== */
static char ws_host[64] = "ikanet.d0a.net";
static char ws_path[64] = "/ws";
static int  ws_port = 80;
static int  ws_connected;

#define WS_RXCAP 8192
static uint8_t  ws_rx[WS_RXCAP];         /* raw bytes not yet framed            */
static int      ws_rxlen;
static uint32_t ws_skip;                 /* bytes of an oversized frame to drop */
static char     ws_msg[WS_RXCAP];        /* one decoded text message            */

static uint32_t ws_rand(void)
{
    static uint32_t s;
    if (!s) s = now() ^ 0xA5A5u;
    s = s * 1664525u + 1013904223u;
    return s;
}

/* Upgrade handshake. 0 ok, else the HTTP status (or -1) for the status line. */
static int ws_handshake(void)
{
    char key[32], req[400];
    snprintf(key, sizeof key, "pLuS%08lx%08lxAA==", (unsigned long)ws_rand(), (unsigned long)ws_rand());
    int n = snprintf(req, sizeof req,
        "GET %s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\nUser-Agent: IKANET-PlusOS\r\n\r\n",
        ws_path, ws_host, key);
    if (net_tcp_send(req, n) < 0) return -1;

    /* Stream-parse the response: keep only the status line, and watch for the
     * blank line that ends the headers with a 4-byte rolling window. Header
     * size no longer matters (Cloudflare's 101 is ~630 bytes and growing). */
    char first[40]; int fl = 0, in_first = 1;
    uint32_t tail = 0;                       /* last 4 bytes seen */
    ws_rxlen = 0; ws_skip = 0;
    uint32_t end = now() + 600;              /* ~10 s */
    while (now() < end) {
        net_poll();
        uint8_t b[256];
        int got = net_tcp_recv(b, sizeof b);
        if (got < 0) return -1;
        for (int i = 0; i < got; i++) {
            uint8_t c = b[i];
            if (in_first) {
                if (c == '\r' || c == '\n') in_first = 0;
                else if (fl < (int)sizeof first - 1) first[fl++] = (char)c;
            }
            tail = (tail << 8) | c;
            if (tail == 0x0D0A0D0Au) {           /* "\r\n\r\n": end of headers */
                first[fl] = 0;
                int status = fl > 9 ? atoi(first + 9) : -1;     /* "HTTP/1.1 101 ..." */
                if (status != 101) return status > 0 ? status : -1;
                for (i++; i < got && ws_rxlen < WS_RXCAP; i++) ws_rx[ws_rxlen++] = b[i];
                return 0;
            }
        }
        if (!got) plusos->sleep_ticks(1);
    }
    return -1;
}

/* One masked text frame, sent as a single TCP segment where possible. */
static int ws_send_text(const char *s)
{
    static uint8_t f[1200];
    int len = (int)strlen(s), hn = 0;
    uint32_t mk = ws_rand();
    uint8_t mask[4] = { (uint8_t)mk, (uint8_t)(mk >> 8), (uint8_t)(mk >> 16), (uint8_t)(mk >> 24) };
    f[hn++] = 0x81;
    if (len < 126) f[hn++] = (uint8_t)(0x80 | len);
    else { f[hn++] = 0x80 | 126; f[hn++] = (uint8_t)(len >> 8); f[hn++] = (uint8_t)len; }
    for (int i = 0; i < 4; i++) f[hn++] = mask[i];
    int off = 0;
    while (off < len || hn) {
        int room = (int)sizeof f - hn, c = len - off;
        if (c > room) c = room;
        for (int i = 0; i < c; i++) f[hn + i] = (uint8_t)s[off + i] ^ mask[(off + i) & 3];
        if (net_tcp_send(f, hn + c) < 0) { ws_connected = 0; return -1; }
        off += c; hn = 0;
        if (off >= len) break;
    }
    return 0;
}

/* Read what TCP has, decode ONE complete text frame into ws_msg.
 * 1 = message ready, 0 = nothing yet, -1 = connection gone. */
static int ws_poll_message(void)
{
    net_poll();
    uint8_t b[512];
    int got = net_tcp_recv(b, sizeof b);
    if (got < 0) return -1;
    int i = 0;
    while (ws_skip && i < got) { ws_skip--; i++; }          /* finish dropping a huge frame */
    for (; i < got && ws_rxlen < WS_RXCAP; i++) ws_rx[ws_rxlen++] = b[i];

    for (;;) {
        if (ws_rxlen < 2) return 0;
        uint8_t op = ws_rx[0] & 0x0F;
        int masked = ws_rx[1] & 0x80;
        uint32_t plen = ws_rx[1] & 0x7F;
        int hsz = 2;
        if (plen == 126) { if (ws_rxlen < 4) return 0; plen = ((uint32_t)ws_rx[2] << 8) | ws_rx[3]; hsz = 4; }
        else if (plen == 127) { if (ws_rxlen < 10) return 0;
            plen = ((uint32_t)ws_rx[6] << 24) | ((uint32_t)ws_rx[7] << 16) | ((uint32_t)ws_rx[8] << 8) | ws_rx[9]; hsz = 10; }
        int moff = hsz; if (masked) hsz += 4;
        uint32_t total = (uint32_t)hsz + plen;

        if (total > WS_RXCAP) {                              /* can never fit: skip it */
            ws_skip = total - (uint32_t)ws_rxlen;
            ws_rxlen = 0;
            return 0;
        }
        if ((uint32_t)ws_rxlen < total) return 0;            /* wait for the rest */

        uint8_t *pay = ws_rx + hsz;
        if (masked) { uint8_t *mk = ws_rx + moff; for (uint32_t k = 0; k < plen; k++) pay[k] ^= mk[k & 3]; }

        int ready = 0;
        if (op == 0x8) return -1;                            /* close */
        if (op == 0x9) {                                     /* ping -> pong */
            uint8_t pong[2 + 125]; int pn = 0;
            pong[pn++] = 0x8A; pong[pn++] = 0x80 | (uint8_t)(plen > 125 ? 125 : plen);
            /* client frames must be masked: use a zero mask */
            uint8_t z[4] = {0,0,0,0};
            uint8_t fr[2 + 4 + 125]; int fn = 0;
            fr[fn++] = pong[0]; fr[fn++] = pong[1];
            for (int k = 0; k < 4; k++) fr[fn++] = z[k];
            for (uint32_t k = 0; k < plen && k < 125; k++) fr[fn++] = pay[k];
            net_tcp_send(fr, fn);
        } else if (op == 0x1 || op == 0x0) {
            memcpy(ws_msg, pay, plen); ws_msg[plen] = 0; ready = 1;
        }
        memmove(ws_rx, ws_rx + total, (size_t)(ws_rxlen - (int)total));
        ws_rxlen -= (int)total;
        if (ready) return 1;
    }
}

/* ---- minimal JSON readers ----------------------------------------------- */
static const char *json_val(const char *s, const char *key)
{
    char pat[48]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == ':') p++;
    return p;
}
static int json_str(const char *s, const char *key, char *out, int max)
{
    const char *p = json_val(s, key);
    if (!p || *p != '"') { if (max) out[0] = 0; return -1; }
    p++;
    int o = 0;
    while (*p && *p != '"' && o < max - 1) {
        if (*p == '\\' && p[1]) {
            p++;
            if (*p == 'n') out[o++] = ' ';
            else if (*p == 'u') { p += 4; out[o++] = '?'; }    /* non-ASCII: placeholder */
            else out[o++] = *p;
        } else out[o++] = *p;
        p++;
    }
    out[o] = 0; return 0;
}
static int json_int(const char *s, const char *key)
{
    const char *p = json_val(s, key); if (!p) return -1;
    int neg = 0; if (*p == '-') { neg = 1; p++; }
    if (*p < '0' || *p > '9') return -1;
    int v = 0; while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
    return neg ? -v : v;
}
static int json_true(const char *s, const char *key)
{
    const char *p = json_val(s, key);
    return p && !strncmp(p, "true", 4);
}
static void json_esc(const char *s, char *out, int max)
{
    int o = 0;
    for (; *s && o < max - 2; s++) {
        if (*s == '"' || *s == '\\') { out[o++] = '\\'; out[o++] = *s; }
        else if ((unsigned char)*s >= 32 && (unsigned char)*s < 127) out[o++] = *s;
    }
    out[o] = 0;
}

/* ======================================================================== *
 *  Icons and the message log
 * ======================================================================== */
static void default_icon(char *out)
{
    static const char *art =
        "0000011110000000" "0000111111000000" "0001111111100000" "0011011101110000"
        "0011011101110000" "0011111111110000" "0001111111100000" "0000111111000000"
        "0001010110100000" "0010101011010000" "0010101010110000" "0101010101010000"
        "0100101010010000" "0000100101000000" "0000000000000000" "0000000000000000";
    scopy(out, art, ICONBITS + 1);
}

/* 16x16 icon scaled into an sz x sz box at (l,t). */
static void draw_icon(const char *bits, int l, int t, int sz)
{
    gfx_fill(l, t, l + sz, t + sz, 0);
    if (bits && (int)strlen(bits) >= ICONBITS) {
        for (int gy = 0; gy < sz; gy++) {
            int sy = gy * 16 / sz;
            for (int gx = 0; gx < sz; gx++)
                if (bits[sy * 16 + gx * 16 / sz] == '1') gfx_pixel(l + gx, t + gy, 1);
        }
    }
    gfx_frame(l, t, l + sz, t + sz);
}

static void scroll_to_end(void)
{
    scroll = nmsg > VISLINES ? nmsg - VISLINES : 0;
}

static void push_msg(const char *from, const char *text, int sys, const char *icon)
{
    if (nmsg >= NMSG) { memmove(msgs, msgs + 1, sizeof(Msg) * (NMSG - 1)); nmsg = NMSG - 1; if (scroll) scroll--; }
    Msg *m = &msgs[nmsg++];
    scopy(m->from, from ? from : "", sizeof m->from);
    scopy(m->text, text ? text : "", sizeof m->text);
    m->sys = (char)sys;
    if (icon) scopy(m->icon, icon, ICONBITS + 1); else m->icon[0] = 0;
    if (stick_bottom) scroll_to_end();
    g_dirty = 1;
}
static void sys_msg(const char *text){ push_msg("", text, 1, 0); }

/* one message object from a "msg" frame */
static void ingest_msg_obj(const char *obj)
{
    char user[24], body[MSGLEN], icon[ICONBITS + 1];
    json_str(obj, "user", user, sizeof user);
    json_str(obj, "body", body, sizeof body);
    json_str(obj, "icon", icon, sizeof icon);
    if (!body[0]) {                                          /* attachment only */
        const char *f = json_val(obj, "file");
        char name[64];
        if (f && *f == '{' && !json_str(f, "name", name, sizeof name)) {
            const char *img = json_true(f, "is_image") ? "image" : "file";
            snprintf(body, sizeof body, "[%s: %s]", img, name);
        }
    }
    int is_sys = json_true(obj, "system") || !user[0];
    push_msg(is_sys ? "" : user, body, is_sys, (!is_sys && icon[0]) ? icon : 0);
}

/* ======================================================================== *
 *  Protocol
 * ======================================================================== */
static int ws_subscribe(int cid)
{
    char b[96];
    snprintf(b, sizeof b, "{\"t\":\"subscribe\",\"channel_id\":%d,\"split\":1,\"limit\":40}", cid);
    return ws_send_text(b);
}
static int ws_say(const char *text)
{
    char esc[420], b[520];
    json_esc(text, esc, sizeof esc);
    snprintf(b, sizeof b, "{\"t\":\"say\",\"channel_id\":%d,\"body\":\"%s\"}", g_chan_id, esc);
    return ws_send_text(b);
}
static int ws_add_channel(const char *name)
{
    char esc[64], b[120];
    json_esc(name, esc, sizeof esc);
    snprintf(b, sizeof b, "{\"t\":\"add_channel\",\"name\":\"%s\",\"split\":1}", esc);
    return ws_send_text(b);
}
static int ws_set_icon(const char *bits)
{
    char b[300];
    snprintf(b, sizeof b, "{\"t\":\"set_icon\",\"icon\":\"%s\"}", bits);
    return ws_send_text(b);
}

static uint32_t g_ping_sent_at;
static int      g_ping_outstanding;
static uint32_t g_last_ping;

static void set_channel(int id, const char *name)
{
    g_chan_id = id; nmsg = 0; scroll = 0; stick_bottom = 1;
    if (name && name[0]) scopy(g_chan, name, sizeof g_chan);
    if (!g_fullscreen) {
        char t[48]; snprintf(t, sizeof t, "IKANET - %s", g_chan);
        gwin_title(&win, t);
    }
    ws_subscribe(id);
    g_dirty = 1;
}

static void handle_json(const char *s)
{
    char t[24];
    if (json_str(s, "t", t, sizeof t)) return;

    if (!strcmp(t, "welcome")) {
        json_str(s, "quote", g_quote, sizeof g_quote);
        const char *u = json_val(s, "user");
        if (u && *u == '{') { char ic[ICONBITS + 1]; if (!json_str(u, "icon", ic, sizeof ic) && ic[0]) scopy(g_icon, ic, sizeof g_icon); }
        const char *ch = json_val(s, "channels");
        if (ch) {
            char nm[40] = "#general";
            json_str(ch, "name", nm, sizeof nm);
            int id = json_int(ch, "id");
            if (id > 0) set_channel(id, nm);
        }
        g_authed = 1;
    }
    else if (!strcmp(t, "history_begin")) {
        if (json_int(s, "channel_id") == g_chan_id) { nmsg = 0; scroll = 0; stick_bottom = 1; g_dirty = 1; }
    }
    else if (!strcmp(t, "history_end")) {
        scroll_to_end(); g_dirty = 1;
    }
    else if (!strcmp(t, "history")) {                    /* legacy single-frame history */
        if (json_int(s, "channel_id") == g_chan_id) {
            nmsg = 0;
            const char *p = json_val(s, "messages");
            if (p && *p == '[') {
                p++;
                while (*p && *p != ']') {
                    if (*p == '{') {
                        const char *e = p; int depth = 0, inq = 0;
                        for (; *e; e++) {
                            if (*e == '\\' && inq) { e++; continue; }
                            if (*e == '"') inq = !inq;
                            else if (!inq && *e == '{') depth++;
                            else if (!inq && *e == '}') { if (!--depth) { e++; break; } }
                        }
                        static char obj[900]; int c = (int)(e - p); if (c > (int)sizeof obj - 1) c = sizeof obj - 1;
                        memcpy(obj, p, (size_t)c); obj[c] = 0; ingest_msg_obj(obj); p = e;
                    } else p++;
                }
            }
        }
    }
    else if (!strcmp(t, "msg")) {
        if (json_int(s, "channel_id") == g_chan_id) ingest_msg_obj(s);
    }
    else if (!strcmp(t, "pong")) {
        if (g_ping_outstanding) {
            g_ping = (int)((now() - g_ping_sent_at) * 50 / 3);   /* ticks -> ms */
            g_ping_outstanding = 0; g_dirty = 1;
        }
    }
    else if (!strcmp(t, "error")) {
        char m[120]; json_str(s, "msg", m, sizeof m); sys_msg(m[0] ? m : "error");
    }
    else if (!strcmp(t, "channel_added") || !strcmp(t, "dm_opened")) {
        const char *c = json_val(s, "channel");
        if (c && *c == '{') {
            char nm[40] = ""; json_str(c, "name", nm, sizeof nm);
            int id = json_int(c, "id");
            if (id > 0) set_channel(id, nm);
        }
    }
    else if (!strcmp(t, "icon_set")) {
        json_str(s, "icon", g_icon, sizeof g_icon); g_dirty = 1;
    }
}

/* Connect + handshake, reporting progress through status_cb. 0 = connected. */
static void (*g_status_cb)(void);
static void status(const char *s){ scopy(g_status, s, sizeof g_status); if (g_status_cb) g_status_cb(); }

static int ikanet_connect(void)
{
    ws_connected = 0;
    status("Starting network...");
    if (net_init()) { status("No network - is the DaynaPORT on?"); return -1; }
    char line[64];
    snprintf(line, sizeof line, "Looking up %s...", ws_host);
    status(line);
    uint32_t ip;
    if (net_resolve(ws_host, &ip)) { status("Can't find the IKANET server (DNS)"); return -1; }
    char ips[16];
    snprintf(line, sizeof line, "Connecting to %s...", net_ip2str(ip, ips));
    status(line);
    if (net_tcp_connect(ip, (uint16_t)ws_port, 60 * 10)) { status("Server didn't answer"); return -1; }
    status("Opening chat line...");
    int hs = ws_handshake();
    if (hs) {
        if (hs > 0) snprintf(line, sizeof line, "Server refused chat (HTTP %d)", hs);
        else        snprintf(line, sizeof line, "Chat handshake timed out");
        status(line);
        net_tcp_close();
        return -1;
    }
    ws_connected = 1;
    status("Connected.");
    return 0;
}

/* Send the auth frame and wait for welcome / error. 1 = logged in. */
static int send_auth(const char *mode, char *err, int errmax)
{
    char eu[48], ep[48], b[420];
    json_esc(g_user, eu, sizeof eu); json_esc(g_pass, ep, sizeof ep);
    if (!strcmp(mode, "join") && g_icon[0])
        snprintf(b, sizeof b, "{\"t\":\"auth\",\"mode\":\"join\",\"user\":\"%s\",\"pass\":\"%s\",\"icon\":\"%s\"}", eu, ep, g_icon);
    else
        snprintf(b, sizeof b, "{\"t\":\"auth\",\"mode\":\"%s\",\"user\":\"%s\",\"pass\":\"%s\"}", mode, eu, ep);
    if (ws_send_text(b) < 0) { scopy(err, "Lost the connection", errmax); return 0; }

    uint32_t end = now() + 600;
    while (now() < end) {
        int r = ws_poll_message();
        if (r < 0) { ws_connected = 0; scopy(err, "Disconnected", errmax); return 0; }
        if (r == 1) {
            char t[24]; json_str(ws_msg, "t", t, sizeof t);
            if (!strcmp(t, "welcome")) { handle_json(ws_msg); return 1; }
            if (!strcmp(t, "error")) { json_str(ws_msg, "msg", err, errmax); ws_connected = 0; return 0; }
        } else plusos->sleep_ticks(1);
    }
    scopy(err, "No answer from server", errmax);
    return 0;
}

/* Drain all waiting messages. Returns -1 if the connection dropped. */
static uint32_t g_reconnect_at;
static int pump_net(void)
{
    for (int n = 0; n < 16; n++) {
        int r = ws_poll_message();
        if (r < 0) {
            ws_connected = 0;
            sys_msg("Connection lost - reconnecting...");
            g_reconnect_at = now() + 300;
            return -1;
        }
        if (r == 0) break;
        handle_json(ws_msg);
    }
    return 0;
}

static void ping_tick(void)
{
    uint32_t t = now();
    if (t - g_last_ping > 600) {                         /* every ~10 s */
        char b[48]; snprintf(b, sizeof b, "{\"t\":\"ping\",\"n\":%lu}", (unsigned long)t);
        if (ws_send_text(b) >= 0) { g_ping_sent_at = t; g_ping_outstanding = 1; g_last_ping = t; }
    }
}

static void try_reconnect(void)
{
    if (ws_connected || !g_authed || now() < g_reconnect_at) return;
    g_status_cb = 0;
    char err[80];
    if (!ikanet_connect() && send_auth("login", err, sizeof err)) sys_msg("Reconnected.");
    else g_reconnect_at = now() + 600;
    g_dirty = 1;
}

/* ======================================================================== *
 *  Drawing - always drawn light (white paper, black ink); DarkMode inverts
 *  everything below the menu bar, which stays a light Mac menu bar.
 * ======================================================================== */
static void draw_button(int l, int t, int r, int b, const char *s)
{
    gfx_fill_round_rect(l + 2, t + 2, r + 2, b + 2, 5, 1);  /* drop shadow */
    gfx_fill_round_rect(l, t, r, b, 5, 0);
    gfx_round_rect(l, t, r, b, 5, 1);
    gfx_text(l + ((r - l) - gfx_text_width(s)) / 2, t + ((b - t) - 8) / 2, s, 0);
}

static const char *menu_names[] = { "File", "Edit", "Channel", "Font", "DarkMode" };
static int menu_x[6];                    /* left edge of each title; [5] = end */

static void draw_menubar(void)
{
    gfx_fill(0, 0, WINW, MENU_B, 0);
    gfx_hline(0, WINW, MENU_B - 1, 1);
    /* apple */
    gfx_fill_circle(9, 7, 4, 1); gfx_fill(9, 1, 11, 4, 1);
    int x = 22;
    for (int i = 0; i < 5; i++) {
        menu_x[i] = x;
        const char *nm = (i == 4 && g_dark) ? "LightMode" : menu_names[i];
        x = gfx_text(x + 4, 4, nm, 0) + 8;
    }
    menu_x[5] = x;
    if (!g_fullscreen) {                                     /* zoom box */
        gfx_frame(ZOOM_L, 3, ZOOM_L + 11, 13);
        if (g_maximized) gfx_frame(ZOOM_L + 3, 6, ZOOM_L + 8, 10);
        else             gfx_frame(ZOOM_L + 2, 5, ZOOM_L + 9, 11);
    }
}

static void draw_bars(int x, int y, int n)
{
    for (int i = 0; i < 4; i++) {
        int h = 3 + i * 2;
        if (i < n) gfx_fill(x + i * 4, y + 8 - h, x + i * 4 + 3, y + 8, 1);
        else       gfx_frame(x + i * 4, y + 8 - h, x + i * 4 + 3, y + 8);
    }
}

static void draw_titlebar(void)
{
    gfx_hline(0, WINW, TITLE_B - 1, 1);
    draw_icon(g_icon[0] ? g_icon : 0, 3, TITLE_T + 2, 14);
    int x = gfx_text(22, TITLE_T + 5, g_user, 0) + 8;
    /* connection status pill */
    const char *st = ws_connected ? "Online" : "Offline";
    int pw = gfx_text_width(st) + 10;
    gfx_round_rect(x, TITLE_T + 2, x + pw, TITLE_B - 3, 6, 1);
    gfx_text(x + 5, TITLE_T + 5, st, 0);
    x += pw + 10;
    /* how others see you */
    gfx_frame(x, TITLE_T + 5, x + 8, TITLE_T + 13);
    if (g_appear_off) gfx_fill(x + 2, TITLE_T + 7, x + 6, TITLE_T + 11, 1);
    gfx_text(x + 12, TITLE_T + 5, "Appearing As Offline", 0);
    /* ping meter, right aligned */
    char pbuf[32];
    const char *label = g_ping < 0 ? "Ping..." : g_ping < 80 ? "Nice Ping!" : g_ping < 200 ? "OK Ping" : "Slow Ping...";
    int bars = g_ping < 0 ? 0 : g_ping < 80 ? 4 : g_ping < 200 ? 3 : g_ping < 450 ? 2 : 1;
    if (g_ping >= 0) snprintf(pbuf, sizeof pbuf, "%s %d", label, g_ping);
    else scopy(pbuf, label, sizeof pbuf);
    int px = WINW - gfx_text_width(pbuf) - 24;
    if (px > x + gfx_text_width("Appearing As Offline") + 20) {
        draw_bars(px, TITLE_T + 5, bars);
        gfx_text(px + 18, TITLE_T + 5, pbuf, 0);
    }
}

static void draw_messages(void)
{
    gfx_fill(0, MSG_T, SBX, MSG_B, 0);
    int y = MSG_T + 2;
    for (int i = scroll, shown = 0; i < nmsg && shown < VISLINES; i++, shown++, y += LINEH) {
        Msg *m = &msgs[i];
        int maxc = (SBX - 24) / 8;
        char line[MSGLEN + 30];
        if (m->sys) {
            scopy(line, m->text, sizeof line);
            if ((int)strlen(line) > maxc) line[maxc] = 0;
            int w = gfx_text_width(line);
            gfx_frame(20, y, 24 + w, y + LINEH - 1);
            gfx_text(22, y + (LINEH - 8) / 2, line, 0);
        } else {
            draw_icon(m->icon[0] ? m->icon : 0, 4, y, LINEH - 2);
            snprintf(line, sizeof line, "|%s| %s", m->from, m->text);
            if ((int)strlen(line) > maxc) line[maxc] = 0;
            gfx_text(18, y + (LINEH - 8) / 2, line, 0);
        }
    }
    /* scrollbar with a thumb */
    gfx_frame(SBX, MSG_T, WINW, MSG_B);
    gfx_frame(SBX, MSG_T, WINW, MSG_T + SBW);
    gfx_frame(SBX, MSG_B - SBW, WINW, MSG_B);
    gfx_text(SBX + 4, MSG_T + 4, "^", 0);
    gfx_text(SBX + 4, MSG_B - SBW + 4, "v", 0);
    int tt = MSG_T + SBW, tb = MSG_B - SBW;
    gfx_gray(SBX + 1, tt, WINW - 1, tb);
    if (nmsg > VISLINES) {
        int span = tb - tt - 12;
        int ty = tt + span * scroll / (nmsg - VISLINES);
        gfx_fill(SBX + 1, ty, WINW - 1, ty + 12, 0);
        gfx_frame(SBX, ty, WINW, ty + 12);
    }
}

static void draw_composer(void)
{
    gfx_fill(0, COMP_T, WINW, COMP_T + COMP_H, 0);
    gfx_hline(0, WINW, COMP_T, 1);
    draw_icon(g_icon[0] ? g_icon : 0, 2, COMP_T + 2, COMP_H - 4);
    gfx_vline(COMP_ICON + 1, COMP_T, COMP_T + COMP_H, 1);
    char show[200]; scopy(show, input, sizeof show);
    int tx = COMP_ICON + 6, maxc = (DL_X - tx - 6) / 8;
    int len = (int)strlen(show);
    const char *vis = len > maxc ? show + (len - maxc) : show;
    int ex = gfx_text(tx, COMP_T + 5, vis, 0);
    gfx_vline(ex + 1, COMP_T + 3, COMP_T + COMP_H - 3, 1);
    gfx_vline(DL_X, COMP_T, COMP_T + COMP_H, 1);
    gfx_vline(UP_X, COMP_T, COMP_T + COMP_H, 1);
    gfx_vline(SEND_X, COMP_T, COMP_T + COMP_H, 1);
    gfx_text(DL_X + 3, COMP_T + 5, "DL", 0);
    gfx_text(UP_X + 3, COMP_T + 5, "UP", 0);
    gfx_text(SEND_X + 4, COMP_T + 5, "SEND", 0);
}

static void draw_strip(void)
{
    gfx_fill(0, STRIP_T, WINW, WINH, 0);
    gfx_hline(0, WINW, STRIP_T, 1);
    gfx_text(4, STRIP_T + 3, g_quote, 0);
    gfx_vline(gfx_text_width(g_quote) + 10, STRIP_T, WINH, 1);
    int bx = WINW - 64;
    if (g_activity[0]) gfx_text(bx - 10 - gfx_text_width(g_activity), STRIP_T + 3, g_activity, 0);
    gfx_vline(bx, STRIP_T, WINH, 1);
    gfx_text(bx + 6, STRIP_T + 3, "IKANET", 0);
}

static void redraw(void)
{
    ui_target();
    gfx_fill(0, 0, WINW, WINH, 0);
    draw_menubar();
    draw_titlebar();
    draw_messages();
    draw_composer();
    draw_strip();
    gfx_frame(0, 0, WINW, WINH);
    if (g_dark) gfx_invert(0, MENU_B, WINW, WINH);
    ui_present();
    g_dirty = 0;
}

/* ======================================================================== *
 *  Menus
 * ======================================================================== */
/* Pull-down that tracks the mouse while the button is held; returns the item
 * index or -1. Items starting with '-' are separators. */
static int popup_menu(int mx, const char **items, int n)
{
    int iw = 0;
    for (int i = 0; i < n; i++) { int w = gfx_text_width(items[i]); if (w > iw) iw = w; }
    iw += 24;
    int l = mx, t = MENU_B - 1, r = l + iw, b = t + n * 14 + 4;
    if (r > WINW) { r = WINW; l = r - iw; }
    ui_target();
    gfx_fill_round_rect(l + 2, t + 2, r + 2, b + 2, 0, 1);
    gfx_fill(l, t, r, b, 0); gfx_frame(l, t, r, b);
    for (int i = 0; i < n; i++) {
        if (items[i][0] == '-') gfx_gray(l + 1, t + 2 + i * 14 + 6, r - 1, t + 2 + i * 14 + 7);
        else gfx_text(l + 10, t + 5 + i * 14, items[i], 0);
    }
    ui_present();
    int chosen = -1, shown = -1;
    while (mouse_down()) {
        int x = mouse_x(), y = mouse_y();
        int idx = (y - t - 2) / 14;
        chosen = (x >= l && x < r && y > t && idx >= 0 && idx < n && items[idx][0] != '-') ? idx : -1;
        if (chosen != shown) {
            ui_target();
            if (shown >= 0)  gfx_invert(l + 1, t + 2 + shown * 14, r - 1, t + 16 + shown * 14);
            if (chosen >= 0) gfx_invert(l + 1, t + 2 + chosen * 14, r - 1, t + 16 + chosen * 14);
            shown = chosen;
            ui_present();
        } else track_cursor();
        net_poll();
    }
    g_dirty = 1;
    return chosen;
}

static void icon_editor(void);

static void do_logout(void){ net_tcp_close(); ws_connected = 0; g_authed = 0; }

static void menu_click(int mx)
{
    int which = -1;
    for (int i = 0; i < 5; i++) if (mx >= menu_x[i] && mx < menu_x[i + 1]) which = i;
    if (!g_fullscreen && mx >= ZOOM_L - 2) { toggle_maximize(); return; }
    if (which < 0) return;
    int bx = menu_x[which];

    if (which == 4) { g_dark = !g_dark; g_dirty = 1; return; }
    if (which == 0) {
        const char *it[] = { "Upload File", "Download File", "-", "Appear As Online",
                             "Appear As Offline", "-", "User Icon...", "-", "Log Out", "Quit" };
        int c = popup_menu(bx, it, 10);
        if (c == 0 || c == 1) sys_msg("File transfer is web-only for now - use ikanet.d0a.net");
        else if (c == 3 || c == 4) {
            g_appear_off = (c == 4);
            if (ws_connected) ws_send_text(g_appear_off ? "{\"t\":\"appear_offline\",\"value\":true}"
                                                        : "{\"t\":\"appear_offline\",\"value\":false}");
        }
        else if (c == 6) icon_editor();
        else if (c == 8) { do_logout(); sys_msg("Logged out. Restart IKANET to log in again."); }
        else if (c == 9) { do_logout(); g_authed = -1; }       /* -1 = quit request */
    } else if (which == 1) {
        const char *it[] = { "Copy   Cmd-C", "Paste  Cmd-V" };
        int c = popup_menu(bx, it, 2);
        if (c == 0) scopy(g_clip, input, sizeof g_clip);
        if (c == 1) { for (const char *p = g_clip; *p && ilen < (int)sizeof input - 1; p++) input[ilen++] = *p; input[ilen] = 0; }
    } else if (which == 2) {
        const char *it[] = { "#general", "#swimming", "#gameboy", "-", "Leave Channel" };
        int c = popup_menu(bx, it, 5);
        if (c >= 0 && c <= 2 && ws_connected) ws_add_channel(it[c]);
        else if (c == 4 && ws_connected && g_chan_id > 0) {
            char b[64]; snprintf(b, sizeof b, "{\"t\":\"leave_channel\",\"channel_id\":%d}", g_chan_id);
            ws_send_text(b); ws_add_channel("#general");
        }
    } else if (which == 3) {
        const char *it[] = { "  Chicago", "  Times New Roman", "  Arial", "  Compact Font" };
        static char lab[4][24];
        const char *show[4];
        for (int i = 0; i < 4; i++) { scopy(lab[i], it[i], sizeof lab[i]); if (i == g_font) lab[i][0] = '*'; show[i] = lab[i]; }
        int c = popup_menu(bx, show, 4);
        if (c >= 0) { g_font = c; g_lineh = (c == 3) ? 9 : 11; if (stick_bottom) scroll_to_end(); }
    }
    g_dirty = 1;
}

/* ======================================================================== *
 *  16x16 icon editor (centred, modal)
 * ======================================================================== */
static void icon_editor(void)
{
    char bits[ICONBITS + 1];
    if ((int)strlen(g_icon) >= ICONBITS) scopy(bits, g_icon, sizeof bits); else default_icon(bits);
    const int CELL = 12, GW = 16 * CELL, GH = 16 * CELL;
    int GX = LX + 80, GY = LY + 50;
    int by_t = GY + GH + 10, by_b = by_t + 20;

    for (;;) {
        ui_target();
        gfx_fill(0, 0, WINW, WINH, 0);
        gfx_text(GX, GY - 20, "Draw your 16x16 icon:", 0);
        for (int gy = 0; gy < 16; gy++) for (int gx = 0; gx < 16; gx++) {
            int l = GX + gx * CELL, t = GY + gy * CELL;
            if (bits[gy * 16 + gx] == '1') gfx_fill(l, t, l + CELL, t + CELL, 1);
            gfx_frame(l, t, l + CELL + 1, t + CELL + 1);
        }
        gfx_text(GX + GW + 16, GY, "Preview:", 0);
        draw_icon(bits, GX + GW + 16, GY + 14, 16);
        draw_icon(bits, GX + GW + 16, GY + 36, 11);
        draw_button(GX, by_t, GX + 64, by_b, "Save");
        draw_button(GX + 76, by_t, GX + 140, by_b, "Clear");
        draw_button(GX + 152, by_t, GX + 224, by_b, "Cancel");
        gfx_frame(0, 0, WINW, WINH);
        if (g_dark) gfx_invert(0, 0, WINW, WINH);
        ui_present();

        GEvent e; int r = ui_event(&e, 1);
        if (r < 0 || (r > 0 && e.type == GEV_CLOSE)) return;
        if (r > 0 && e.type == GEV_KEY && e.key == KEY_ESC) break;
        if (r > 0 && e.type == GEV_MOUSEDOWN) {
            int x = e.x, y = e.y;
            if (in_rect(x, y, GX, GY, GX + GW, GY + GH)) {
                int set = -1;
                do {
                    int mx = mouse_x(), my = mouse_y();
                    if (in_rect(mx, my, GX, GY, GX + GW, GY + GH)) {
                        int gx = (mx - GX) / CELL, gy = (my - GY) / CELL, idx = gy * 16 + gx;
                        if (set < 0) set = bits[idx] == '1' ? 0 : 1;
                        if (bits[idx] != (set ? '1' : '0')) {
                            bits[idx] = set ? '1' : '0';
                            int l = GX + gx * CELL, t = GY + gy * CELL;
                            ui_target();
                            int ink = set ^ g_dark;
                            gfx_fill(l + 1, t + 1, l + CELL, t + CELL, ink);
                            ui_present();
                        } else track_cursor();
                    }
                    net_poll();
                } while (mouse_down());
            } else if (in_rect(x, y, GX, by_t, GX + 64, by_b)) {
                scopy(g_icon, bits, sizeof g_icon);
                if (ws_connected) ws_set_icon(bits);
                break;
            } else if (in_rect(x, y, GX + 76, by_t, GX + 140, by_b)) {
                for (int i = 0; i < ICONBITS; i++) bits[i] = '0';
                bits[ICONBITS] = 0;
            } else if (in_rect(x, y, GX + 152, by_t, GX + 224, by_b)) break;
        }
    }
    g_dirty = 1;
}

/* ======================================================================== *
 *  Login screen (centred)
 * ======================================================================== */
static int li_field;                     /* 0 = user, 1 = pass */

static void draw_login(const char *err)
{
    int fx = LX + 150, fw = 240;
    ui_target();
    gfx_fill(0, 0, WINW, WINH, 0);

    /* IKANET wordmark + squid */
    const char *brand = "IKANET";
    int bw = gfx_text_width(brand) * 2;
    int bxx = LX + (MIN_W - bw) / 2 - 10, byy = LY + 60;
    for (int i = 0; brand[i]; i++) {                       /* double-size letters */
        for (int r = 0; r < 8; r++) {
            uint8_t g = plusos->font8x8[(brand[i] - 0x20) * 8 + r];
            for (int c = 0; c < 8; c++) if (g & (0x80 >> c))
                gfx_fill(bxx + i * 16 + c * 2, byy + r * 2, bxx + i * 16 + c * 2 + 2, byy + r * 2 + 2, 1);
        }
    }
    draw_icon(g_icon[0] ? g_icon : 0, bxx + bw + 6, byy, 16);

    gfx_text(fx - 50, LY + 112, "User", 0);
    gfx_frame(fx, LY + 107, fx + fw, LY + 123);
    gfx_text(fx + 4, LY + 111, g_user, 0);
    if (li_field == 0) gfx_vline(fx + 5 + gfx_text_width(g_user), LY + 109, LY + 121, 1);

    gfx_text(fx - 50, LY + 142, "Pass", 0);
    gfx_frame(fx, LY + 137, fx + fw, LY + 153);
    char stars[24]; int pl = (int)strlen(g_pass);
    for (int i = 0; i < pl && i < 23; i++) stars[i] = '*';
    stars[pl < 23 ? pl : 23] = 0;
    gfx_text(fx + 4, LY + 141, stars, 0);
    if (li_field == 1) gfx_vline(fx + 5 + gfx_text_width(stars), LY + 139, LY + 151, 1);

    gfx_text(fx - 50, LY + 170, "Icon", 0);
    draw_icon(g_icon[0] ? g_icon : 0, fx, LY + 165, 16);
    draw_button(fx + 26, LY + 164, fx + 116, LY + 182, "Edit Icon");

    draw_button(LX + 130, LY + 200, LX + 240, LY + 220, "JOIN IKANET");
    draw_button(LX + 270, LY + 200, LX + 350, LY + 220, "LOGIN");

    const char *note = (err && err[0]) ? err : g_status;
    if (note && note[0]) gfx_text(LX + (MIN_W - gfx_text_width(note)) / 2, LY + 238, note, 0);
    gfx_text(LX + (MIN_W - gfx_text_width("Tab: next field   Return: log in   Esc: quit")) / 2,
             LY + 290, "Tab: next field   Return: log in   Esc: quit", 0);

    gfx_frame(0, 0, WINW, WINH);
    if (g_dark) gfx_invert(0, 0, WINW, WINH);
    ui_present();
}

static void login_status_redraw(void){ draw_login(""); }

/* mode "login" / "join". Connects lazily on first use. 1 = logged in. */
static int try_auth(const char *mode)
{
    if (!g_user[0] || !g_pass[0]) { draw_login("Type a user name and password first"); return 0; }
    g_status_cb = login_status_redraw;
    if (!ws_connected && ikanet_connect()) { g_status_cb = 0; draw_login(""); return 0; }
    status(!strcmp(mode, "join") ? "Joining IKANET..." : "Logging in...");
    char err[80] = "";
    int ok = send_auth(mode, err, sizeof err);
    g_status_cb = 0;
    if (!ok) { if (!ws_connected) net_tcp_close(); draw_login(err); }
    return ok;
}

static int login_screen(void)
{
    li_field = 0;
    draw_login("");
    for (;;) {
        GEvent e;
        int r = ui_event(&e, 1);
        if (r < 0 || (r > 0 && e.type == GEV_CLOSE)) return 0;
        if (r > 0 && e.type == GEV_MOUSEDOWN) {
            int x = e.x, y = e.y, fx = LX + 150;
            if (in_rect(x, y, fx, LY + 105, fx + 240, LY + 125)) li_field = 0;
            else if (in_rect(x, y, fx, LY + 135, fx + 240, LY + 155)) li_field = 1;
            else if (in_rect(x, y, fx + 26, LY + 164, fx + 118, LY + 184)) icon_editor();
            else if (in_rect(x, y, LX + 130, LY + 200, LX + 242, LY + 222)) { if (try_auth("join")) return 1; continue; }
            else if (in_rect(x, y, LX + 270, LY + 200, LX + 352, LY + 222)) { if (try_auth("login")) return 1; continue; }
            else if (!g_fullscreen && y < MENU_B && x >= ZOOM_L - 2) toggle_maximize();
            draw_login("");
        }
        if (r > 0 && e.type == GEV_KEY) {
            int k = e.key;
            char *f = li_field == 0 ? g_user : g_pass;
            int fl = (int)strlen(f), cap = li_field == 0 ? 20 : 22;
            if (k == KEY_ESC || ((k == 'q' || k == 'Q') && cmd_down())) return 0;
            else if (k == KEY_TAB) li_field ^= 1;
            else if (k == KEY_RETURN || k == KEY_ENTER) {
                if (li_field == 0 && !g_pass[0]) li_field = 1;
                else if (try_auth("login")) return 1;
                else continue;
            }
            else if (k == KEY_BACKSPACE || k == KEY_DELETE) { if (fl) f[fl - 1] = 0; }
            else if (k >= 32 && k < 127 && fl < cap) { f[fl] = (char)k; f[fl + 1] = 0; }
            draw_login("");
        }
    }
}

/* ======================================================================== *
 *  Chat loop
 * ======================================================================== */
static void do_send(void)
{
    if (!ilen) return;
    input[ilen] = 0;
    if (!ws_connected) { sys_msg("Not connected - message not sent"); return; }
    if (ws_say(input) < 0) { sys_msg("Send failed - reconnecting"); g_reconnect_at = now(); return; }
    set_activity("SWIMMING, SWIMMING", 60);
    ilen = 0; input[0] = 0;
    stick_bottom = 1; scroll_to_end();
    g_dirty = 1;
}

static void chat_click(int x, int y)
{
    if (y < MENU_B) { menu_click(x); return; }
    if (y >= TITLE_T && y < TITLE_B) {
        int ax = 22 + gfx_text_width(g_user) + 8 + gfx_text_width(ws_connected ? "Online" : "Offline") + 20;
        if (x >= ax && x < ax + 12 + gfx_text_width("Appearing As Offline")) {
            g_appear_off = !g_appear_off;
            if (ws_connected) ws_send_text(g_appear_off ? "{\"t\":\"appear_offline\",\"value\":true}"
                                                        : "{\"t\":\"appear_offline\",\"value\":false}");
            g_dirty = 1;
        }
        return;
    }
    if (in_rect(x, y, SEND_X, COMP_T, WINW, COMP_T + COMP_H)) { do_send(); return; }
    if (in_rect(x, y, DL_X, COMP_T, SEND_X, COMP_T + COMP_H)) { sys_msg("File transfer is web-only for now - use ikanet.d0a.net"); return; }
    if (in_rect(x, y, SBX, MSG_T, WINW, MSG_T + SBW)) { if (scroll > 0) scroll--; stick_bottom = 0; g_dirty = 1; return; }
    if (in_rect(x, y, SBX, MSG_B - SBW, WINW, MSG_B)) {
        if (scroll < nmsg - VISLINES) scroll++;
        stick_bottom = scroll >= nmsg - VISLINES; g_dirty = 1; return;
    }
    if (in_rect(x, y, SBX, MSG_T + SBW, WINW, MSG_B - SBW) && nmsg > VISLINES) {   /* page */
        int mid = (MSG_T + MSG_B) / 2;
        scroll += (y < mid) ? -VISLINES : VISLINES;
        if (scroll < 0) scroll = 0;
        if (scroll > nmsg - VISLINES) scroll = nmsg - VISLINES;
        stick_bottom = scroll >= nmsg - VISLINES; g_dirty = 1;
    }
}

static int chat_key(int k)               /* returns 1 to quit */
{
    if (k == KEY_FKEY || ((k == 'q' || k == 'Q') && cmd_down())) return 1;
    if (g_fullscreen && k == KEY_ESC) return 1;
    if ((k == 'm' || k == 'M') && cmd_down()) { toggle_maximize(); return 0; }
    if ((k == 'c' || k == 'C') && cmd_down()) { scopy(g_clip, input, sizeof g_clip); return 0; }
    if ((k == 'v' || k == 'V') && cmd_down()) {
        for (const char *p = g_clip; *p && ilen < (int)sizeof input - 1; p++) input[ilen++] = *p;
        input[ilen] = 0; g_dirty = 1; return 0;
    }
    if (k == KEY_RETURN || k == KEY_ENTER) do_send();
    else if (k == KEY_BACKSPACE || k == KEY_DELETE) { if (ilen) input[--ilen] = 0; }
    else if (k == KEY_UP || k == KEY_PGUP) { scroll -= (k == KEY_UP) ? 1 : VISLINES; if (scroll < 0) scroll = 0; stick_bottom = 0; }
    else if (k == KEY_DOWN || k == KEY_PGDN) {
        scroll += (k == KEY_DOWN) ? 1 : VISLINES;
        int mx = nmsg > VISLINES ? nmsg - VISLINES : 0;
        if (scroll > mx) scroll = mx;
        stick_bottom = scroll >= mx;
    }
    else if (k >= 32 && k < 127 && ilen < (int)sizeof input - 1) { input[ilen++] = (char)k; input[ilen] = 0; }
    g_dirty = 1;
    return 0;
}

/* ======================================================================== *
 *  main
 * ======================================================================== */
int main(int argc, char **argv)
{
    int force = 0;                       /* 1 = full screen, 2 = window */
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (a[0] == '-') {
            char c = a[1] == '-' ? a[2] : a[1];
            if (c == 'f' || c == 'F') force = 1;
            else if (c == 'w' || c == 'W') force = 2;
            continue;
        }
        if (pos == 0) scopy(ws_host, a, sizeof ws_host);
        else if (pos == 1) ws_port = atoi(a);
        else if (pos == 2) scopy(ws_path, a, sizeof ws_path);
        pos++;
    }

    int desk = under_desktop();
    g_fullscreen = force == 1 ? 1 : force == 2 ? !desk : !desk;
    if (g_fullscreen) {
        if (setup_fullscreen()) { printf("ikanet: out of memory\n"); return 1; }
    } else if (open_window(MIN_W, MIN_H)) {
        printf("ikanet: cannot open a window\n");
        return 1;
    }
    default_icon(g_icon);
    scopy(g_status, "Type a name, then JOIN or LOGIN", sizeof g_status);

    if (!login_screen()) goto out;

    set_activity("", 0);
    sys_msg("Welcome to IKANET!");
    redraw();

    for (;;) {
        if (ws_connected) { pump_net(); if (ws_connected) ping_tick(); }
        else try_reconnect();

        GEvent e;
        int r = ui_event(&e, 0);
        if (r < 0 || (r > 0 && e.type == GEV_CLOSE)) break;
        if (r > 0 && e.type == GEV_MOUSEDOWN) chat_click(e.x, e.y);
        if (r > 0 && e.type == GEV_KEY && chat_key(e.key)) break;
        if (g_authed < 0) break;                             /* File > Quit */

        if (g_activity_until && now() > g_activity_until) set_activity("", 0);

        if (g_dirty) redraw();
        else if (r == 0) { track_cursor(); plusos->sleep_ticks(1); }
    }

out:
    if (ws_connected) net_tcp_close();
    if (g_fullscreen) {
        if (!canvas_is_bb && canvas.owned) gs_free(&canvas);
        gfx_target(gfx_backbuffer());
        gfx_fill(0, 0, gfx_backbuffer()->w, gfx_backbuffer()->h, 0);
        gfx_present();
        plusos->con_clear();
    } else gwin_close(&win);
    return 0;
}
