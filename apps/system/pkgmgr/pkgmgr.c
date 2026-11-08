/* pkgmgr - PlusOS Software Center & Package Manager (GUI).
 *
 * A graphical package management suite for the PlusOS Desktop:
 *   - Browse all official PlusOS packages and applications
 *   - Filter by categories: All, Games, Development, Network, Tools
 *   - View package details, version, disk footprint, and description
 *   - One-click Install / Download over DaynaPORT HTTP connection
 *   - One-click Launch and Uninstall / Remove
 *   - Offline cache of package catalog (PlusOS Packages file)
 */

#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/thread.h>
#include <plusnet/net.h>
#include <plusrom/FileMgr.h>

#define WINW 410
#define WINH 285

#define CAT_TOP 26
#define CAT_BOT 44

#define LIST_TOP 50
#define LIST_BOT 192
#define LIST_LEFT 10
#define LIST_RIGHT 250
#define ROWH 13
#define VISROWS ((LIST_BOT - LIST_TOP) / ROWH)

#define DETAIL_LEFT 258
#define DETAIL_RIGHT (WINW - 10)
#define DETAIL_TOP 50
#define DETAIL_BOT 240

#define STATUS_TOP (WINH - 22)
#define STATUS_BOT (WINH - 4)

#define MAX_PKGS 128
#define DEFAULT_REPO "plusos.d0a.net"
#define DEFAULT_PORT 8088

typedef struct {
    char name[24];
    char version[12];
    char category[16];
    uint32_t size;
    char desc[64];
    int installed;
} PkgEntry;

static GWindow win;
static PkgEntry pkgs[MAX_PKGS];
static int npkgs = 0;
static int filtered_indices[MAX_PKGS];
static int nfiltered = 0;
static int cur_cat = 0; /* 0=All, 1=Games, 2=Dev, 3=Net, 4=Tools */
static int sel = 0, scroll = 0;
static char status_msg[128] = "Ready. Select a package to install or launch.";

static const char *categories[] = { "All", "Games", "Dev", "Net", "Tools" };
#define NUM_CATS 5
static int cat_x0[NUM_CATS], cat_x1[NUM_CATS];

static const unsigned char cat_file[] = "\x0F" "PlusOS Packages";

static void c2p(unsigned char *p, const char *c)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

static void to_upper(char *d, const char *s)
{
    while (*s) {
        *d = (*s >= 'a' && *s <= 'z') ? (*s - 32) : *s;
        d++; s++;
    }
    *d = 0;
}

static int in_rect(int x, int y, int l, int t, int r, int b)
{
    return x >= l && x < r && y >= t && y < b;
}

static int is_pkg_installed(const char *name, const char *category)
{
    char upper[32], path[64];
    to_upper(upper, name);

    /* Check in :bin: first */
    snprintf(path, sizeof(path), ":bin:%s", upper);
    unsigned char p[64];
    c2p(p, path);
    INTEGER ref;
    if (FSOpen(p, 0, &ref) == 0) {
        FSClose(ref);
        return 1;
    }

    /* Check in :Games: */
    snprintf(path, sizeof(path), ":Games:%s", upper);
    c2p(p, path);
    if (FSOpen(p, 0, &ref) == 0) {
        FSClose(ref);
        return 1;
    }

    /* Check in :System 3.2: */
    snprintf(path, sizeof(path), ":System 3.2:%s", upper);
    c2p(p, path);
    if (FSOpen(p, 0, &ref) == 0) {
        FSClose(ref);
        return 1;
    }

    return 0;
}

static void refresh_installed_status(void)
{
    for (int i = 0; i < npkgs; i++) {
        pkgs[i].installed = is_pkg_installed(pkgs[i].name, pkgs[i].category);
    }
}

