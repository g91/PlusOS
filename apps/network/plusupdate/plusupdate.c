/* plusupdate - the PlusOS Software Update client (native 68k app).
 *
 * A 1-bit Macintosh application that connects over TCP/IP (PlusNET) to the OTA
 * update bridge and shows the user which updates are available, an AI-written
 * message about each one, the items it contains, and a chat view to ask the
 * Update Assistant questions. It mirrors the IKANET client's look and its use
 * of net.h + a newline/0x01 line protocol (PlusOS has no HTTP/WS stack).
 *
 *   +-------------------------------------------------------------+
 *   | * PlusOS Software Update                         [Chat][x]  | title
 *   +-------------------------------------------------------------+
 *   | > stable 2026.11.14-1  "Desktop speedups"   1.2M  4 items   | list
 *   |   beta   2026.11.10-3  "Net fixes"          0.8M  2 items   |
 *   +-------------------------------------------------------------+
 *   | The Update Assistant says:                                  | detail
 *   |   This update speeds up the LC III desktop ...              |
 *   |   - kernel.pex   kernel   18 KB                             |
 *   |   - SHELL        app       9 KB                             |
 *   +-------------------------------------------------------------+
 *   | [ Refresh ]                    [ Apply on next sync ]       | actions
 *   +-------------------------------------------------------------+
 *
 * Applying updates to the running volume from inside PlusOS is not yet possible
 * (no self-writing of the boot volume); this app marks a release as "wanted" so
 * the host-side Updater applies it on the next sync. The real value here is the
 * read-only catalog + AI messages + chat, which work the moment the bridge is
 * reachable.
 *
 * Protocol (see otaserver/ota_bridge.py):
 *   -> HELLO\x01machine\x01channel      <- HI\x01greeting
 *   -> LIST                              <- REL\x01... (xN) then END
 *   -> INFO\x01id                        <- MSG\x01text, ITEM\x01... (xN), END
 *   -> CHAT\x01text                      <- AI\x01reply
 *   -> PING                              <- PONG
 * Newlines inside a field arrive as 0x02.
 *
 * Usage:  plusupdate [SERVER] [PORT] [CHANNEL]   (defaults: ota.local 8990 stable)
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusnet/net.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- window geometry ------------------------------------------------------ */
#define WINW     520
#define WINH     340
#define MENU_B   16
#define TITLE_B  (MENU_B + 18)
#define LIST_T   TITLE_B
#define LISTROWH 13
#define NLISTVIS 7
#define LIST_B   (LIST_T + NLISTVIS*LISTROWH + 2)
#define DET_T    LIST_B
#define ACT_H    20
#define DET_B    (WINH - ACT_H)
#define ACT_T    DET_B
#define DETLINEH 11
#define NDETVIS  ((DET_B - DET_T - 4) / DETLINEH)

/* ---- state ---------------------------------------------------------------- */
static GWindow win;
static int g_dark = 1;
static int g_connected;
static char g_server[64] = "ota.local";
static int  g_port = 8990;
static char g_channel[16] = "stable";
static char g_machine[12] = "";          /* filled from the OS if available */

/* release list */
#define NREL 60
typedef struct {
    int  id;
    char channel[12];
    char version[24];
    char machine[12];
    char title[48];
    long bytes;
    int  nitems;
    char isnew;
    char wanted;                          /* user marked for next sync */
} Rel;
static Rel  rels[NREL];
static int  nrel, listsel, listscroll;

/* detail (AI message + items) for the selected release */
#define NDET 80
static char det[NDET][96];
static int  ndet, detscroll;
static int  det_for = -1;                 /* which release id det[] holds */

/* chat */
#define NCHAT 60
static char chat[NCHAT][96];
static int  nchat, chatscroll;
static char chatin[160];
static int  chatlen;
static int  view_chat;                    /* 0 = catalog view, 1 = chat view */

/* network rx assembly */
static char rxline[2048];
static int  rxlen;

