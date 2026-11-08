/* pkg.c - PlusOS Package Manager (CLI).
 *
 * An APT / YUM style package manager for PlusOS:
 *   pkg update           - download and cache repository package catalog
 *   pkg list             - display available and installed packages
 *   pkg search <term>    - search catalog by name or description
 *   pkg install <pkg>    - download and install package binary (.pex)
 *   pkg remove <pkg>     - uninstall package from disk
 *   pkg config repo <ip> - set package repository host (default: plusos.d0a.net)
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusnet/net.h>
#include <plusrom/FileMgr.h>

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
} PkgInfo;

static PkgInfo pkgs[MAX_PKGS];
static int npkgs = 0;
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

static int is_installed(const char *name)
{
    char upper[32], path[64];
    to_upper(upper, name);
    snprintf(path, sizeof(path), ":bin:%s", upper);
    unsigned char p[64];
    c2p(p, path);
    INTEGER ref;
    if (FSOpen(p, 0, &ref) == 0) {
        FSClose(ref);
        return 1;
    }
    return 0;
}

static void load_catalog(void)
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

            /* Parse name|version|category|size|desc */
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
            pkgs[npkgs].installed = is_installed(pkgs[npkgs].name);
            npkgs++;
        }
    }
    FSClose(ref);
}

static void save_catalog(const char *data, int len)
{
    Create(cat_file, 0, 0x74747874 /* 'ttxt' */, 0x54455854 /* 'TEXT' */);
    INTEGER ref;
    if (FSOpen(cat_file, 0, &ref) == 0) {
        LONGINT n = len;
        SetFPos(ref, 1, 0);
        FSWrite(ref, &n, data);
        SetEOF(ref, len);
        FSClose(ref);
        FlushVol(0, 0);
    }
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
    uint32_t ip;
    printf("Resolving %s... ", host);
    if (net_resolve(host, &ip) != 0) {
        printf("FAILED\n");
        return -1;
    }
    char ip_str[16];
    printf("%s\nConnecting to %s:%d... ", net_ip2str(ip, ip_str), host, port);

    if (net_tcp_connect(ip, (uint16_t)port, 300) != 0) {
        printf("FAILED\n");
        return -1;
    }
    printf("CONNECTED\n");

    char req[256];
    int req_len = snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: PlusOS-Pkg/1.0 (m68k)\r\n"
        "Connection: close\r\n\r\n", path, host);

    net_tcp_send(req, req_len);

    /* Stream response and skip HTTP headers */
    int total_read = 0;
    int in_body = 0;
    uint8_t temp[512];
    uint32_t timeout = 600;

    while (timeout > 0) {
        net_poll();
        int rc = net_tcp_recv(temp, sizeof(temp));
        if (rc < 0) break; /* connection closed */
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

static void cmd_update(void)
{
    char host[64]; int port;
    get_repo_host(host, sizeof(host), &port);
    printf("Updating package lists from http://%s:%d/...\n", host, port);

    static uint8_t cat_buf[8192];
    int n = http_get(host, port, "/packages/index.txt", cat_buf, sizeof(cat_buf) - 1);
    if (n <= 0) {
        printf("pkg: failed to fetch repository catalog.\n");
        return;
    }
    cat_buf[n] = 0;
    save_catalog((char *)cat_buf, n);
    load_catalog();
    printf("Fetched %d packages from repository.\n", npkgs);
}

static void cmd_list(const char *filter)
{
    load_catalog();
    if (npkgs == 0) {
        printf("Catalog is empty. Run 'pkg update' to download repository list.\n");
        return;
    }

    printf("\n%-14s %-12s %-8s %-12s %s\n", "PACKAGE", "CATEGORY", "SIZE", "STATUS", "DESCRIPTION");
    printf("----------------------------------------------------------------------\n");

    int shown = 0;
    for (int i = 0; i < npkgs; i++) {
        if (filter && filter[0]) {
            if (!strstr(pkgs[i].name, filter) && !strstr(pkgs[i].desc, filter) && !strstr(pkgs[i].category, filter)) {
                continue;
            }
        }
        char sz_str[16];
        snprintf(sz_str, sizeof(sz_str), "%lu KB", (unsigned long)(pkgs[i].size / 1024));
        const char *st = pkgs[i].installed ? "[Installed]" : "[Available]";
        printf("%-14s %-12s %-8s %-12s %s\n", pkgs[i].name, pkgs[i].category, sz_str, st, pkgs[i].desc);
        shown++;
    }
    printf("----------------------------------------------------------------------\n");
    printf("%d package(s) listed.\n", shown);
}

static void cmd_install(const char *name)
{
    char host[64]; int port;
    get_repo_host(host, sizeof(host), &port);

    char url_path[64];
    snprintf(url_path, sizeof(url_path), "/packages/%s.pex", name);
    printf("Downloading %s from http://%s:%d%s...\n", name, host, port, url_path);

    /* Allocate buffer for package download */
    uint32_t buf_sz = 128 * 1024; /* 128 KB max app size */
    uint8_t *bin_buf = (uint8_t *)plusos_syscall(SYS_malloc, (void *)(intptr_t)buf_sz);
    if (!bin_buf) {
        printf("pkg: insufficient memory to download package.\n");
        return;
    }

    int bytes = http_get(host, port, url_path, bin_buf, (int)buf_sz);
    if (bytes <= 0) {
        printf("pkg: failed to download '%s.pex'. Check package name or connection.\n", name);
        plusos_syscall(SYS_free, bin_buf);
        return;
    }

    /* Verify PLOS header signature */
    if (bytes >= 8 && memcmp(&bin_buf[4], "PLOS", 4) != 0) {
        printf("pkg: warning: downloaded file has no PLOS executable header.\n");
    }

    /* Save to :bin:<NAME> */
    char upper[32], bin_path[64];
    to_upper(upper, name);
    snprintf(bin_path, sizeof(bin_path), ":bin:%s", upper);

    unsigned char p[64];
    c2p(p, bin_path);
    Create(p, 0, 0x504C4F53 /* 'PLOS' */, 0x504C4F53 /* 'PLOS' */);
    INTEGER ref;
    if (FSOpen(p, 0, &ref) == 0) {
        LONGINT n = bytes;
        SetFPos(ref, 1, 0);
        FSWrite(ref, &n, bin_buf);
        SetEOF(ref, bytes);
        FSClose(ref);
        FlushVol(0, 0);
        printf("Successfully installed %s (%d bytes) to :bin:%s!\n", name, bytes, upper);
        printf("Type '%s' to run it.\n", name);
    } else {
        printf("pkg: failed to write %s to disk.\n", bin_path);
    }

    plusos_syscall(SYS_free, bin_buf);
}

static void cmd_remove(const char *name)
{
    char upper[32], bin_path[64];
    to_upper(upper, name);
    snprintf(bin_path, sizeof(bin_path), ":bin:%s", upper);

    unsigned char p[64];
    c2p(p, bin_path);
    if (FSDelete(p, 0) == 0) {
        FlushVol(0, 0);
        printf("Package '%s' successfully uninstalled from :bin:.\n", name);
    } else {
        printf("pkg: package '%s' is not installed or could not be removed.\n", name);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("PlusOS Package Manager (APT/YUM style)\n");
        printf("usage: pkg <command> [args]\n");
        printf("  pkg update             download latest package lists\n");
        printf("  pkg list               show all available and installed packages\n");
        printf("  pkg search <term>      find packages by keyword\n");
        printf("  pkg install <package>  download and install package to :bin:\n");
        printf("  pkg remove <package>   delete installed package\n");
        printf("  pkg config repo <host> set repository address\n");
        return 0;
    }

    const char *cmd = argv[1];

    if (!strcmp(cmd, "list")) {
        cmd_list(argc >= 3 ? argv[2] : NULL);
        return 0;
    }
    if (!strcmp(cmd, "search")) {
        if (argc < 3) { printf("usage: pkg search <term>\n"); return 1; }
        cmd_list(argv[2]);
        return 0;
    }
    if (!strcmp(cmd, "remove")) {
        if (argc < 3) { printf("usage: pkg remove <package>\n"); return 1; }
        cmd_remove(argv[2]);
        return 0;
    }
    if (!strcmp(cmd, "config") && argc >= 4 && !strcmp(argv[2], "repo")) {
        struct plusos_reg_args ra;
        memset(&ra, 0, sizeof(ra));
        ra.key = "pkg.repo";
        ra.value = argv[3];
        plusos_syscall(SYS_regset, &ra);
        printf("Package repository set to: %s\n", argv[3]);
        return 0;
    }

    /* Network required commands */
    if (net_init() != 0) {
        printf("pkg: network interface offline. Check BlueSCSI DaynaPORT or 'net' setup.\n");
        return 1;
    }

    if (!strcmp(cmd, "update")) {
        cmd_update();
        return 0;
    }
    if (!strcmp(cmd, "install")) {
        if (argc < 3) { printf("usage: pkg install <package>\n"); return 1; }
        cmd_install(argv[2]);
        return 0;
    }

    printf("pkg: unknown command '%s'. Try 'pkg' for help.\n", cmd);
    return 1;
}