static void apply_filter(void)
{
    nfiltered = 0;
    int is_all = (cur_cat == 0);

    for (int i = 0; i < npkgs; i++) {
        int match = 0;
        if (is_all) {
            match = 1;
        } else if (cur_cat == 1 && (!strcmp(pkgs[i].category, "game") || !strcmp(pkgs[i].category, "games"))) {
            match = 1;
        } else if (cur_cat == 2 && (!strcmp(pkgs[i].category, "dev") || !strcmp(pkgs[i].category, "devel"))) {
            match = 1;
        } else if (cur_cat == 3 && (!strcmp(pkgs[i].category, "net") || !strcmp(pkgs[i].category, "network"))) {
            match = 1;
        } else if (cur_cat == 4 && (!strcmp(pkgs[i].category, "system") || !strcmp(pkgs[i].category, "tool") || !strcmp(pkgs[i].category, "tools"))) {
            match = 1;
        }

        if (match) {
            filtered_indices[nfiltered++] = i;
        }
    }

    if (sel >= nfiltered) sel = nfiltered > 0 ? nfiltered - 1 : 0;
    if (scroll > nfiltered - VISROWS) scroll = nfiltered - VISROWS;
    if (scroll < 0) scroll = 0;
}

static void load_cached_catalog(void)
{
    npkgs = 0;
    INTEGER ref;
    if (FSOpen(cat_file, 0, &ref) != 0) return;

    char buf[4096];
    LONGINT n = sizeof(buf) - 1;
    if (FSRead(ref, &n, buf) == 0 && n > 0) {
        buf[n] = 0;
        char *p = buf;
        while (*p && npkgs < MAX_PKGS) {
            char line[160];
            int li = 0;
            while (*p && *p != '\r' && *p != '\n' && li < (int)sizeof(line) - 1) line[li++] = *p++;
            line[li] = 0;
            while (*p == '\r' || *p == '\n') p++;
            if (!line[0] || line[0] == '#') continue;

            char *f1 = strchr(line, '|');
            if (!f1) continue;
            *f1++ = 0;
            char *f2 = strchr(f1, '|');
            if (!f2) continue;
            *f2++ = 0;
            char *f3 = strchr(f2, '|');
            if (!f3) continue;
            *f3++ = 0;
            char *f4 = strchr(f3, '|');
            if (!f4) continue;
            *f4++ = 0;

            snprintf(pkgs[npkgs].name, sizeof(pkgs[npkgs].name), "%s", line);
            snprintf(pkgs[npkgs].version, sizeof(pkgs[npkgs].version), "%s", f1);
            snprintf(pkgs[npkgs].category, sizeof(pkgs[npkgs].category), "%s", f2);
            pkgs[npkgs].size = (uint32_t)atoi(f3);
            snprintf(pkgs[npkgs].desc, sizeof(pkgs[npkgs].desc), "%s", f4);
            pkgs[npkgs].installed = 0;
            npkgs++;
        }
    }
    FSClose(ref);
    refresh_installed_status();
    apply_filter();
}

static void get_repo_host(char *host, int max, int *port)
{
    *port = DEFAULT_PORT;
    char reg_buf[64];
    memset(reg_buf, 0, sizeof(reg_buf));
    struct plusos_reg_args ra;
    memset(&ra, 0, sizeof(ra));
    ra.key = "pkg.repo";
    ra.vbuf = reg_buf;
    ra.vsize = sizeof(reg_buf);

    if (plusos_syscall(SYS_regget, &ra) > 0 && reg_buf[0]) {
        char *col = strchr(reg_buf, ':');
        if (col) {
            *col++ = 0;
            *port = atoi(col);
        }
        snprintf(host, max, "%s", reg_buf);
    } else {
        snprintf(host, max, "%s", DEFAULT_REPO);
    }
}

