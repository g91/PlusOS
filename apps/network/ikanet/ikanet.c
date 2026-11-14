/* ikanet - the IKANET chat client for PlusOS.
 *
 * A native 1-bit Macintosh application that connects over TCP/IP (PlusNET) to
 * the IKANET server and chats in rooms and private messages, with a login
 * screen and chat window drawn to match the IKANET design:
 *
 *   +-------------------------------------------------------------+
 *   | * File Edit Channel Font DarkMode                           |  menu bar
 *   +-------------------------------------------------------------+
 *   | [ic] user  (Online)  o Appearing As Offline     Nice Ping! |  title/status
 *   +-------------------------------------------------------------+
 *   | [ic]|user| message text .....                            ^  |
 *   |                                                          #  |  messages
 *   | [ic]|user| ...                                           v  |
 *   +-------------------------------------------------------------+
 *   | [ic]| type here...                        | DL | UP | SEND  |  composer
 *   +-------------------------------------------------------------+
 *   | LONG LIVE THE APPLE III      SWIMMING, SWIMMING   IKANET *  |  status strip
 *   +-------------------------------------------------------------+
 *
 * Transport: plain WebSocket (RFC 6455) to ws://HOST/ws over TCP. This is the
 * SAME endpoint the browser client uses, so it rides one port behind Cloudflare
 * (443 wss -> origin 8787 ws; TLS is terminated at Cloudflare / a reverse proxy,
 * never on the 68000). Messages are JSON, matching the server hub:
 *   -> {"t":"auth","mode":"login"|"join","user":..,"pass":..}
 *   -> {"t":"subscribe","channel_id":N}   {"t":"say","channel_id":N,"body":".."}
 *   -> {"t":"add_channel","name":"#x"}    {"t":"open_dm","username":".."}
 *   -> {"t":"ping","n":T}
 *   <- {"t":"welcome","user":{..},"channels":[..],"quote":".."}
 *   <- {"t":"history","channel_id":N,"messages":[..]}
 *   <- {"t":"msg","channel_id":N,"user":"..","body":"..","system":false}
 *   <- {"t":"pong","n":T}   {"t":"error","msg":".."}
 *
 * Usage:  ikanet [HOST] [PORT] [PATH]   (defaults: ikanet.d0a.net 80 /ws)
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusnet/net.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- window geometry ------------------------------------------------------ */
#define WINW     480
#define WINH     320
#define MENU_B   16                      /* menu bar [0,16)            */
#define TITLE_T  MENU_B
#define TITLE_B  (MENU_B + 18)           /* title/status row          */
#define MSG_T    TITLE_B
#define COMP_H   18
#define STRIP_H  14
#define COMP_T   (WINH - STRIP_H - COMP_H)
#define MSG_B    COMP_T
#define STRIP_T  (WINH - STRIP_H)
#define SBW      15
#define SBX      (WINW - SBW)            /* message scrollbar x       */
#define LINEH    11
#define VISLINES ((MSG_B - MSG_T - 2) / LINEH)

/* composer buttons, right to left */
#define SEND_W   40
#define TOOL_W   22
#define SEND_X   (WINW - SEND_W)
#define UP_X     (SEND_X - TOOL_W)
#define DL_X     (UP_X - TOOL_W)
#define COMP_ICON 18

/* ---- state ---------------------------------------------------------------- */
static GWindow win;
static int origin_x, origin_y;           /* window content origin on screen */

#define NMSG   200
#define MSGLEN 160
#define ICONBITS 256                      /* 16x16 mono, chars '0'/'1'  */
typedef struct { char from[24]; char text[MSGLEN]; char sys; char icon[ICONBITS+1]; } Msg;
static Msg  msgs[NMSG];
static int  nmsg;
static int  scroll;                      /* index of first visible message */

static char g_user[24];
static char g_icon[ICONBITS+1];          /* the logged-in user's own 16x16 icon */
static char g_chan[40] = "#general";
static char g_quote[48] = "LONG LIVE THE APPLE III";
static char g_activity[32];
static int  g_online = 1;                /* Appearing As Online */
static int  g_dark = 1;                  /* dark mode (inverted) on by default to match mockup */
static int  g_connected;

static char input[200];
static int  ilen;

/* ---- tiny helpers --------------------------------------------------------- */
static int in_rect(int x,int y,int l,int t,int r,int b){return x>=l&&x<r&&y>=t&&y<b;}
static int mouse_x(void){ return *(volatile int16_t*)0x832 - origin_x; }
static int mouse_y(void){ return *(volatile int16_t*)0x830 - origin_y; }
static int mouse_down(void){ return (*(volatile uint8_t*)0x172 & 0x80) == 0; }
static int cmd_down(void){ return (plusos->key_mods() & 0x0100) != 0; }
static void scopy(char*d,const char*s,int n){int i=0;for(;s[i]&&i<n-1;i++)d[i]=s[i];d[i]=0;}

/* ======================================================================== *
 *  WebSocket client (RFC 6455) over PlusNET TCP.
 *  TLS is terminated upstream (Cloudflare / reverse proxy), so we speak plain
 *  ws:// here - no crypto on the 68000. We send masked text frames and read
 *  (possibly fragmented) server frames, answering control pings.
 * ======================================================================== */
static char ws_host[64] = "ikanet.d0a.net";
static char ws_path[64] = "/ws";
static int  ws_connected;

