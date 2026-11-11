/* users.c - PlusOS user accounts, backed by /etc/passwd and /etc/shadow.
 *
 * The HFS volume has no "/" separator; everything on disk uses the colon-
 * delimited pascal strings the Mac ROM expects. All path strings here are
 * hard-coded in that form (":etc:passwd", ":home:bob:Desktop", ...).
 *
 * Phase 1 scope: read/write the two files, salted-hash passwords, hand out
 * records. No enforcement. No /etc/sudoers (that is Phase 4). No Unix-path
 * translator in the shell (Phase 2).
 */
#include "kernel.h"
#include "kconfig.h"
#include "users.h"
#include "kmem.h"
#include "proc.h"

#if CONFIG_USERS

#define MAX_USERS   16                  /* keep the in-memory table small */

/* A loaded account (passwd record + its shadow entry). */
typedef struct {
    user_rec rec;
    char     salt[USER_SALT_LEN + 1];
    char     hash[USER_HASH_LEN + 1];
} account;

static account accts[MAX_USERS];
static int     accts_n;
static int     current_uid;             /* 0 until login sets it */
static int     loaded;                  /* users_init() has run */
static int     passwd_absent;           /* 1 if /etc/passwd could not be read */

/* ---- HFS helpers --------------------------------------------------------- */
/* All reads/writes go through the ROM File Manager - the same pattern as
 * config.c. Paths are HFS-style colon strings. */
static void c2pstr(unsigned char *p, const char *c)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

static int fs_ok(void) { return (g_flags & PLUSOS_HAVE_FS) != 0; }