static int http_get(const char *host, int port, const char *path, uint8_t *out_buf, int max_size)
{
    uint32_t ip = 0;
    if (net_resolve(host, &ip) != 0) {
        return -1;
    }

    if (net_tcp_connect(ip, (uint16_t)port, 300) != 0) {
        return -2;
    }

    char req[256];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "User-Agent: PlusOS-PkgMgr/1.0 (m68k)\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    net_tcp_send((const uint8_t *)req, (int)strlen(req));

    int total_read = 0;
    int in_body = 0;
    uint8_t temp[512];
    uint32_t timeout = 600;

    while (timeout > 0) {
        net_poll();
        int rc = net_tcp_recv(temp, sizeof(temp));
        if (rc < 0) break;
        if (rc == 0) {
            plusos_syscall(SYS_sleep, (void *)1);
            timeout--;
            continue;
        }

        if (!in_body) {
            for (int i = 0; i < rc - 3; i++) {
                if (temp[i] == '\r' && temp[i+1] == '\n' && temp[i+2] == '\r' && temp[i+3] == '\n') {
                    in_body = 1;
                    int body_start = i + 4;
                    int body_len = rc - body_start;
                    if (body_len > 0 && total_read + body_len < max_size) {
                        memcpy(&out_buf[total_read], &temp[body_start], body_len);
                        total_read += body_len;
                    }
                    break;
                }
            }
        } else {
            if (total_read + rc < max_size) {
                memcpy(&out_buf[total_read], temp, rc);
                total_read += rc;
            }
        }
    }

    net_tcp_close();
    return total_read;
}