/* reassembled receive bytes (raw TCP) and one decoded message payload */
static uint8_t ws_rx[2048];
static int     ws_rxlen;
static char    ws_msg[1600];       /* a complete decoded text message */

static uint32_t ws_rand(void)      /* cheap PRNG for the mask + key */
{
    static uint32_t s;
    if (!s) s = plusos->ticks() ^ 0xA5A5u;
    s = s*1664525u + 1013904223u;
    return s;
}

/* Handshake: send the upgrade request, wait for "101". Returns 0 on success. */
static int ws_handshake(void)
{
    char key[32];
    uint32_t r = ws_rand();
    /* a fixed-shape base64-ish 24-char key; servers don't require real entropy */
    snprintf(key,sizeof key,"pLuS%08lx%08lxAA==",(unsigned long)r,(unsigned long)ws_rand());
    char req[400];
    int n = snprintf(req,sizeof req,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        ws_path, ws_host, key);
    if (net_tcp_send(req,n) < 0) return -1;

    /* read response headers until CRLFCRLF */
    char hbuf[512]; int hl=0;
    uint32_t end = plusos->ticks()+300;
    while (plusos->ticks()<end){
        net_poll();
        uint8_t b[128];
        int got = net_tcp_recv(b,sizeof b);
        if (got < 0) return -1;
        for (int i=0;i<got && hl<(int)sizeof hbuf-1;i++) hbuf[hl++]=(char)b[i];
        hbuf[hl]=0;
        if (strstr(hbuf,"\r\n\r\n")) break;
    }
    hbuf[hl]=0;
    if (!strstr(hbuf," 101")) return -1;
    ws_rxlen = 0;
    return 0;
}

/* Send one masked text frame. Returns 0 ok. */
static int ws_send_text(const char *s)
{
    int len = (int)strlen(s);
    uint8_t hdr[8]; int hn=0;
    hdr[hn++] = 0x81;                        /* FIN + text */
    uint8_t mask[4];
    uint32_t mk = ws_rand();
    mask[0]=mk; mask[1]=mk>>8; mask[2]=mk>>16; mask[3]=mk>>24;
    if (len < 126){ hdr[hn++] = 0x80 | (uint8_t)len; }
    else { hdr[hn++] = 0x80 | 126; hdr[hn++] = (uint8_t)(len>>8); hdr[hn++] = (uint8_t)len; }
    if (net_tcp_send(hdr,hn) < 0) return -1;
    if (net_tcp_send(mask,4) < 0) return -1;
    /* masked payload, chunked */
    static uint8_t out[512];
    int off=0;
    while (off < len){
        int c = len-off; if (c>(int)sizeof out) c=(int)sizeof out;
        for (int i=0;i<c;i++) out[i] = (uint8_t)s[off+i] ^ mask[(off+i)&3];
        if (net_tcp_send(out,c) < 0) return -1;
        off += c;
    }
    return 0;
}

/* Pull TCP bytes into ws_rx; decode exactly one complete text frame into ws_msg.
 * Returns 1 if a message is ready, 0 if not yet, -1 on disconnect. */
static int ws_poll_message(void)
{
    net_poll();
    uint8_t b[512];
    int got = net_tcp_recv(b,sizeof b);
    if (got < 0) return -1;
    for (int i=0;i<got && ws_rxlen<(int)sizeof ws_rx;i++) ws_rx[ws_rxlen++]=b[i];

    for (;;){
        if (ws_rxlen < 2) return 0;
        uint8_t op = ws_rx[0] & 0x0F;
        int masked = ws_rx[1] & 0x80;
        uint32_t plen = ws_rx[1] & 0x7F;
        int hsz = 2;
        if (plen == 126){ if (ws_rxlen<4) return 0; plen = (ws_rx[2]<<8)|ws_rx[3]; hsz=4; }
        else if (plen == 127){ if (ws_rxlen<10) return 0; plen = (ws_rx[6]<<24)|(ws_rx[7]<<16)|(ws_rx[8]<<8)|ws_rx[9]; hsz=10; }
        int moff = hsz; if (masked) hsz += 4;
        if ((uint32_t)ws_rxlen < hsz + plen) return 0;        /* wait for more */
        uint8_t *pay = ws_rx + hsz;
        if (masked){ uint8_t *mk = ws_rx+moff; for (uint32_t k=0;k<plen;k++) pay[k]^=mk[k&3]; }

        int ready = 0;
        if (op == 0x8){ return -1; }                           /* close */
        else if (op == 0x9){                                   /* ping -> pong */
            uint8_t pong[2+125]; int pn=0; pong[pn++]=0x8A; pong[pn++]=(uint8_t)(plen&0x7F);
            for (uint32_t k=0;k<plen && k<125;k++) pong[pn++]=pay[k];
            net_tcp_send(pong,pn);
        } else if (op == 0x1 || op == 0x0){                    /* text / continuation */
            uint32_t c = plen; if (c > sizeof ws_msg-1) c = sizeof ws_msg-1;
            memcpy(ws_msg,pay,c); ws_msg[c]=0; ready=1;
        }
        /* consume the frame */
        int total = hsz + (int)plen;
        memmove(ws_rx, ws_rx+total, ws_rxlen-total);
        ws_rxlen -= total;
        if (ready) return 1;
    }
}

