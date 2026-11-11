/* perms.c - PlusOS file permissions sidecar.
 *
 * Reads and writes /etc/perms.db. See perms.h for the full honest framing of
 * what "enforcement" means without an MMU on a 68000.
 */
#include "kernel.h"
#include "kconfig.h"
#include "perms.h"
#include "kmem.h"
#include "proc.h"

#if CONFIG_PERMS

#define MAX_PERMS 64

static perms_rec tab[MAX_PERMS];
static int       tab_n;
static int       loaded;

/* ---- HFS helpers (duplicate of users.c, kept tiny so perms doesn't need to
 * depend on the users module). */
static void c2pstr(unsigned char *p, const char *c)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

static char *read_file(const char *path, long *size_out)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return 0;
    unsigned char pn[256]; INTEGER ref;
    c2pstr(pn, path);
    if (FSOpen(pn, g_boot_vref, &ref) != 0) return 0;
    LONGINT size = 0;
    GetEOF(ref, &size);
    if (size < 0 || size > 65536) { FSClose(ref); return 0; }
    char *buf = kmalloc((uint32_t)size + 1);
    if (!buf) { FSClose(ref); return 0; }
    LONGINT n = size;
    if (FSRead(ref, &n, buf) != 0 || n != size) { kfree(buf); FSClose(ref); return 0; }
    buf[size] = 0;
    FSClose(ref);
    if (size_out) *size_out = size;
    return buf;
}

static int write_file(const char *path, const char *data, long len)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return -1;
    unsigned char pn[256]; INTEGER ref;
    c2pstr(pn, path);
    if (FSOpen(pn, g_boot_vref, &ref) != 0) {
        Create(pn, g_boot_vref, 0x504C4F53, 0x54455854);
        if (FSOpen(pn, g_boot_vref, &ref) != 0) return -1;
    }
    SetFPos(ref, 1, 0);
    LONGINT n = len;
    int rc = (FSWrite(ref, &n, data) == 0 && n == len) ? 0 : -1;
    SetEOF(ref, len);
    FSClose(ref);
    FlushVol(0, g_boot_vref);
    return rc;
}

/* ---- parsing ------------------------------------------------------------- */
static int parse_octal(const char *s)
{
    int v = 0;
    while (*s >= '0' && *s <= '7') { v = v * 8 + (*s - '0'); s++; }
    return v;
}

static int parse_dec(const char *s)
{
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

static const char *copy_field(const char *src, char *dst, int cap, char sep)
{
    int n = 0;
    while (src[n] && src[n] != sep && src[n] != '\n' && src[n] != '\r' && n < cap - 1) {
        dst[n] = src[n]; n++;
    }
    dst[n] = 0;
    while (src[n] && src[n] != sep && src[n] != '\n' && src[n] != '\r') n++;
    if (src[n] == sep) n++;
    return src + n;
}

static void load_db(void)
{
    if (loaded) return;
    loaded = 1;
    tab_n = 0;
    long sz = 0;
    char *data = read_file(":etc:perms.db", &sz);
    if (!data) return;
    const char *p = data;
    while (*p && tab_n < MAX_PERMS) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        if (!*p) break;
        char mode_s[8], uid_s[8], gid_s[8];
        p = copy_field(p, tab[tab_n].path, PERMS_PATH_MAX, ':');
        p = copy_field(p, mode_s, sizeof mode_s, ':');
        p = copy_field(p, uid_s,  sizeof uid_s,  ':');
        p = copy_field(p, gid_s,  sizeof gid_s,  ':');
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        tab[tab_n].mode = (uint16_t)parse_octal(mode_s);
        tab[tab_n].uid  = (uint16_t)parse_dec(uid_s);
        tab[tab_n].gid  = (uint16_t)parse_dec(gid_s);
        if (tab[tab_n].path[0]) tab_n++;
    }
    kfree(data);
}

static int save_db(void)
{
    char buf[2048]; int n = 0;
    n += snprintf(buf + n, sizeof buf - n,
        "# /etc/perms.db - PlusOS advisory permissions (see perms.h)\n"
        "# path:mode:uid:gid     (mode in octal)\n");
    for (int i = 0; i < tab_n && n < (int)sizeof buf - 64; i++) {
        n += snprintf(buf + n, sizeof buf - n, "%s:%04o:%u:%u\n",
            tab[i].path, tab[i].mode, (unsigned)tab[i].uid, (unsigned)tab[i].gid);
    }
    return write_file(":etc:perms.db", buf, n);
}

/* ---- public API ---------------------------------------------------------- */
void perms_init(void)
{
    load_db();
    printf("perms: %d entrie%s loaded\n", tab_n, tab_n == 1 ? "" : "s");
}

int perms_get(const char *path, perms_rec *out)
{
    load_db();
    for (int i = 0; i < tab_n; i++)
        if (strcmp(tab[i].path, path) == 0) {
            if (out) *out = tab[i];
            return 0;
        }
    return -1;
}

int perms_set(const char *path, uint16_t mode, uint16_t uid, uint16_t gid)
{
    load_db();
    if (!path || !path[0]) return -1;
    for (int i = 0; i < tab_n; i++) {
        if (strcmp(tab[i].path, path) == 0) {
            tab[i].mode = mode; tab[i].uid = uid; tab[i].gid = gid;
            return save_db();
        }
    }
    if (tab_n >= MAX_PERMS) return -1;
    strncpy(tab[tab_n].path, path, PERMS_PATH_MAX - 1);
    tab[tab_n].path[PERMS_PATH_MAX - 1] = 0;
    tab[tab_n].mode = mode;
    tab[tab_n].uid  = uid;
    tab[tab_n].gid  = gid;
    tab_n++;
    return save_db();
}

int perms_chmod(const char *path, uint16_t mode, int caller_uid)
{
    perms_rec r;
    int have = (perms_get(path, &r) == 0);
    uint16_t cur_uid = have ? r.uid : 0;
    uint16_t cur_gid = have ? r.gid : 0;
    /* Root, or owner. */
    if (caller_uid != 0 && (uint16_t)caller_uid != cur_uid) return -1;
    return perms_set(path, mode & 0777, cur_uid, cur_gid);
}

int perms_chown(const char *path, uint16_t uid, uint16_t gid, int caller_uid)
{
    /* Changing ownership is root-only, like Unix. */
    if (caller_uid != 0) return -1;
    perms_rec r;
    uint16_t mode = (perms_get(path, &r) == 0) ? r.mode : 0644;
    return perms_set(path, mode, uid, gid);
}

int perms_check(const char *path, int caller_uid, int caller_gid, int want)
{
    if (caller_uid == 0) return 1;              /* root wins */
    perms_rec r;
    uint16_t mode = 0644, owner_uid = 0, owner_gid = 0;
    if (perms_get(path, &r) == 0) {
        mode = r.mode; owner_uid = r.uid; owner_gid = r.gid;
    }
    int bits;
    if ((uint16_t)caller_uid == owner_uid)      bits = (mode >> 6) & 7;
    else if ((uint16_t)caller_gid == owner_gid) bits = (mode >> 3) & 7;
    else                                        bits = mode & 7;
    return (bits & (want & 7)) == (want & 7);
}

int perms_count(void) { load_db(); return tab_n; }
int perms_at(int i, perms_rec *out)
{
    load_db();
    if (i < 0 || i >= tab_n) return -1;
    if (out) *out = tab[i];
    return 0;
}

#endif /* CONFIG_PERMS */