static void draw_ui(void)
{
    gfx_target(&win.surf);
    gfx_fill(0, 0, win.surf.w, win.surf.h, 0);

    /* Title Bar Area */
    gfx_fill(0, 0, WINW, 22, 1);
    gfx_text(10, 5, "PlusOS Software Center (Package Manager)", 0);

    /* Category Filter Buttons */
    int x = 10;
    for (int i = 0; i < NUM_CATS; i++) {
        int w = gfx_text_width(categories[i]) + 16;
        cat_x0[i] = x;
        cat_x1[i] = x + w;

        if (cur_cat == i) {
            gfx_fill(x, CAT_TOP, x + w, CAT_BOT, 1);
            gfx_frame(x, CAT_TOP, x + w, CAT_BOT);
            gfx_text(x + 8, CAT_TOP + 3, categories[i], 0);
        } else {
            gfx_frame(x, CAT_TOP, x + w, CAT_BOT);
            gfx_text(x + 8, CAT_TOP + 3, categories[i], 1);
        }
        x += w + 6;
    }

    /* Update Catalog button on top right */
    int btn_upd_x0 = WINW - 110, btn_upd_x1 = WINW - 10;
    gfx_frame(btn_upd_x0, CAT_TOP, btn_upd_x1, CAT_BOT);
    gfx_text(btn_upd_x0 + 8, CAT_TOP + 3, "Update Catalog", 1);

    /* Left Side: Package List Box */
    gfx_frame(LIST_LEFT, LIST_TOP, LIST_RIGHT, LIST_BOT);
    int y = LIST_TOP + 3;

    for (int i = scroll; i < nfiltered && i < scroll + VISROWS; i++) {
        int idx = filtered_indices[i];
        int ry = y + (i - scroll) * ROWH;

        if (i == sel) {
            gfx_fill(LIST_LEFT + 2, ry, LIST_RIGHT - 2, ry + ROWH, 1);
            gfx_text(LIST_LEFT + 4, ry + 2, pkgs[idx].name, 0);
            if (pkgs[idx].installed) {
                gfx_text(LIST_RIGHT - 55, ry + 2, "[INST]", 0);
            }
        } else {
            gfx_text(LIST_LEFT + 4, ry + 2, pkgs[idx].name, 1);
            if (pkgs[idx].installed) {
                gfx_text(LIST_RIGHT - 55, ry + 2, "[INST]", 1);
            }
        }
    }

    /* Right Side: Package Details Pane */
    gfx_frame(DETAIL_LEFT, DETAIL_TOP, DETAIL_RIGHT, DETAIL_BOT);

    if (nfiltered > 0 && sel >= 0 && sel < nfiltered) {
        int idx = filtered_indices[sel];
        PkgEntry *p = &pkgs[idx];

        char line[64];
        snprintf(line, sizeof(line), "Name: %s", p->name);
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 8, line, 1);

        snprintf(line, sizeof(line), "Version: %s", p->version);
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 22, line, 1);

        snprintf(line, sizeof(line), "Category: %s", p->category);
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 36, line, 1);

        snprintf(line, sizeof(line), "Size: %lu bytes (%lu KB)", (unsigned long)p->size, (unsigned long)(p->size >> 10));
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 50, line, 1);

        snprintf(line, sizeof(line), "Status: %s", p->installed ? "INSTALLED" : "Available in Repo");
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 66, line, 1);

        /* Description (multi-line wrap approx) */
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 84, "Description:", 1);
        char d1[32], d2[32];
        int dlen = (int)strlen(p->desc);
        if (dlen > 24) {
            memcpy(d1, p->desc, 24); d1[24] = 0;
            snprintf(d2, sizeof(d2), "%s", p->desc + 24);
        } else {
            snprintf(d1, sizeof(d1), "%s", p->desc);
            d2[0] = 0;
        }
        gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 98, d1, 1);
        if (d2[0]) gfx_text(DETAIL_LEFT + 8, DETAIL_TOP + 110, d2, 1);

        /* Action Buttons */
        if (!p->installed) {
            /* [ Install / Download ] */
            gfx_fill(DETAIL_LEFT + 8, DETAIL_BOT - 32, DETAIL_RIGHT - 8, DETAIL_BOT - 8, 1);
            gfx_frame(DETAIL_LEFT + 8, DETAIL_BOT - 32, DETAIL_RIGHT - 8, DETAIL_BOT - 8);
            gfx_text(DETAIL_LEFT + 20, DETAIL_BOT - 25, "Install / Download", 0);
        } else {
            /* [ Launch ] and [ Remove ] */
            int mid = DETAIL_LEFT + (DETAIL_RIGHT - DETAIL_LEFT) / 2;
            gfx_fill(DETAIL_LEFT + 8, DETAIL_BOT - 32, mid - 4, DETAIL_BOT - 8, 1);
            gfx_frame(DETAIL_LEFT + 8, DETAIL_BOT - 32, mid - 4, DETAIL_BOT - 8);
            gfx_text(DETAIL_LEFT + 16, DETAIL_BOT - 25, "Launch", 0);

            gfx_frame(mid + 4, DETAIL_BOT - 32, DETAIL_RIGHT - 8, DETAIL_BOT - 8);
            gfx_text(mid + 16, DETAIL_BOT - 25, "Remove", 1);
        }
    } else {
        gfx_text(DETAIL_LEFT + 12, DETAIL_TOP + 20, "No packages found in", 1);
        gfx_text(DETAIL_LEFT + 12, DETAIL_TOP + 34, "this category.", 1);
    }

    /* Bottom Status Bar */
    gfx_fill(LIST_LEFT, STATUS_TOP, WINW - 10, STATUS_BOT, 0);
    gfx_frame(LIST_LEFT, STATUS_TOP, WINW - 10, STATUS_BOT);
    gfx_text(LIST_LEFT + 6, STATUS_TOP + 4, status_msg, 1);

    gwin_update(&win);
}

static void update_catalog(void)
{
    snprintf(status_msg, sizeof(status_msg), "Connecting to package repository...");
    draw_ui();

    if (net_init() != 0) {
        snprintf(status_msg, sizeof(status_msg), "Network offline. Check DaynaPORT.");
        draw_ui();
        return;
    }

    char host[64];
    int port;
    get_repo_host(host, sizeof(host), &port);

    static uint8_t cat_buf[8192];
    snprintf(status_msg, sizeof(status_msg), "Fetching catalog from %s:%d...", host, port);
    draw_ui();

    int n = http_get(host, port, "/packages/index.txt", cat_buf, sizeof(cat_buf) - 1);
    if (n <= 0) {
        snprintf(status_msg, sizeof(status_msg), "Catalog download failed (code %d).", n);
        draw_ui();
        return;
    }

    cat_buf[n] = 0;
    FSDelete(cat_file, 0);
    Create(cat_file, 0, 'PLOS', 'TEXT');
    INTEGER ref;
    if (FSOpen(cat_file, 0, &ref) == 0) {
        LONGINT len = n;
        FSWrite(ref, &len, cat_buf);
        FSClose(ref);
    }

    load_cached_catalog();
    snprintf(status_msg, sizeof(status_msg), "Catalog updated: %d packages available.", npkgs);
    draw_ui();
}