/* ---- minimal JSON field reader (string/number by key) -------------------- */
/* find "key": in s, copy the following JSON string value into out. 0 ok. */
static int json_str(const char *s,const char *key,char *out,int max)
{
    char pat[48]; snprintf(pat,sizeof pat,"\"%s\"",key);
    const char *p = strstr(s,pat); if(!p) return -1;
    p += strlen(pat);
    while (*p==' '||*p==':') p++;
    if (*p!='"') return -1; p++;
    int o=0;
    while (*p && *p!='"' && o<max-1){
        if (*p=='\\' && p[1]){ p++; if(*p=='n'){out[o++]='\n';} else out[o++]=*p; }
        else out[o++]=*p;
        p++;
    }
    out[o]=0; return 0;
}
static int json_type(const char *s,char *out,int max){ return json_str(s,"t",out,max); }

/* ---- drawing -------------------------------------------------------------- */
/* In "dark mode" we draw normally then invert the whole window at the end, so
 * black ink on white becomes white ink on black like the IKANET mockups. */

static void draw_frame(int l,int t,int r,int b){ gfx_frame(l,t,r,b); }

static void draw_button(int l,int t,int r,int b,const char*s,int pressed)
{
    gfx_fill_round_rect(l,t,r,b,4,0);              /* black fill  */
    gfx_fill_round_rect(l+1,t+1,r-1,b-1,3,1);      /* white inset */
    int tw = gfx_text_width(s);
    int tx = l + ((r-l)-tw)/2;
    int ty = t + ((b-t)-8)/2;
    gfx_text(tx,ty,s,0);
    if (pressed) gfx_invert(l,t,r,b);
}

static void draw_menubar(void)
{
    gfx_fill(0,0,WINW,MENU_B,1);
    gfx_hline(0,WINW,MENU_B-1,1);
    static const char *items[] = {"\024"," File"," Edit"," Channel"," Font"," DarkMode"};
    int x = 2;
    /* apple */
    gfx_text(x+2,4," ",0); gfx_fill_circle(x+5,7,3,1); x += 16;
    for (int i=1;i<6;i++){ gfx_text(x,4,items[i],0); x += gfx_text_width(items[i]) + 10; }
}

static void draw_bars(int x,int y,int n)        /* signal strength 1..4 */
{
    for (int i=0;i<4;i++){
        int h = 3 + i*2;
        if (i < n) gfx_fill(x+i*4, y+8-h, x+i*4+3, y+8, 1);
        else       gfx_frame(x+i*4, y+8-h, x+i*4+3, y+8);
    }
}

static void draw_titlebar(int ping_ms)
{
    gfx_hline(0,WINW,TITLE_B-1,1);
    /* logged-in user's own 16x16 icon */
    draw_icon(g_icon[0]?g_icon:NULL, 3, TITLE_T+2, 14);
    int x = 20;
    x = gfx_text(x,TITLE_T+5,g_user,0) + 8;
    /* online pill */
    const char *st = g_online ? "Online" : "Offline";
    int pw = gfx_text_width(st)+10;
    gfx_round_rect(x,TITLE_T+3,x+pw,TITLE_B-3,6,1);
    gfx_text(x+5,TITLE_T+5,st,0); x += pw + 10;
    /* appearing-as toggle */
    x = gfx_text(x,TITLE_T+5, g_online ? "o Appearing As Offline" : "* Appearing As Offline", 0) + 10;

    /* ping (right aligned) */
    char pbuf[32];
    const char *label = ping_ms < 0 ? "..." : ping_ms < 80 ? "Nice Ping!" :
                        ping_ms < 200 ? "OK Ping" : "Slow Ping...";
    int bars = ping_ms < 0 ? 0 : ping_ms < 80 ? 4 : ping_ms < 200 ? 3 : ping_ms < 450 ? 2 : 1;
    if (ping_ms >= 0) snprintf(pbuf,sizeof pbuf,"%s %d",label,ping_ms);
    else              snprintf(pbuf,sizeof pbuf,"%s",label);
    int pw2 = gfx_text_width(pbuf);
    int px = SBX - pw2 - 24;
    draw_bars(px,TITLE_T+5,bars);
    gfx_text(px+18,TITLE_T+5,pbuf,0);
}

static void draw_messages(void)
{
    gfx_fill(0,MSG_T,SBX,MSG_B,1);               /* clear (white) */
    int y = MSG_T + 2;
    int shown = 0;
    for (int i = scroll; i < nmsg && shown < VISLINES; i++, shown++){
        Msg *m = &msgs[i];
        if (m->sys){
            /* system line in a box */
            int w = gfx_text_width(m->text);
            gfx_frame(4,y,8+w,y+LINEH-1);
            gfx_text(6,y+1,m->text,0);
        } else {
            draw_icon(m->icon[0]?m->icon:NULL, 4, y, 9);   /* 16x16 icon -> 9px slot */
            char head[40]; snprintf(head,sizeof head,"|%s| ", m->from);
            int hx = gfx_text(16,y+1,head,0);
            /* clip body text to the message area width */
            char body[MSGLEN];
            scopy(body,m->text,sizeof body);
            int maxpx = SBX - hx - 4;
            while (gfx_text_width(body) > maxpx && body[0]){
                body[strlen(body)-1] = 0;
            }
            gfx_text(hx,y+1,body,0);
        }
        y += LINEH;
    }
    /* scrollbar */
    gfx_vline(SBX,MSG_T,MSG_B,1);
    gfx_frame(SBX,MSG_T,WINW,MSG_B);
    gfx_frame(SBX,MSG_T,WINW,MSG_T+SBW);          /* up box  */
    gfx_frame(SBX,MSG_B-SBW,WINW,MSG_B);          /* down box*/
    gfx_text(SBX+4,MSG_T+4,"^",0);
    gfx_text(SBX+4,MSG_B-SBW+4,"v",0);
}