/* ---- helpers -------------------------------------------------------------- */
static void scopy(char*d,const char*s,int n){int i=0;if(!s)s="";for(;s[i]&&i<n-1;i++)d[i]=s[i];d[i]=0;}
static int in_rect(int x,int y,int l,int t,int r,int b){return x>=l&&x<r&&y>=t&&y<b;}
static int cmd_down(void){ return (plusos->key_mods() & 0x0100) != 0; }

static void fmt_size(long b, char *out, int n)
{
    if (b < 1024) snprintf(out,n,"%ld B", b);
    else if (b < 1024L*1024) snprintf(out,n,"%ld KB", b/1024);
    else snprintf(out,n,"%ld.%ld MB", b/(1024L*1024), (b%(1024L*1024))/(105L*1024));
}

/* undo the 0x02-for-newline encoding in place, returning the first line only
 * when we want a single row, or keeping 0x02 to split later. */
static void decode_nl(char *s){ for(char*p=s;*p;p++) if(*p=='\x02') *p='\n'; }

/* ---- drawing -------------------------------------------------------------- */
static void draw_button(int l,int t,int r,int b,const char*s,int pressed)
{
    gfx_fill_round_rect(l,t,r,b,4,0);
    gfx_fill_round_rect(l+1,t+1,r-1,b-1,3,1);
    int tw=gfx_text_width(s), tx=l+((r-l)-tw)/2, ty=t+((b-t)-8)/2;
    gfx_text(tx,ty,s,0);
    if (pressed) gfx_invert(l,t,r,b);
}

static void draw_menubar(void)
{
    gfx_fill(0,0,WINW,MENU_B,1);
    gfx_hline(0,WINW,MENU_B-1,1);
    gfx_fill_circle(7,7,3,1);
    gfx_text(16,4," File",0);
    gfx_text(70,4," View",0);
    gfx_text(130,4," DarkMode",0);
}

static void draw_title(void)
{
    gfx_hline(0,WINW,TITLE_B-1,1);
    gfx_text(6,MENU_B+5,"PlusOS Software Update",0);
    const char *st = g_connected ? "online" : "offline";
    int pw=gfx_text_width(st)+10;
    gfx_round_rect(WINW-pw-110,MENU_B+3,WINW-110,TITLE_B-3,6,1);
    gfx_text(WINW-pw-105,MENU_B+5,st,0);
    draw_button(WINW-96,MENU_B+2,WINW-44,TITLE_B-2, view_chat?"Catalog":"Chat", 0);
    draw_button(WINW-40,MENU_B+2,WINW-6,TITLE_B-2,"Quit",0);
}

static void draw_list(void)
{
    gfx_fill(0,LIST_T,WINW,LIST_B,1);
    int y=LIST_T+1, shown=0;
    for (int i=listscroll; i<nrel && shown<NLISTVIS; i++,shown++,y+=LISTROWH){
        Rel *r=&rels[i];
        if (i==listsel) gfx_fill(0,y,WINW,y+LISTROWH,1), gfx_invert(0,y,WINW,y+LISTROWH);
        char sz[16]; fmt_size(r->bytes,sz,sizeof sz);
        char line[120];
        snprintf(line,sizeof line,"%s %-10s %-22s %6s  %d items%s",
                 r->wanted?"*":(r->isnew?">":" "),
                 r->channel, r->version, sz, r->nitems,
                 r->title[0]?"":"");
        gfx_text(4,y+2,line,0);
        if (r->title[0]){
            int tx=4+gfx_text_width(line)+6;
            char q[56]; snprintf(q,sizeof q,"\"%s\"",r->title);
            if (tx < WINW-20) gfx_text(tx,y+2,q,0);
        }
    }
    gfx_hline(0,WINW,LIST_B-1,1);
}