static void install_selected(void)
{
    if (nfiltered <= 0 || sel < 0 || sel >= nfiltered) return;
    int idx = filtered_indices[sel];
    PkgEntry *p = &pkgs[idx];

    snprintf(status_msg, sizeof(status_msg), "Initializing network for %s...", p->name);
    draw_ui();

    if (net_init() != 0) {
        snprintf(status_msg, sizeof(status_msg), "Network offline. Check DaynaPORT.");
        draw_ui();
        return;
    }

    char host[64];
    int port;
    get_repo_host(host, sizeof(host), &port);

    char url[64];
    snprintf(url, sizeof(url), "/packages/%s.pex", p->name);

    snprintf(status_msg, sizeof(status_msg), "Downloading %s.pex (%lu KB)...", p->name, (unsigned long)(p->size >> 10));
    draw_ui();

    static uint8_t pex_buf[196608]; /* up to 192 KB executable */
    int bytes = http_get(host, port, url, pex_buf, sizeof(pex_buf));
    if (bytes <= 0) {
        snprintf(status_msg, sizeof(status_msg), "Download failed for %s (code %d).", p->name, bytes);
        draw_ui();
        return;
    }

    /* Choose target directory based on category */
    char target_path[64], upper[32];
    to_upper(upper, p->name);
    if (!strcmp(p->category, "game") || !strcmp(p->category, "games")) {
        snprintf(target_path, sizeof(target_path), ":Games:%s", upper);
    } else {
        snprintf(target_path, sizeof(target_path), ":bin:%s", upper);
    }

    unsigned char pas_path[64];
    c2p(pas_path, target_path);

    FSDelete(pas_path, 0);
    Create(pas_path, 0, 'PLOS', 'APPL');
    INTEGER ref;
    if (FSOpen(pas_path, 0, &ref) != 0) {
        /* Fallback to root if bin doesn't exist */
        snprintf(target_path, sizeof(target_path), ":%s", upper);
        c2p(pas_path, target_path);
        Create(pas_path, 0, 'PLOS', 'APPL');
        if (FSOpen(pas_path, 0, &ref) != 0) {
            snprintf(status_msg, sizeof(status_msg), "Failed to create %s on disk.", target_path);
            draw_ui();
            return;
        }
    }

    LONGINT wr_len = bytes;
    FSWrite(ref, &wr_len, pex_buf);
    FSClose(ref);

    refresh_installed_status();
    snprintf(status_msg, sizeof(status_msg), "Successfully installed %s (%d bytes).", p->name, bytes);
    draw_ui();
}

static void remove_selected(void)
{
    if (nfiltered <= 0 || sel < 0 || sel >= nfiltered) return;
    int idx = filtered_indices[sel];
    PkgEntry *p = &pkgs[idx];

    char upper[32], path[64];
    to_upper(upper, p->name);
    unsigned char pas[64];

    snprintf(path, sizeof(path), ":bin:%s", upper);
    c2p(pas, path);
    FSDelete(pas, 0);

    snprintf(path, sizeof(path), ":Games:%s", upper);
    c2p(pas, path);
    FSDelete(pas, 0);

    refresh_installed_status();
    snprintf(status_msg, sizeof(status_msg), "Removed package %s.", p->name);
    draw_ui();
}