static void draw_composer(void)
{
    gfx_hline(0,WINW,COMP_T,1);
    /* my 16x16 icon */
    draw_icon(g_icon[0]?g_icon:NULL, 2, COMP_T+2, COMP_H-4);
    gfx_vline(COMP_ICON+1,COMP_T,COMP_T+COMP_H,1);
    /* text */
    char show[200]; scopy(show,input,sizeof show);
    int tx = COMP_ICON + 6;
    int maxpx = DL_X - tx - 4;
    while (gfx_text_width(show) > maxpx && show[0]) memmove(show,show+1,strlen(show));
    int ex = gfx_text(tx,COMP_T+5,show,0);
    gfx_vline(ex+1,COMP_T+3,COMP_T+COMP_H-3,1);   /* caret */
    /* tool buttons */
    gfx_vline(DL_X,COMP_T,COMP_T+COMP_H,1);
    gfx_vline(UP_X,COMP_T,COMP_T+COMP_H,1);
    gfx_vline(SEND_X,COMP_T,COMP_T+COMP_H,1);
    gfx_text(DL_X+5,COMP_T+5,"DL",0);
    gfx_text(UP_X+5,COMP_T+5,"UP",0);
    gfx_text(SEND_X+6,COMP_T+5,"SEND",0);
}

static void draw_strip(void)
{
    gfx_hline(0,WINW,STRIP_T,1);
    gfx_text(4,STRIP_T+3,g_quote,0);
    gfx_vline(gfx_text_width(g_quote)+10,STRIP_T,WINH,1);
    if (g_activity[0]){
        int w = gfx_text_width(g_activity);
        gfx_text(WINW-90-w, STRIP_T+3, g_activity, 0);
    }
    int bx = WINW - 70;
    gfx_vline(bx,STRIP_T,WINH,1);
    gfx_text(bx+4,STRIP_T+3,"IKANET",0);
}

static int g_ping = -1;
static void redraw(void)
{
    gfx_target(&win.surf);
    gfx_fill(0,0,WINW,WINH,1);
    draw_menubar();
    draw_titlebar(g_ping);
    draw_messages();
    draw_composer();
    draw_strip();
    draw_frame(0,0,WINW,WINH);
    if (g_dark) gfx_invert(0,0,WINW,WINH);        /* DarkMode = whole-window invert */
    gwin_update(&win);
}

/* ---- 16x16 user icons ----------------------------------------------------- */
/* An icon is 256 chars of '0'/'1', row-major, bit i at (i%16, i/16). */
static void default_icon(char *out)
{
    /* a little squid silhouette so new users aren't blank */
    static const char *art =
        "0001111000000000"
        "0011001100000000"
        "0110000110000000"
        "0100110010000000"
        "0100110010000000"
        "0110000110000000"
        "0011111100000000"
        "0010101010000000"
        "0010101010000000"
        "0010101010000000"
        "0001010100000000"
        "0000000000000000"
        "0000000000000000"
        "0000000000000000"
        "0000000000000000"
        "0000000000000000";
    scopy(out, art, ICONBITS+1);
}

/* Draw a 16x16 icon scaled into a box [l,t]..[l+sz,t+sz] (sz is 9..16). */
static void draw_icon(const char *bits, int l, int t, int sz)
{
    if (!bits || (int)strlen(bits) < ICONBITS){ gfx_frame(l,t,l+sz,t+sz); return; }
    for (int gy=0; gy<sz; gy++){
        int sy = gy*16/sz;
        for (int gx=0; gx<sz; gx++){
            int sx = gx*16/sz;
            if (bits[sy*16+sx]=='1') gfx_pixel(l+gx, t+gy, 1);
        }
    }
    gfx_frame(l,t,l+sz,t+sz);
}

/* ---- message log ---------------------------------------------------------- */
static void push_msg(const char *from, const char *text, int sys)
{ /* kept for system/local lines (no icon) */
    if (nmsg >= NMSG){
        memmove(msgs, msgs+1, sizeof(Msg)*(NMSG-1));
        nmsg = NMSG-1;
    }
    Msg *m = &msgs[nmsg++];
    scopy(m->from, from?from:"", sizeof m->from);
    scopy(m->text, text?text:"", sizeof m->text);
    m->sys = (char)sys;
    m->icon[0] = 0;
    if (nmsg > VISLINES) scroll = nmsg - VISLINES; else scroll = 0;
}
static void push_msg_icon(const char *from, const char *text, int sys, const char *icon)
{
    push_msg(from, text, sys);
    if (icon) scopy(msgs[nmsg-1].icon, icon, ICONBITS+1);
}

/* ---- protocol (JSON over WebSocket) --------------------------------------- */
static int g_chan_id = -1;