static void draw_detail(void)
{
    gfx_fill(0,DET_T,WINW,DET_B,1);
    int y=DET_T+2, shown=0;
    char **src; int n, *scroll;
    if (view_chat){ n=nchat; scroll=&chatscroll; }
    else          { n=ndet;  scroll=&detscroll; }
    for (int i=*scroll; i<n && shown<NDETVIS; i++,shown++,y+=DETLINEH){
        const char *ln = view_chat ? chat[i] : det[i];
        /* clip to width */
        char buf[100]; scopy(buf,ln,sizeof buf);
        int maxpx=WINW-10;
        while (gfx_text_width(buf)>maxpx && buf[0]) buf[strlen(buf)-1]=0;
        gfx_text(5,y,buf,0);
    }
    if (view_chat){
        /* input line */
        gfx_hline(0,WINW,DET_B-14,1);
        char show[160]; scopy(show,chatin,sizeof show);
        int tx=5, maxpx=WINW-60;
        while (gfx_text_width(show)>maxpx && show[0]) memmove(show,show+1,strlen(show));
        int ex=gfx_text(tx,DET_B-12,show,0);
        gfx_vline(ex+1,DET_B-13,DET_B-3,1);
        gfx_text(WINW-46,DET_B-12,"[SEND]",0);
    }
}

static void draw_actions(void)
{
    gfx_hline(0,WINW,ACT_T,1);
    draw_button(6,ACT_T+2,76,ACT_T+ACT_H-2,"Refresh",0);
    if (!view_chat)
        draw_button(WINW-150,ACT_T+2,WINW-6,ACT_T+ACT_H-2,"Want on next sync",0);
}

static void redraw(void)
{
    gfx_target(&win.surf);
    gfx_fill(0,0,WINW,WINH,1);
    draw_menubar();
    draw_title();
    if (view_chat){ draw_detail(); }
    else { draw_list(); draw_detail(); }
    draw_actions();
    gfx_frame(0,0,WINW,WINH);
    if (g_dark) gfx_invert(0,0,WINW,WINH);
    gwin_update(&win);
}

/* ---- log helpers ---------------------------------------------------------- */
static void det_clear(void){ ndet=0; detscroll=0; }
static void det_add(const char *s){
    if (ndet>=NDET){ memmove(det,det[1],sizeof(det[0])*(NDET-1)); ndet=NDET-1; }
    scopy(det[ndet++],s,sizeof det[0]);
    if (ndet>NDETVIS) detscroll=ndet-NDETVIS;
}
static void chat_add(const char *s){
    if (nchat>=NCHAT){ memmove(chat,chat[1],sizeof(chat[0])*(NCHAT-1)); nchat=NCHAT-1; }
    scopy(chat[nchat++],s,sizeof chat[0]);
    if (nchat>NDETVIS) chatscroll=nchat-NDETVIS;
}
/* add a multi-line string (0x02-delimited) as several wrapped rows */
static void add_wrapped(void (*add)(const char*), const char *prefix, char *text)
{
    decode_nl(text);
    char *line = text;
    int first=1;
    for (char *p=text;; p++){
        if (*p=='\n' || *p==0){
            char save=*p; *p=0;
            char row[96];
            if (first && prefix) snprintf(row,sizeof row,"%s%s",prefix,line);
            else snprintf(row,sizeof row,"%s",line);
            first=0;
            /* word-wrap at ~ width */
            char *s=row;
            while (*s){
                char chunk[96]; scopy(chunk,s,sizeof chunk);
                while (gfx_text_width(chunk)>WINW-12 && chunk[0]) chunk[strlen(chunk)-1]=0;
                add(chunk);
                s += strlen(chunk);
                if (!*s) break;
            }
            if (save==0) break;
            line=p+1;
        }
    }
}

/* ---- protocol ------------------------------------------------------------- */
static int send_line(const char *s)
{
    char buf[700]; int n=snprintf(buf,sizeof buf,"%s\n",s);
    if (n>=(int)sizeof buf) n=(int)sizeof buf-1;
    return net_tcp_send(buf,n);
}
static int send2(const char*c,const char*a){char b[600];snprintf(b,sizeof b,"%s\x01%s",c,a);return send_line(b);}
static int send3(const char*c,const char*a,const char*d){char b[600];snprintf(b,sizeof b,"%s\x01%s\x01%s",c,a,d);return send_line(b);}