static void launch_selected(void)
{
    if (nfiltered <= 0 || sel < 0 || sel >= nfiltered) return;
    int idx = filtered_indices[sel];
    PkgEntry *p = &pkgs[idx];

    char upper[32], path[64];
    to_upper(upper, p->name);

    if (!strcmp(p->category, "game") || !strcmp(p->category, "games")) {
        snprintf(path, sizeof(path), ":Games:%s", upper);
    } else {
        snprintf(path, sizeof(path), ":bin:%s", upper);
    }

    char *av[] = { path, 0 };
    struct plusos_spawn_args s = { path, 1, av };
    plusos_syscall(SYS_termspawn, &s);
    snprintf(status_msg, sizeof(status_msg), "Launched %s.", p->name);
    draw_ui();
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (gwin_open(&win, "PlusOS Software Center", WINW, WINH) != 0) {
        printf("pkgmgr: must be run inside the PlusOS graphical desktop.\n");
        printf("For command line, use 'pkg list' or 'pkg install <name>'.\n");
        return 1;
    }

    load_cached_catalog();
    draw_ui();

    for (;;) {
        GEvent ev;
        if (gwin_event(&win, &ev, 1) <= 0) continue;

        if (ev.type == GEV_CLOSE) {
            break;
        }

        if (ev.type == GEV_KEY) {
            if (ev.key == KEY_ESC || ev.key == 'q' || ev.key == 'Q') {
                break;
            } else if (ev.key == KEY_UP) {
                if (sel > 0) {
                    sel--;
                    if (sel < scroll) scroll = sel;
                    draw_ui();
                }
            } else if (ev.key == KEY_DOWN) {
                if (sel < nfiltered - 1) {
                    sel++;
                    if (sel >= scroll + VISROWS) scroll = sel - VISROWS + 1;
                    draw_ui();
                }
            } else if (ev.key == KEY_RETURN || ev.key == KEY_ENTER) {
                if (nfiltered > 0 && sel >= 0 && sel < nfiltered) {
                    int idx = filtered_indices[sel];
                    if (pkgs[idx].installed) launch_selected();
                    else install_selected();
                }
            }
        }

        if (ev.type == GEV_MOUSEDOWN) {
            int mx = ev.x, my = ev.y;

            /* Category clicks */
            for (int i = 0; i < NUM_CATS; i++) {
                if (in_rect(mx, my, cat_x0[i], CAT_TOP, cat_x1[i], CAT_BOT)) {
                    cur_cat = i;
                    apply_filter();
                    draw_ui();
                    break;
                }
            }

            /* Update Catalog click */
            if (in_rect(mx, my, WINW - 110, CAT_TOP, WINW - 10, CAT_BOT)) {
                update_catalog();
                continue;
            }

            /* List item click */
            if (in_rect(mx, my, LIST_LEFT, LIST_TOP, LIST_RIGHT, LIST_BOT)) {
                int clicked_row = scroll + (my - LIST_TOP) / ROWH;
                if (clicked_row >= 0 && clicked_row < nfiltered) {
                    sel = clicked_row;
                    draw_ui();
                }
            }

            /* Action button clicks */
            if (nfiltered > 0 && sel >= 0 && sel < nfiltered) {
                int idx = filtered_indices[sel];
                PkgEntry *p = &pkgs[idx];

                if (!p->installed) {
                    /* Install / Download button */
                    if (in_rect(mx, my, DETAIL_LEFT + 8, DETAIL_BOT - 32, DETAIL_RIGHT - 8, DETAIL_BOT - 8)) {
                        install_selected();
                    }
                } else {
                    /* Launch / Remove buttons */
                    int mid = DETAIL_LEFT + (DETAIL_RIGHT - DETAIL_LEFT) / 2;
                    if (in_rect(mx, my, DETAIL_LEFT + 8, DETAIL_BOT - 32, mid - 4, DETAIL_BOT - 8)) {
                        launch_selected();
                    } else if (in_rect(mx, my, mid + 4, DETAIL_BOT - 32, DETAIL_RIGHT - 8, DETAIL_BOT - 8)) {
                        remove_selected();
                    }
                }
            }
        }
    }

    gwin_close(&win);
    return 0;
}