/* JSON-escape a user string into out */
static void json_esc(const char *s,char *out,int max)
{
    int o=0;
    for (;*s && o<max-2;s++){
        if (*s=='"'||*s=='\\'){ out[o++]='\\'; out[o++]=*s; }
        else if (*s=='\n'){ out[o++]='\\'; out[o++]='n'; }
        else if ((unsigned char)*s>=32) out[o++]=*s;
    }
    out[o]=0;
}

static int ws_subscribe(int cid){ char b[64]; snprintf(b,sizeof b,"{\"t\":\"subscribe\",\"channel_id\":%d}",cid); return ws_send_text(b); }
static int ws_say(const char *text)
{
    char esc[400]; json_esc(text,esc,sizeof esc);
    char b[600]; snprintf(b,sizeof b,"{\"t\":\"say\",\"channel_id\":%d,\"body\":\"%s\"}",g_chan_id,esc);
    return ws_send_text(b);
}
static int ws_add_channel(const char *name)
{
    char esc[64]; json_esc(name,esc,sizeof esc);
    char b[120]; snprintf(b,sizeof b,"{\"t\":\"add_channel\",\"name\":\"%s\"}",esc);
    return ws_send_text(b);
}
static int ws_set_icon(const char *bits)      /* bits = 256 chars '0'/'1' */
{
    char b[300]; snprintf(b,sizeof b,"{\"t\":\"set_icon\",\"icon\":\"%s\"}",bits);
    return ws_send_text(b);
}

/* extract an integer value for a key, default -1 */
static int json_int(const char *s,const char *key)
{
    char pat[48]; snprintf(pat,sizeof pat,"\"%s\"",key);
    const char *p=strstr(s,pat); if(!p) return -1;
    p+=strlen(pat); while(*p==' '||*p==':')p++;
    int neg=0; if(*p=='-'){neg=1;p++;}
    if(*p<'0'||*p>'9') return -1;
    int v=0; while(*p>='0'&&*p<='9'){v=v*10+(*p-'0');p++;}
    return neg?-v:v;
}

/* parse one message object (from "msg" or a history element) and log it */
static void ingest_msg_obj(const char *obj)
{
    char user[24]="", body[MSGLEN]="", icon[ICONBITS+1]="";
    json_str(obj,"user",user,sizeof user);
    json_str(obj,"body",body,sizeof body);
    json_str(obj,"icon",icon,sizeof icon);         /* 16x16 sender icon, may be absent */
    int is_sys = (strstr(obj,"\"system\":true")!=NULL) || (user[0]==0);
    push_msg_icon(is_sys?"":user, body, is_sys, is_sys?NULL:(icon[0]?icon:NULL));
}

/* dispatch a decoded server message */
static uint32_t g_last_pong_rtt = 0;
static int      g_ping_outstanding = 0;
static uint32_t g_ping_sent_at = 0;

static void handle_json(const char *s)
{
    char t[24]; if (json_type(s,t,sizeof t)) return;
    if (!strcmp(t,"welcome")){
        json_str(s,"quote",g_quote,sizeof g_quote);
        /* find the first channel id in the channels array */
        const char *ch = strstr(s,"\"channels\"");
        if (ch){ int id=json_int(ch,"id"); if(id>0){ g_chan_id=id; ws_subscribe(id); } }
        g_connected=1;
    }
    else if (!strcmp(t,"history")){
        int cid=json_int(s,"channel_id");
        if (cid==g_chan_id){
            nmsg=0;
            /* walk each {...} object inside "messages":[ ... ] */
            const char *p=strstr(s,"\"messages\"");
            if (p){ p=strchr(p,'['); if(p){ p++;
                while (*p && *p!=']'){
                    if (*p=='{'){
                        const char *e=p; int depth=0;
                        for (;*e;e++){ if(*e=='{')depth++; else if(*e=='}'){depth--; if(!depth){e++;break;}} }
                        char obj[700]; int c=e-p; if(c>(int)sizeof obj-1)c=sizeof obj-1;
                        memcpy(obj,p,c); obj[c]=0; ingest_msg_obj(obj); p=e;
                    } else p++;
                }
            }}
        }
    }
    else if (!strcmp(t,"msg")){
        int cid=json_int(s,"channel_id");
        if (cid==g_chan_id) ingest_msg_obj(s);
    }
    else if (!strcmp(t,"pong")){
        if (g_ping_outstanding){ g_last_pong_rtt = plusos->ticks()-g_ping_sent_at; g_ping_outstanding=0; }
    }
    else if (!strcmp(t,"error")){
        char m[120]=""; json_str(s,"msg",m,sizeof m); push_msg("",m[0]?m:"error",1);
    }
    else if (!strcmp(t,"channel_added") || !strcmp(t,"dm_opened")){
        const char *c=strstr(s,"\"channel\""); int id=c?json_int(c,"id"):-1;
        if (id>0){ g_chan_id=id; nmsg=0; ws_subscribe(id); }
    }
    else if (!strcmp(t,"icon_set")){
        json_str(s,"icon",g_icon,sizeof g_icon);   /* server confirmed our new icon */
    }
}

static void pump_net(void)
{
    int r = ws_poll_message();
    if (r < 0){ g_connected=0; ws_connected=0; push_msg("","[disconnected]",1); return; }
    if (r == 1) handle_json(ws_msg);
}