static void req_list(void){ nrel=0; listsel=0; listscroll=0; send_line("LIST"); }
static void req_info(int rid){ char b[16]; snprintf(b,sizeof b,"%d",rid); det_clear(); det_for=rid; send2("INFO",b); }

static void handle_line(char *line)
{
    char *f[8]={line}; int nf=1;
    for (char *p=line; *p && nf<8; p++) if (*p=='\x01'){ *p=0; f[nf++]=p+1; }
    const char *cmd=f[0];

    if (!strcmp(cmd,"HI")){ g_connected=1; det_clear(); if(f[1]){char t[96];scopy(t,f[1],sizeof t);decode_nl(t);det_add(t);} req_list(); }
    else if (!strcmp(cmd,"PONG")){ /* keepalive */ }
    else if (!strcmp(cmd,"ERR")){ det_add("! "); det_add(f[1]?f[1]:"error"); }
    else if (!strcmp(cmd,"REL")){
        if (nrel<NREL){
            Rel *r=&rels[nrel++];
            r->id = f[1]?atoi(f[1]):0;
            scopy(r->channel, f[2], sizeof r->channel);
            scopy(r->version, f[3], sizeof r->version);
            scopy(r->machine, f[4], sizeof r->machine);
            scopy(r->title,   f[5], sizeof r->title);
            r->bytes  = f[6]?atol(f[6]):0;
            r->nitems = f[7]?atoi(f[7]):0;
            r->isnew  = f[8]?atoi(f[8]):0;
            r->wanted = 0;
        }
    }
    else if (!strcmp(cmd,"END")){ /* list / info finished */ }
    else if (!strcmp(cmd,"MSG")){
        det_clear();
        det_add("The Update Assistant says:");
        char t[400]; scopy(t,f[1]?f[1]:"",sizeof t);
        add_wrapped(det_add,"  ",t);
        det_add("");
        det_add("Contents:");
    }
    else if (!strcmp(cmd,"ITEM")){
        char sz[16]; fmt_size(f[3]?atol(f[3]):0,sz,sizeof sz);
        char row[96]; snprintf(row,sizeof row,"  - %-14s %-8s %s",f[1]?f[1]:"?",f[2]?f[2]:"",sz);
        det_add(row);
    }
    else if (!strcmp(cmd,"AI")){
        char t[600]; scopy(t,f[1]?f[1]:"",sizeof t);
        add_wrapped(chat_add,"AI: ",t);
    }
}

static void pump_net(void)
{
    net_poll();
    static uint8_t buf[1024];
    int n=net_tcp_recv(buf,sizeof buf);
    if (n<0){ g_connected=0; det_add("[disconnected]"); return; }
    for (int i=0;i<n;i++){
        char c=(char)buf[i];
        if (c=='\n'){ rxline[rxlen]=0; if(rxlen) handle_line(rxline); rxlen=0; }
        else if (c!='\r' && rxlen<(int)sizeof rxline-1) rxline[rxlen++]=c;
    }
}

/* ---- ping keepalive ------------------------------------------------------- */
static uint32_t last_ping;
static void ping_tick(void){
    uint32_t t=plusos->ticks();
    if (t-last_ping>900){ send_line("PING"); last_ping=t; }
}

/* ---- chat send ------------------------------------------------------------ */
static void chat_send(void)
{
    if (!chatlen) return;
    chatin[chatlen]=0;
    char me[96]; snprintf(me,sizeof me,"You: %s",chatin);
    chat_add(me);
    send2("CHAT",chatin);
    chatlen=0; chatin[0]=0;
}

/* ---- menu ----------------------------------------------------------------- */
static void menu_click(int mx)
{
    if (mx<16) return;
    if (mx>=16 && mx<66){ /* File: just Quit for now */ }
    else if (mx>=70 && mx<128){ view_chat=!view_chat; }
    else if (mx>=130){ g_dark=!g_dark; }
    redraw();
}