/* Read a whole file into a kmalloc'd buffer (nul-terminated). */
static char *read_file(const char *path, long *size_out)
{
    if (!fs_ok()) return 0;
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
    if (!fs_ok()) return -1;
    unsigned char pn[256]; INTEGER ref;
    c2pstr(pn, path);
    if (FSOpen(pn, g_boot_vref, &ref) != 0) {
        Create(pn, g_boot_vref, 0x504C4F53 /* 'PLOS' */, 0x54455854 /* 'TEXT' */);
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

/* Find a child directory by name under parent dir id; returns dir id or 0. */
static long find_subdir(long parent, const char *name)
{
    for (int idx = 1; idx < 300; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = g_boot_vref;
        pb.dirInfo.ioDrDirID = parent;
        pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        int n = nm[0]; for (int i = 0; i < n; i++) c[i] = (char)nm[i + 1]; c[n] = 0;
        if (!(pb.dirInfo.ioFlAttrib & 0x10)) continue;
        /* case-insensitive match */
        int eq = 1;
        for (int i = 0; i <= n; i++) {
            char a = c[i], b = name[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
            if (a != b) { eq = 0; break; }
        }
        if (eq) return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

static long ensure_dir(long parent, const char *name)
{
    long d = find_subdir(parent, name);
    if (d) return d;
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    c2pstr(nm, name);
    pb.fileParam.ioNamePtr = nm;
    pb.fileParam.ioVRefNum = g_boot_vref;
    pb.fileParam.ioDirID   = parent;
    if (PBDirCreateSync(&pb) != 0) return 0;
    return find_subdir(parent, name);
}

/* ---- password hashing ---------------------------------------------------- */
/* Simple salted multi-round FNV-1a-ish hash. Not real crypto - it keeps the
 * password out of plain sight in /etc/shadow and makes a dictionary attack
 * at least annoying on this hardware. See users.h for the honest framing. */
#define FNV_OFF  0x811C9DC5u
#define FNV_P    0x01000193u
#define HASH_ROUNDS  1000

static void hash_pw(const char *salt, const char *pw, char out[USER_HASH_LEN + 1])
{
    uint32_t h[4] = { FNV_OFF, FNV_OFF ^ 0x9E3779B9u, FNV_OFF ^ 0x6A09E667u, FNV_OFF ^ 0xBB67AE85u };
    for (int r = 0; r < HASH_ROUNDS; r++) {
        const char *q = salt;
        while (*q) { for (int k = 0; k < 4; k++) h[k] = (h[k] ^ (uint8_t)*q) * FNV_P; q++; }
        q = pw;
        while (*q) { for (int k = 0; k < 4; k++) h[k] = (h[k] ^ (uint8_t)*q) * FNV_P; q++; }
        h[0] += r; h[1] ^= h[0]; h[2] += h[1]; h[3] ^= h[2];
    }
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 4; i++) {
        uint32_t v = h[i];
        for (int j = 0; j < 4; j++) {
            out[i * 4 + j] = hex[(v >> 28) & 0xF];
            v <<= 4;
        }
    }
    out[USER_HASH_LEN] = 0;
}

static void make_salt(char salt[USER_SALT_LEN + 1])
{
    /* Mix the tick counter with a few lowmem words - predictable but not
     * identical across machines/boots, which is all we need. */
    uint32_t t = (uint32_t)tick_count();
    uint32_t m = LM_LONG(0x20C) ^ LM_LONG(0x174);
    static const char alnum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < USER_SALT_LEN; i++) {
        t = t * 1103515245u + 12345u + m;
        salt[i] = alnum[(t >> 8) % 62];
    }
    salt[USER_SALT_LEN] = 0;
}

/* ---- passwd / shadow parsing -------------------------------------------- */
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

static int parse_int(const char *s)
{
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

static void parse_passwd(char *data)
{
    accts_n = 0;
    const char *p = data;
    while (*p && accts_n < MAX_USERS) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        if (!*p) break;
        account *a = &accts[accts_n];
        char uid_s[8], gid_s[8], flags_s[8];
        p = copy_field(p, a->rec.name,  USER_NAME_MAX,  ':');
        p = copy_field(p, uid_s,        sizeof uid_s,   ':');
        p = copy_field(p, gid_s,        sizeof gid_s,   ':');
        p = copy_field(p, a->rec.home,  USER_HOME_MAX,  ':');
        p = copy_field(p, a->rec.shell, USER_SHELL_MAX, ':');
        p = copy_field(p, flags_s,      sizeof flags_s, ':');
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        a->rec.uid   = (uint16_t)parse_int(uid_s);
        a->rec.gid   = (uint16_t)parse_int(gid_s);
        a->rec.flags = (uint8_t)parse_int(flags_s);
        a->salt[0] = 0; a->hash[0] = 0;
        if (a->rec.name[0]) accts_n++;
    }
}

static void parse_shadow(char *data)
{
    const char *p = data;
    while (*p) {
        char name[USER_NAME_MAX], salt[USER_SALT_LEN + 1], hash[USER_HASH_LEN + 1];
        if (*p == '#' || *p == '\n') { while (*p && *p != '\n') p++; if (*p) p++; continue; }
        p = copy_field(p, name, sizeof name, ':');
        p = copy_field(p, salt, sizeof salt, ':');
        p = copy_field(p, hash, sizeof hash, ':');
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        for (int i = 0; i < accts_n; i++) {
            if (!strcmp(accts[i].rec.name, name)) {
                strncpy(accts[i].salt, salt, USER_SALT_LEN); accts[i].salt[USER_SALT_LEN] = 0;
                strncpy(accts[i].hash, hash, USER_HASH_LEN); accts[i].hash[USER_HASH_LEN] = 0;
                break;
            }
        }
    }
}

/* ---- writers ------------------------------------------------------------- */
static int write_passwd(void)
{
    char buf[1024]; int n = 0;
    n += snprintf(buf + n, sizeof buf - n,
        "# /etc/passwd - PlusOS user accounts\n"
        "# name:uid:gid:home:shell:flags   (flags bit 0 = sudoer)\n");
    for (int i = 0; i < accts_n && n < (int)sizeof buf - 128; i++) {
        account *a = &accts[i];
        n += snprintf(buf + n, sizeof buf - n, "%s:%u:%u:%s:%s:%u\n",
            a->rec.name, (unsigned)a->rec.uid, (unsigned)a->rec.gid,
            a->rec.home, a->rec.shell, (unsigned)a->rec.flags);
    }
    return write_file(":etc:passwd", buf, n);
}

static int write_shadow(void)
{
    char buf[1024]; int n = 0;
    n += snprintf(buf + n, sizeof buf - n,
        "# /etc/shadow - PlusOS password hashes (keep this to root only)\n"
        "# name:salt:hash\n");
    for (int i = 0; i < accts_n && n < (int)sizeof buf - 64; i++) {
        account *a = &accts[i];
        n += snprintf(buf + n, sizeof buf - n, "%s:%s:%s\n",
            a->rec.name, a->salt, a->hash);
    }
    return write_file(":etc:shadow", buf, n);
}

/* ---- public API ---------------------------------------------------------- */
void users_init(void)
{
    if (loaded) return;
    loaded = 1;
    long sz = 0;
    char *pw = read_file(":etc:passwd", &sz);
    if (!pw) { passwd_absent = 1; return; }
    parse_passwd(pw);
    kfree(pw);
    char *sh = read_file(":etc:shadow", &sz);
    if (sh) { parse_shadow(sh); kfree(sh); }
    printf("users: %d account%s loaded\n", accts_n, accts_n == 1 ? "" : "s");
}

int users_firstrun_needed(void)
{
    users_init();
    return passwd_absent || accts_n == 0;
}

int users_find(const char *name, user_rec *out)
{
    users_init();
    for (int i = 0; i < accts_n; i++)
        if (!strcmp(accts[i].rec.name, name)) {
            if (out) *out = accts[i].rec;
            return 0;
        }
    return -1;
}

int users_find_uid(int uid, user_rec *out)
{
    users_init();
    for (int i = 0; i < accts_n; i++)
        if (accts[i].rec.uid == (uint16_t)uid) {
            if (out) *out = accts[i].rec;
            return 0;
        }
    return -1;
}

int users_authenticate(const char *name, const char *password)
{
    users_init();
    for (int i = 0; i < accts_n; i++) {
        if (strcmp(accts[i].rec.name, name)) continue;
        /* empty hash means no-password account; accept any empty password */
        if (accts[i].hash[0] == 0) return (password == 0 || password[0] == 0);
        if (!password) return 0;
        char h[USER_HASH_LEN + 1];
        hash_pw(accts[i].salt, password, h);
        return strcmp(h, accts[i].hash) == 0;
    }
    return 0;
}

static void make_home(const char *home)
{
    /* home is e.g. "/home/bob" or "/root"; translate to HFS dir ids. */
    long parent = 2;                     /* 2 = root dir in HFS */
    const char *p = home;
    if (*p == '/') p++;
    while (*p) {
        char seg[32]; int n = 0;
        while (*p && *p != '/' && n < 31) { seg[n++] = *p++; }
        seg[n] = 0;
        if (*p == '/') p++;
        parent = ensure_dir(parent, seg);
        if (!parent) return;
    }
    ensure_dir(parent, "Desktop");
    ensure_dir(parent, "Documents");
    ensure_dir(parent, "Settings");
}

int users_add(const char *name, const char *password, int is_sudoer, int is_root)
{
    users_init();
    if (!name || !name[0]) return -1;
    if (accts_n >= MAX_USERS) return -1;
    if (users_find(name, 0) == 0) return -1;              /* already exists */

    /* /etc must exist before we can write into it */
    ensure_dir(2, "etc");

    account *a = &accts[accts_n];
    memset(a, 0, sizeof *a);
    strncpy(a->rec.name, name, USER_NAME_MAX - 1);
    if (is_root) {
        a->rec.uid = 0; a->rec.gid = 0;
        strncpy(a->rec.home, "/root", USER_HOME_MAX - 1);
    } else {
        uint16_t next_uid = 1000;
        for (int i = 0; i < accts_n; i++)
            if (accts[i].rec.uid >= next_uid) next_uid = (uint16_t)(accts[i].rec.uid + 1);
        a->rec.uid = next_uid;
        a->rec.gid = next_uid;
        snprintf(a->rec.home, USER_HOME_MAX, "/home/%s", name);
    }
    strncpy(a->rec.shell, "/bin/shell", USER_SHELL_MAX - 1);
    a->rec.flags = is_sudoer ? USER_F_SUDOER : 0;
    if (password && password[0]) {
        make_salt(a->salt);
        hash_pw(a->salt, password, a->hash);
    } else {
        a->salt[0] = 0; a->hash[0] = 0;
    }
    accts_n++;
    passwd_absent = 0;

    make_home(a->rec.home);
    if (write_passwd() != 0) { accts_n--; return -1; }
    write_shadow();
    users_write_sudoers();
    return 0;
}

int users_set_password(const char *name, const char *new_pw)
{
    users_init();
    for (int i = 0; i < accts_n; i++) {
        if (strcmp(accts[i].rec.name, name) != 0) continue;
        if (new_pw && new_pw[0]) {
            make_salt(accts[i].salt);
            hash_pw(accts[i].salt, new_pw, accts[i].hash);
        } else {
            accts[i].salt[0] = 0; accts[i].hash[0] = 0;
        }
        return write_shadow();
    }
    return -1;
}

int users_is_sudoer(const char *name)
{
    if (!name || !name[0]) return 0;
    long sz = 0;
    char *data = read_file(":etc:sudoers", &sz);
    if (!data) return 0;
    int hit = 0;
    const char *p = data;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        int i = 0;
        while (p[i] && p[i] != '\n' && p[i] != '\r' && p[i] != ' ' && p[i] != '\t' && i < 32) i++;
        /* compare */
        int match = 1, k;
        for (k = 0; k < i; k++) if (p[k] != name[k]) { match = 0; break; }
        if (match && name[i] == 0) { hit = 1; break; }
        while (*p && *p != '\n') p++;
        if (*p) p++;
    }
    kfree(data);
    return hit;
}

int users_write_sudoers(void)
{
    char buf[512]; int n = 0;
    n += snprintf(buf + n, sizeof buf - n,
        "# /etc/sudoers - one username per line; '#' starts a comment.\n"
        "# Managed by the firstrun wizard and adduser; edit by hand if you must.\n");
    for (int i = 0; i < accts_n && n < (int)sizeof buf - 32; i++) {
        if (accts[i].rec.flags & USER_F_SUDOER)
            n += snprintf(buf + n, sizeof buf - n, "%s\n", accts[i].rec.name);
    }
    return write_file(":etc:sudoers", buf, n);
}

int users_delete(const char *name)
{
    users_init();
    if (!name || !name[0]) return -1;
    for (int i = 0; i < accts_n; i++) {
        if (strcmp(accts[i].rec.name, name) != 0) continue;
        if (accts[i].rec.uid == 0) return -1;             /* never delete root */
        for (int k = i; k < accts_n - 1; k++) accts[k] = accts[k + 1];
        accts_n--;
        write_passwd();
        write_shadow();
        users_write_sudoers();
        return 0;
    }
    return -1;
}

int hfs_path_translate(const char *in, char *out, int cap)
{
    if (!in || !out || cap < 2) return -1;
    const char *src = in;
    char home[USER_HOME_MAX] = "";
    int o = 0;

    /* ~ expansion: "~" alone or "~/foo..." - use the current user's home. */
    if (src[0] == '~' && (src[1] == 0 || src[1] == '/')) {
        user_rec u;
        if (users_find_uid(current_uid, &u) == 0) {
            strncpy(home, u.home, sizeof home - 1);
        } else {
            strncpy(home, "/", sizeof home - 1);
        }
        /* Splice: anything in `home` that starts with '/' becomes ':' */
        for (int i = 0; home[i] && o < cap - 1; i++)
            out[o++] = (home[i] == '/') ? ':' : home[i];
        src++;                                           /* skip the '~' */
    }

    /* Convert each remaining character. Absolute path (/foo/bar) becomes
     * :foo:bar; a relative path passes through unchanged. Collapse runs of
     * '/' so "//etc///passwd" normalises cleanly. */
    int last_was_sep = 0;
    while (*src && o < cap - 1) {
        char c = *src++;
        if (c == '/') {
            if (last_was_sep) continue;
            out[o++] = ':';
            last_was_sep = 1;
        } else {
            out[o++] = c;
            last_was_sep = 0;
        }
    }
    out[o] = 0;
    return (*src == 0) ? 0 : -1;
}

int  users_get_current(void) { return current_uid; }
void users_set_current(int uid) { current_uid = uid; }
int  users_count(void) { users_init(); return accts_n; }
int  users_at(int i, user_rec *out)
{
    users_init();
    if (i < 0 || i >= accts_n) return -1;
    if (out) *out = accts[i].rec;
    return 0;
}

#endif /* CONFIG_USERS */