/* ---- ping timing ---------------------------------------------------------- */
static uint32_t last_ping;
static void ping_tick(void)
{
    uint32_t t = plusos->ticks();
    if (!g_ping_outstanding && t - last_ping > 600){   /* every ~10s (60 ticks/s) */
        char b[48]; snprintf(b,sizeof b,"{\"t\":\"ping\",\"n\":%lu}",(unsigned long)t);
        if (ws_send_text(b) >= 0){ g_ping_sent_at=t; g_ping_outstanding=1; last_ping=t;
            /* convert last rtt (ticks) to ms for the meter: ~16.6ms/tick */
            g_ping = (int)(g_last_pong_rtt * 50 / 3);
        }
    }
}

/* ---- menus (drop-down) ---------------------------------------------------- */
/* Simple modal menu: draw items, track until release, return chosen index. */
static int popup_menu(int mx,const char**items,int n)
{
    int iw=0; for(int i=0;i<n;i++){int w=gfx_text_width(items[i]);if(w>iw)iw=w;}
    iw+=16; int ih=n*14+2;
    int l=mx, t=MENU_B, r=l+iw, b=t+ih;
    if (r>WINW) r=WINW, l=r-iw;
    gfx_target(&win.surf);
    /* save under menu by just redrawing afterwards */
    gfx_fill(l,t,r,b,1); gfx_frame(l,t,r,b);
    for(int i=0;i<n;i++) gfx_text(l+8,t+2+i*14,items[i],0);
    if (g_dark) gfx_invert(l,t,r,b);
    gwin_update(&win);
    int chosen=-1;
    while (mouse_down()){
        int my=mouse_y();
        int idx=(my-t-2)/14;
        if (mouse_x()>=l && mouse_x()<r && idx>=0 && idx<n) chosen=idx; else chosen=-1;
        net_poll();
    }
    return chosen;
}

/* ---- icon editor (16x16) -------------------------------------------------- */
/* Modal: a 16x16 grid of 12px cells. Click/drag toggles; Save sends set_icon. */
static void icon_editor(void)
{
    char bits[ICONBITS+1];
    scopy(bits, g_icon[0]?g_icon:"", ICONBITS+1);
    if ((int)strlen(bits) < ICONBITS){ if(g_icon[0]) default_icon(bits); else default_icon(bits); }
    const int CELL=12, GX=100, GY=MENU_B+40, GW=16*CELL, GH=16*CELL;
    const int bSave_l=GX, bSave_r=GX+70, bClr_l=GX+80, bClr_r=GX+150, bCan_l=GX+160, bCan_r=GX+240;
    const int by_t=GY+GH+8, by_b=GY+GH+26;

    int running=1;
    while (running){
        /* draw */
        gfx_target(&win.surf);
        gfx_fill(0,0,WINW,WINH,1);
        gfx_text(GX, MENU_B+16, "Draw your 16x16 icon:", 0);
        /* grid */
        for (int gy=0; gy<16; gy++) for (int gx=0; gx<16; gx++){
            int l=GX+gx*CELL, t=GY+gy*CELL;
            if (bits[gy*16+gx]=='1') gfx_fill(l,t,l+CELL,t+CELL,1);
            gfx_frame(l,t,l+CELL,t+CELL);
        }
        /* live preview at real size */
        gfx_text(GX+GW+16, GY, "Preview:", 0);
        draw_icon(bits, GX+GW+16, GY+14, 16);
        /* buttons */
        draw_button(bSave_l,by_t,bSave_r,by_b,"Save",0);
        draw_button(bClr_l,by_t,bClr_r,by_b,"Clear",0);
        draw_button(bCan_l,by_t,bCan_r,by_b,"Cancel",0);
        if (g_dark) gfx_invert(0,0,WINW,WINH);
        gwin_update(&win);

        /* event */
        GEvent e; int r=gwin_event(&win,&e,1);
        if (r<0 || (r>0 && e.type==GEV_CLOSE)) return;
        if (r>0 && e.type==GEV_MOUSEDOWN){
            int x=e.x,y=e.y;
            if (in_rect(x,y,GX,GY,GX+GW,GY+GH)){
                /* paint: set the first cell, then track drag until release */
                int set = -1;
                do {
                    int mx=mouse_x(), my=mouse_y();
                    if (in_rect(mx,my,GX,GY,GX+GW,GY+GH)){
                        int gx=(mx-GX)/CELL, gy=(my-GY)/CELL, idx=gy*16+gx;
                        if (set<0) set = (bits[idx]=='1')?0:1;     /* toggle sense from first cell */
                        bits[idx] = set?'1':'0';
                    }
                    net_poll();
                    /* redraw cells quickly while dragging */
                    gfx_target(&win.surf);
                } while (mouse_down());
            }
            else if (in_rect(x,y,bSave_l,by_t,bSave_r,by_b)){
                scopy(g_icon,bits,sizeof g_icon);
                if (ws_connected) ws_set_icon(bits);
                return;
            }
            else if (in_rect(x,y,bClr_l,by_t,bClr_r,by_b)){
                for (int i=0;i<ICONBITS;i++) bits[i]='0'; bits[ICONBITS]=0;
            }
            else if (in_rect(x,y,bCan_l,by_t,bCan_r,by_b)) return;
        }
    }
}

static void do_logout(void){ net_tcp_close(); g_connected=0; ws_connected=0; }

