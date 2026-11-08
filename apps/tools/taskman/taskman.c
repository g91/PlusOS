/* taskman - the PlusOS task manager.
 *
 * On the desktop it is a graphical window: a live list of processes with a CPU
 * bar for each, memory and state, a selection, and an "End Process" button.
 * Run from the shell (no desktop) it falls back to a plain text listing.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/thread.h>

static const char *st[] = { "-", "ready", "run", "block", "zombie" };

/* Is a desktop compositor actually running right now? We look for a live
 * "desktop" process rather than trusting a persistent registry flag, which
 * goes stale if the desktop ever exits uncleanly - a stale flag would send us
 * into GUI mode with no compositor and hang the machine. */
static int under_desktop(void)
{
    struct plusos_procinfo p[16];
    struct plusos_proclist_args a = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    for (int i = 0; i < n; i++)
        if (p[i].state != 4 && !strncmp(p[i].name, "desktop", 7)) return 1;   /* not a zombie */
    return 0;
}

static int get_list(struct plusos_procinfo *p, int max, struct plusos_sysinfo *si)
{
    struct plusos_proclist_args a = { p, max };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    plusos_syscall(SYS_sysinfo, si);
    return n;
}

/* ---- text mode (from the shell) ---- */
static void text_mode(void)
{
    char line[16];
    for (;;) {
        struct plusos_procinfo p[16]; struct plusos_sysinfo si;
        int n = get_list(p, 16, &si);
        unsigned long up = si.ticks ? si.ticks : 1;
        con_clear();
        printf("PlusOS Task Manager   %s  up %lus\n", si.machine, up / 60);
        printf("%luK RAM, %luK free   %d processes\n\n", si.ram_bytes >> 10, si.free_bytes >> 10, n);
        printf("PID PPID TYPE  STATE  CPU%%  MEM  NAME\n");
        for (int i = 0; i < n; i++)
            printf("%3d %4d %-4s  %-6s %3lu%% %3luK  %s\n", p[i].pid, p[i].parent,
                   p[i].ring == 0 ? "sys" : "user", st[p[i].state < 5 ? p[i].state : 0],
                   p[i].cpu_ticks * 100 / up, p[i].mem_bytes >> 10, p[i].name);
        printf("\n[k] end a process   [q] quit");
        int k = 0;
        for (int t = 0; t < 60 && !(k = plusos->key_poll()); t++) plusos_syscall(SYS_sleep, (void *)1);
        if (k == 'q' || k == 'Q' || k == 0x1B) break;
        if (k == 'k' || k == 'K') {
            printf("\npid to end: ");
            plusos->key_line(line, sizeof line);
            int pid = atoi(line);
            if (pid > 0) plusos_syscall(SYS_kill, (void *)(intptr_t)pid);
        }
    }
    con_clear();
}

/* ---- GUI mode (on the desktop) ---- */
#define WINW 340
#define WINH 236
#define ROWH 14
#define LIST_TOP 54
#define BTN_T (WINH - 26)

static GWindow win;
static struct plusos_procinfo procs[16];
static struct plusos_sysinfo si;
static int nproc, sel_pid = -1;

static void draw(void)
{
    int n = get_list(procs, 16, &si);
    nproc = n;
    unsigned long up = si.ticks ? si.ticks : 1;
    int W = win.surf.w, H = win.surf.h;
    gfx_target(&win.surf);
    gfx_fill(0, 0, W, H, 0);
    /* header */
    char s[56];
    snprintf(s, sizeof s, "%s   %luK RAM, %luK free", si.machine, si.ram_bytes >> 10, si.free_bytes >> 10);
    gfx_text(8, 6, s, 0);
    snprintf(s, sizeof s, "%d processes    up %lu:%02lu:%02lu", n, up / 3600, (up / 60) % 60, up % 60);
    gfx_text(8, 18, s, 0);
    gfx_text(8, 36, "NAME", 0);
    gfx_text(150, 36, "CPU", 0);
    gfx_text(250, 36, "MEM", 0);
    gfx_text(298, 36, "STATE", 0);
    gfx_hline(0, W, 48, 1);
    int sel_row = -1;
    for (int i = 0; i < n && i < (BTN_T - LIST_TOP) / ROWH; i++) {
        struct plusos_procinfo *p = &procs[i];
        int y = LIST_TOP + i * ROWH;
        int sel = p->pid == sel_pid;
        if (sel) { gfx_fill(2, y - 1, W - 2, y + ROWH - 2, 1); sel_row = i; }
        char nm[18]; int j = 0; for (; p->name[j] && j < 16; j++) nm[j] = p->name[j]; nm[j] = 0;
        gfx_text(8, y, nm, sel);
        /* CPU bar */
        unsigned long cpu = p->cpu_ticks * 100 / up;
        int bx = 150, bw = 74;
        gfx_frame(bx, y, bx + bw, y + 9);
        int fill = (int)(cpu * (bw - 2) / 100); if (fill > bw - 2) fill = bw - 2;
        if (sel) { gfx_fill(bx, y, bx + bw, y + 9, 0); gfx_frame(bx, y, bx + bw, y + 9); }
        gfx_fill(bx + 1, y + 1, bx + 1 + fill, y + 8, 1);
        snprintf(s, sizeof s, "%luK", p->mem_bytes >> 10);
        gfx_text(250, y, s, sel);
        gfx_text(298, y, st[p->state < 5 ? p->state : 0], sel);
    }
    (void)sel_row;
    /* buttons */
    gfx_hline(0, W, BTN_T - 4, 1);
    int enabled = sel_pid > 0;
    gfx_frame(W - 120, BTN_T, W - 8, BTN_T + 18);
    gfx_text(W - 112, BTN_T + 5, "End Process", 0);
    if (!enabled) gfx_dim(W - 119, BTN_T + 1, W - 9, BTN_T + 17);
    gwin_update(&win);
}

static void gui_mode(void)
{
    if (gwin_open(&win, "Task Manager", WINW, WINH) != 0) { text_mode(); return; }
    draw();
    uint32_t last = plusos->ticks();
    for (;;) {
        GEvent e;
        int r = gwin_event(&win, &e, 0);
        if (r < 0) break;
        if (r > 0) {
            if (e.type == GEV_CLOSE) break;
            if (e.type == GEV_MOUSEDOWN) {
                int W = win.surf.w;
                if (e.y >= BTN_T && e.x >= W - 120 && sel_pid > 0) {
                    plusos_syscall(SYS_kill, (void *)(intptr_t)sel_pid);
                    sel_pid = -1;
                    plusos_syscall(SYS_sleep, (void *)6);
                    draw();
                } else if (e.y >= LIST_TOP && e.y < BTN_T - 4) {
                    int i = (e.y - LIST_TOP) / ROWH;
                    if (i >= 0 && i < nproc) { sel_pid = procs[i].pid; draw(); }
                }
            }
            continue;
        }
        uint32_t t = plusos->ticks();
        if (t - last >= 45) { last = t; draw(); }     /* live refresh ~1.3x/sec */
        gthread_sleep(3);
    }
    gwin_close(&win);
}

int main(void)
{
    if (under_desktop()) gui_mode();
    else text_mode();
    return 0;
}