/* ---- main ----------------------------------------------------------------- */
int main(int argc,char**argv)
{
    if (argc>1) scopy(g_server,argv[1],sizeof g_server);
    if (argc>2) g_port=atoi(argv[2]);
    if (argc>3) scopy(g_channel,argv[3],sizeof g_channel);

    /* best-effort machine tag from the OS (falls back to empty = any) */
#ifdef PLUSOS_MACHINE_SE
    scopy(g_machine,"se",sizeof g_machine);
#elif defined(PLUSOS_MACHINE_PLUS)
    scopy(g_machine,"plus",sizeof g_machine);
#elif defined(PLUSOS_MACHINE_LC3)
    scopy(g_machine,"lc3",sizeof g_machine);
#elif defined(PLUSOS_MACHINE_CLASSIC)
    scopy(g_machine,"classic",sizeof g_machine);
#endif

    if (gwin_open(&win,"PlusOS Software Update",WINW,WINH)){ printf("plusupdate: no window\n"); return 1; }

    det_add("Connecting to the PlusOS update service...");
    redraw();

    if (net_init()){ det_add("PlusNET not available (needs a BlueSCSI DaynaPORT)."); redraw(); }
    else {
        uint32_t ip;
        if (!net_resolve(g_server,&ip) && !net_tcp_connect(ip,(uint16_t)g_port,60*15)){
            send3("HELLO",g_machine,g_channel);
        } else {
            det_add("Could not reach the update server."); g_connected=0;
        }
    }

    for(;;){
        pump_net();
        ping_tick();

        GEvent e;
        int r=gwin_event(&win,&e,0);
        if (r<0 || (r>0 && e.type==GEV_CLOSE)) break;

        if (r>0 && e.type==GEV_MOUSEDOWN){
            int x=e.x,y=e.y;
            if (y<MENU_B){ menu_click(x); }
            else if (in_rect(x,y,WINW-96,MENU_B+2,WINW-44,TITLE_B-2)){ view_chat=!view_chat; redraw(); }
            else if (in_rect(x,y,WINW-40,MENU_B+2,WINW-6,TITLE_B-2)){ break; }        /* Quit */
            else if (in_rect(x,y,6,ACT_T+2,76,ACT_T+ACT_H-2)){                        /* Refresh */
                if (g_connected) req_list(); redraw();
            }
            else if (!view_chat && in_rect(x,y,WINW-150,ACT_T+2,WINW-6,ACT_T+ACT_H-2)){ /* Want */
                if (listsel>=0 && listsel<nrel){ rels[listsel].wanted=1; }
                redraw();
            }
            else if (!view_chat && in_rect(x,y,0,LIST_T,WINW,LIST_B)){               /* list select */
                int row=(y-LIST_T)/LISTROWH + listscroll;
                if (row>=0 && row<nrel){ listsel=row; if(g_connected) req_info(rels[row].id); }
                redraw();
            }
            else if (view_chat && in_rect(x,y,WINW-50,DET_B-14,WINW,DET_B)){          /* SEND */
                chat_send(); redraw();
            }
        }
        if (r>0 && e.type==GEV_KEY){
            int k=e.key;
            if (k==KEY_FKEY || ((k=='q'||k=='Q')&&cmd_down())) break;
            if (view_chat){
                if (k==KEY_RETURN||k==KEY_ENTER) chat_send();
                else if (k==KEY_BACKSPACE||k==KEY_DELETE){ if(chatlen) chatin[--chatlen]=0; }
                else if (k>=32&&k<127&&chatlen<(int)sizeof chatin-1){ chatin[chatlen++]=(char)k; chatin[chatlen]=0; }
            } else {
                if (k==KEY_UP && listsel>0){ listsel--; if(listsel<listscroll)listscroll=listsel; if(g_connected)req_info(rels[listsel].id); }
                else if (k==KEY_DOWN && listsel<nrel-1){ listsel++; if(listsel>=listscroll+NLISTVIS)listscroll=listsel-NLISTVIS+1; if(g_connected)req_info(rels[listsel].id); }
            }
            redraw();
        }

        if (r==0){ redraw(); plusos->sleep_ticks(2); }
    }

    send_line("QUIT");
    net_tcp_close();
    gwin_close(&win);
    return 0;
}