static void menu_click(int mx)
{
    if (mx < 16) return;                              /* apple */
    int x = 16;
    const char *bar[] = {" File"," Edit"," Channel"," Font"," DarkMode"};
    int which=-1,bx=x;
    for (int i=0;i<5;i++){ int w=gfx_text_width(bar[i])+10; if(mx>=bx&&mx<bx+w){which=i;break;} bx+=w; }
    if (which<0) return;
    if (which==4){ g_dark=!g_dark; redraw(); return; }  /* DarkMode toggles */

    if (which==0){ /* File */
        const char *it[]={"Upload File","Download File","Appear Online","Appear Offline","User Icon...","Log Out"};
        int c=popup_menu(bx,it,6);
        if (c==2){ g_online=1; if(ws_connected) ws_send_text("{\"t\":\"appear_offline\",\"value\":false}"); }
        else if (c==3){ g_online=0; if(ws_connected) ws_send_text("{\"t\":\"appear_offline\",\"value\":true}"); }
        else if (c==4){ icon_editor(); }
        else if (c==5){ do_logout(); }
    } else if (which==2){ /* Channel */
        const char *it[]={"Go To #general","Add #swimming","Add #gameboy"};
        int c=popup_menu(bx,it,3);
        if (c==0){ scopy(g_chan,"#general",sizeof g_chan); ws_add_channel("#general"); nmsg=0; }
        else if (c==1){ scopy(g_chan,"#swimming",sizeof g_chan); ws_add_channel("#swimming"); nmsg=0; }
        else if (c==2){ scopy(g_chan,"#gameboy",sizeof g_chan); ws_add_channel("#gameboy"); nmsg=0; }
    } else if (which==3){ /* Font - cosmetic (system font only) */
        const char *it[]={"Chicago","Times","Arial","Compact"};
        popup_menu(bx,it,4);
    } else if (which==1){ /* Edit */
        const char *it[]={"Copy","Paste"};
        popup_menu(bx,it,2);
    }
    redraw();
}

/* ---- composer actions ----------------------------------------------------- */
static void do_send(void)
{
    if (!ilen) return;
    input[ilen]=0;
    ws_say(input);
    scopy(g_activity,"SWIMMING, SWIMMING",sizeof g_activity);
    ilen=0; input[0]=0;
}

/* ---- login screen --------------------------------------------------------- */
/* Returns 1 on success (connected + authed), 0 if the user closed the window. */
static char li_user[24], li_pass[24];
static int  li_field;                 /* 0=user 1=pass */

static void draw_login(const char *err)
{
    gfx_target(&win.surf);
    gfx_fill(0,0,WINW,WINH,1);
    const char *brand="IKANET";
    int bw=gfx_text_width(brand);
    gfx_text((WINW-bw)/2-10, 70, brand, 0);
    gfx_fill_circle((WINW+bw)/2-4, 73, 4, 1);     /* squid dot */

    int fx=140, fw=200;
    gfx_text(fx-56,110,"User",0);
    gfx_frame(fx,106,fx+fw,122);
    gfx_text(fx+4,110,li_user,0);
    if (li_field==0) gfx_vline(fx+4+gfx_text_width(li_user)+1,108,120,1);

    gfx_text(fx-56,140,"Pass",0);
    gfx_frame(fx,136,fx+fw,152);
    char stars[24]; int pl=strlen(li_pass); for(int i=0;i<pl&&i<23;i++)stars[i]='*'; stars[pl<23?pl:23]=0;
    gfx_text(fx+4,140,stars,0);
    if (li_field==1) gfx_vline(fx+4+gfx_text_width(stars)+1,138,150,1);

    /* your 16x16 icon preview + edit (for new accounts) */
    gfx_text(fx-56,166,"Icon",0);
    draw_icon(g_icon[0]?g_icon:NULL, fx, 160, 16);
    draw_button(fx+26,160,fx+110,178,"Edit Icon",0);

    draw_button(120,190,230,210,"JOIN IKANET", 0);
    draw_button(250,190,330,210,"LOGIN", 0);

    if (err && err[0]){ int w=gfx_text_width(err); gfx_text((WINW-w)/2,216,err,0); }
    gfx_frame(0,0,WINW,WINH);
    if (g_dark) gfx_invert(0,0,WINW,WINH);
    gwin_update(&win);
}

/* mode: "login" or "join". Sends the WS auth frame, waits for welcome/error. */
static int try_auth(const char *mode)
{
    if (!ws_connected){ draw_login("Not connected"); return 0; }
    g_connected=0;
    char eu[48], ep[48]; json_esc(li_user,eu,sizeof eu); json_esc(li_pass,ep,sizeof ep);
    char b[400];
    /* on join, include the user's chosen 16x16 icon so it's set from the start */
    if (!strcmp(mode,"join") && g_icon[0])
        snprintf(b,sizeof b,"{\"t\":\"auth\",\"mode\":\"join\",\"user\":\"%s\",\"pass\":\"%s\",\"icon\":\"%s\"}",eu,ep,g_icon);
    else
        snprintf(b,sizeof b,"{\"t\":\"auth\",\"mode\":\"%s\",\"user\":\"%s\",\"pass\":\"%s\"}",mode,eu,ep);
    if (ws_send_text(b) < 0){ draw_login("Send failed"); return 0; }

    uint32_t end = plusos->ticks()+300;      /* ~5s */
    char lasterr[80]="";
    while (plusos->ticks()<end){
        int r = ws_poll_message();
        if (r < 0) { draw_login("Disconnected"); return 0; }
        if (r == 1){
            char t[24]; json_type(ws_msg,t,sizeof t);
            if (!strcmp(t,"welcome")){ handle_json(ws_msg); scopy(g_user,li_user,sizeof g_user); return 1; }
            if (!strcmp(t,"error")){ json_str(ws_msg,"msg",lasterr,sizeof lasterr); break; }
        }
    }
    draw_login(lasterr[0]?lasterr:"No response");
    return 0;
}

static int login_screen(void)
{
    li_field=0; draw_login("");
    for(;;){
        GEvent e;
        int r=gwin_event(&win,&e,1);
        if (r<0 || (r>0 && e.type==GEV_CLOSE)) return 0;
        if (r>0 && e.type==GEV_MOUSEDOWN){
            int x=e.x,y=e.y;
            if (in_rect(x,y,140,106,340,122)) li_field=0;
            else if (in_rect(x,y,140,136,340,152)) li_field=1;
            else if (in_rect(x,y,166,160,250,178)){ icon_editor(); }        /* Edit Icon */
            else if (in_rect(x,y,120,190,230,210)){ if (try_auth("join")) return 1; }
            else if (in_rect(x,y,250,190,330,210)){ if (try_auth("login")) return 1; }
            draw_login("");
        }
        if (r>0 && e.type==GEV_KEY){
            int k=e.key; char *f = li_field==0?li_user:li_pass; int fl=strlen(f);
            if (k==KEY_TAB) li_field^=1;
            else if (k==KEY_RETURN||k==KEY_ENTER){ if (try_auth("login")) return 1; }
            else if (k==KEY_BACKSPACE||k==KEY_DELETE){ if(fl) f[fl-1]=0; }
            else if (k>=32&&k<127&&fl<22){ f[fl]=(char)k; f[fl+1]=0; }
            draw_login("");
        }
        net_poll();
    }
}

/* ---- main ----------------------------------------------------------------- */
int main(int argc,char**argv)
{
    const char *host = argc>1 ? argv[1] : "ikanet.d0a.net";
    int port = argc>2 ? atoi(argv[2]) : 80;        /* default ws port behind Cloudflare: 80->443 */
    if (argc>3) scopy(ws_path, argv[3], sizeof ws_path);
    scopy(ws_host, host, sizeof ws_host);

    if (gwin_open(&win,"IKANET",WINW,WINH)) { printf("ikanet: cannot open window\n"); return 1; }
    origin_x = 0; origin_y = 0;
    default_icon(g_icon);                 /* new users start with the squid icon */

    /* connect: PlusNET up -> resolve -> TCP -> WebSocket handshake */
    if (net_init()){
        push_msg("","PlusNET not available (needs a DaynaPORT adapter)",1);
    } else {
        uint32_t ip;
        if (net_resolve(host,&ip)){ push_msg("","cannot resolve host",1); }
        else if (net_tcp_connect(ip,(uint16_t)port, 60*15)){ push_msg("","connection failed",1); }
        else if (ws_handshake()){ push_msg("","WebSocket handshake failed",1); }
        else { ws_connected = 1; }
    }

    if (!login_screen()){ gwin_close(&win); return 0; }

    scopy(g_activity,"",sizeof g_activity);
    redraw();

    uint32_t act_clear=0;
    for(;;){
        /* network */
        if (ws_connected) pump_net();

        /* ping meter */
        if (ws_connected) ping_tick();

        GEvent e;
        int r=gwin_event(&win,&e,0);           /* non-blocking */
        if (r<0 || (r>0 && e.type==GEV_CLOSE)) break;
        if (r>0 && e.type==GEV_MOUSEDOWN){
            int x=e.x,y=e.y;
            if (y<MENU_B){ menu_click(x); }
            else if (in_rect(x,y,20,TITLE_T,SBX-120,TITLE_B)){ g_online=!g_online; }
            else if (in_rect(x,y,SEND_X,COMP_T,WINW,COMP_T+COMP_H)) do_send();
            else if (in_rect(x,y,SBX,MSG_T,WINW,MSG_T+SBW)){ if(scroll>0)scroll--; }
            else if (in_rect(x,y,SBX,MSG_B-SBW,WINW,MSG_B)){ if(scroll<nmsg-1)scroll++; }
            redraw();
        }
        if (r>0 && e.type==GEV_KEY){
            int k=e.key;
            if (k==KEY_FKEY || ((k=='q'||k=='Q')&&cmd_down())) break;
            else if (k==KEY_RETURN||k==KEY_ENTER) do_send();
            else if (k==KEY_BACKSPACE||k==KEY_DELETE){ if(ilen) input[--ilen]=0; }
            else if (k>=32&&k<127&&ilen<(int)sizeof input-1){ input[ilen++]=(char)k; input[ilen]=0; }
            redraw();
        }

        /* clear the activity text a moment after sending */
        if (g_activity[0]){
            if(!act_clear) act_clear=plusos->ticks()+40;
            else if(plusos->ticks()>act_clear){ g_activity[0]=0; act_clear=0; redraw(); }
        }

        if (r==0) { redraw(); plusos->sleep_ticks(2); }
    }

    do_logout();
    net_tcp_close();
    gwin_close(&win);
    return 0;
}
