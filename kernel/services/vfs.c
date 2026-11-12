/* vfs.c - PlusOS POSIX Virtual File System & Open File Descriptor Layer.
 *
 * Bridges Unix hierarchical paths, symlinks, FIFOs, /dev nodes, open file
 * descriptors (0=stdin, 1=stdout, 2=stderr, ...), and pipes onto the
 * Macintosh HFS volume and kernel pipe subsystem.
 */
#include "kernel.h"
#include "proc.h"
#include "pipe.h"
#include "vfs.h"
#include "environ.h"
#include "perms.h"
#include "entropy.h"
#include "kmem.h"

#define MAX_VFS_FILES  48
#define MAX_SYMLINKS   24
#define MAX_FIFOS      8

enum {
    VF_NONE = 0,
    VF_CONSOLE,
    VF_HFS,
    VF_PIPE_R,
    VF_PIPE_W,
    VF_DEV_NULL,
    VF_DEV_ZERO,
    VF_DEV_RAND
};

typedef struct {
    uint8_t  type;
    uint8_t  refcnt;
    uint16_t flags;
    INTEGER  hfs_ref;
    INTEGER  vref;
    int32_t  pos;
    int      pipe_id;
    char     path[96];
} VFSFile;

typedef struct {
    uint8_t active;
    char    link_path[64];
    char    target[64];
} SymLinkEnt;

typedef struct {
    uint8_t active;
    char    path[64];
    int     pipe_id;
    uint32_t mode;
} FifoNode;

static VFSFile    g_files[MAX_VFS_FILES];
static SymLinkEnt g_links[MAX_SYMLINKS];
static FifoNode   g_fifos[MAX_FIFOS];

/* Reserve slots 0=stdin, 1=stdout, 2=stderr permanently as console objects */
static void p2c(const unsigned char *p, char *c, int cap)
{
    int n = p[0];
    if (n >= cap) n = cap - 1;
    for (int i = 0; i < n; i++) c[i] = (char)p[i + 1];
    c[n] = 0;
}

static void c2p(const char *c, unsigned char *p)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

static int find_link(const char *path)
{
    for (int i = 0; i < MAX_SYMLINKS; i++)
        if (g_links[i].active && strcasecmp(g_links[i].link_path, path) == 0)
            return i;
    return -1;
}

static int find_fifo(const char *path)
{
    for (int i = 0; i < MAX_FIFOS; i++)
        if (g_fifos[i].active && strcasecmp(g_fifos[i].path, path) == 0)
            return i;
    return -1;
}

static uint32_t mac_to_unix_time(uint32_t mac_sec)
{
    return (mac_sec >= MAC_UNIX_EPOCH_DELTA) ? (mac_sec - MAC_UNIX_EPOCH_DELTA) : 0;
}

static uint32_t unix_to_mac_time(uint32_t unix_sec)
{
    return unix_sec + MAC_UNIX_EPOCH_DELTA;
}

static uint32_t hash_str(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++) { h ^= (uint8_t)*s; h *= 16777619u; }
    return h;
}

/* Normalize input path (Unix or :HFS:colon or ~/rel) into clean "/a/b" form. */
static void raw_normalize(const char *in, char *out, int cap)
{
    char tmp[160];
    int o = 0;
    if (!in || !in[0]) {
        const char *cwd = (proc_current && proc_current->cwd[0]) ? proc_current->cwd : "/";
        strlcpy(out, cwd, (size_t)cap);
        return;
    }

    /* Convert HFS ':foo:bar' to '/foo/bar' */
    char conv[128];
    if (in[0] == ':') {
        int j = 0;
        for (int i = 0; in[i] && j < (int)sizeof conv - 1; i++)
            conv[j++] = (in[i] == ':') ? '/' : in[i];
        conv[j] = 0;
        in = conv;
    }

    if (in[0] == '~' && (in[1] == 0 || in[1] == '/')) {
        char home[64] = "/";
        env_get("HOME", home, sizeof home);
        if (home[0] == ':') {
            for (int k = 0; home[k]; k++) if (home[k] == ':') home[k] = '/';
        }
        for (int i = 0; home[i] && o < (int)sizeof tmp - 1; i++) tmp[o++] = home[i];
        in++;
    } else if (in[0] != '/') {
        const char *cwd = "/";
        if (proc_current && proc_current->cwd[0]) cwd = proc_current->cwd;
        else {
            static char pbuf[96];
            if (env_get("PWD", pbuf, sizeof pbuf) > 0 && pbuf[0] == '/') cwd = pbuf;
        }
        for (int i = 0; cwd[i] && o < (int)sizeof tmp - 1; i++) tmp[o++] = cwd[i];
    } else if (proc_current && proc_current->root_dir[0] &&
               strcmp(proc_current->root_dir, "/") != 0) {
        const char *rt = proc_current->root_dir;
        for (int i = 0; rt[i] && o < (int)sizeof tmp - 1; i++) tmp[o++] = rt[i];
    }

    if (o == 0 || tmp[o - 1] != '/') if (o < (int)sizeof tmp - 1) tmp[o++] = '/';
    for (int i = 0; in[i] && o < (int)sizeof tmp - 1; i++) tmp[o++] = in[i];
    tmp[o] = 0;

    int ti = 0;
    out[ti++] = '/';
    int i = 0;
    while (tmp[i]) {
        while (tmp[i] == '/') i++;
        if (!tmp[i]) break;
        char seg[64];
        int s = 0;
        while (tmp[i] && tmp[i] != '/' && s < (int)sizeof seg - 1) seg[s++] = tmp[i++];
        seg[s] = 0;
        if (strcmp(seg, ".") == 0) continue;
        if (strcmp(seg, "..") == 0) {
            if (ti > 1) {
                ti--;
                while (ti > 1 && out[ti - 1] != '/') ti--;
            }
            continue;
        }
        if (ti > 1 && out[ti - 1] != '/' && ti < cap - 1) out[ti++] = '/';
        for (int k = 0; seg[k] && ti < cap - 1; k++) out[ti++] = seg[k];
    }
    if (ti > 1 && out[ti - 1] == '/') ti--;
    out[ti] = 0;
}

int vfs_normalize_path(const char *in, char *out, int cap, int follow_last_link)
{
    char cur[128];
    raw_normalize(in, cur, sizeof cur);
    if (!follow_last_link) {
        strlcpy(out, cur, (size_t)cap);
        return 0;
    }
    for (int hop = 0; hop < 8; hop++) {
        int li = find_link(cur);
        if (li < 0) break;
        const char *tgt = g_links[li].target;
        if (tgt[0] == '/') {
            raw_normalize(tgt, cur, sizeof cur);
        } else {
            char combined[128];
            strlcpy(combined, cur, sizeof combined);
            char *sl = strrchr(combined, '/');
            if (sl) sl[1] = 0;
            size_t cl = strlen(combined);
            if (cl + 1 < sizeof combined)
                strlcpy(combined + cl, tgt, sizeof combined - cl);
            raw_normalize(combined, cur, sizeof cur);
        }
    }
    strlcpy(out, cur, (size_t)cap);
    return 0;
}

/* Walk normalized "/a/b/c" down HFS directories from fsRtDirID=2.
 * Sets *parent_dir and *leaf (if non-root). Returns 0 if parent exists, else -2 (-ENOENT). */
static int hfs_resolve_parent(const char *norm, long *parent_dir, char *leaf, int leaf_cap)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return -19; /* -ENODEV */
    if (strcmp(norm, "/") == 0) {
        *parent_dir = 2L;
        if (leaf) leaf[0] = 0;
        return 0;
    }
    const char *last_slash = strrchr(norm, '/');
    const char *name = last_slash ? (last_slash + 1) : norm;
    if (leaf) strlcpy(leaf, name, (size_t)leaf_cap);

    long dir = 2L; /* fsRtDirID */
    const char *p = norm + 1;
    while (p && p < last_slash) {
        char seg[64];
        int n = 0;
        while (*p && *p != '/' && n < (int)sizeof seg - 1) seg[n++] = *p++;
        seg[n] = 0;
        if (*p == '/') p++;
        if (!seg[0]) continue;
        unsigned char pseg[64];
        c2p(seg, pseg);
        CInfoPBRec pb;
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = pseg;
        pb.dirInfo.ioVRefNum = g_boot_vref;
        pb.dirInfo.ioDrDirID = dir;
        pb.dirInfo.ioFDirIndex = 0;
        if (PBGetCatInfoSync(&pb) != 0) return -2; /* -ENOENT */
        if (!(pb.dirInfo.ioFlAttrib & 0x10)) return -20; /* -ENOTDIR */
        dir = pb.dirInfo.ioDrDirID;
    }
    *parent_dir = dir;
    return 0;
}

static int hfs_get_info(long parent_dir, const char *leaf, CInfoPBRec *pb)
{
    unsigned char pnm[64];
    memset(pb, 0, sizeof *pb);
    if (!leaf || !leaf[0]) {
        /* Stat root directory itself (dirID = 2) */
        c2p("", pnm);
        pb->dirInfo.ioNamePtr = pnm;
        pb->dirInfo.ioVRefNum = g_boot_vref;
        pb->dirInfo.ioDrDirID = parent_dir;
        pb->dirInfo.ioFDirIndex = -1;
        return PBGetCatInfoSync(pb) == 0 ? 0 : -2;
    }
    c2p(leaf, pnm);
    pb->dirInfo.ioNamePtr = pnm;
    pb->dirInfo.ioVRefNum = g_boot_vref;
    pb->dirInfo.ioDrDirID = parent_dir;
    pb->dirInfo.ioFDirIndex = 0;
    return PBGetCatInfoSync(pb) == 0 ? 0 : -2;
}

/* Open a new HFS file handle in a specific directory using HOpen / HCreate */
static OSErr hfs_open_in_dir(long dir_id, const char *leaf, int8_t perm, INTEGER *ref)
{
    HParamBlockRec hp;
    unsigned char pnm[64];
    c2p(leaf, pnm);
    memset(&hp, 0, sizeof hp);
    hp.ioParam.ioNamePtr = pnm;
    hp.ioParam.ioVRefNum = g_boot_vref;
    hp.ioParam.ioPermssn = perm;
    hp.fileParam.ioDirID = dir_id;
    OSErr e = PBHOpenSync(&hp);
    if (e == 0) *ref = hp.ioParam.ioRefNum;
    return e;
}

static OSErr hfs_create_in_dir(long dir_id, const char *leaf)
{
    HParamBlockRec hp;
    unsigned char pnm[64];
    c2p(leaf, pnm);
    memset(&hp, 0, sizeof hp);
    hp.ioParam.ioNamePtr = pnm;
    hp.ioParam.ioVRefNum = g_boot_vref;
    hp.fileParam.ioDirID = dir_id;
    OSErr e = PBHCreateSync(&hp);
    if (e == 0) {
        CInfoPBRec cp;
        memset(&cp, 0, sizeof cp);
        c2p(leaf, pnm);
        cp.hFileInfo.ioNamePtr = pnm;
        cp.hFileInfo.ioVRefNum = g_boot_vref;
        cp.hFileInfo.ioDirID = dir_id;
        if (PBGetCatInfoSync(&cp) == 0) {
            cp.hFileInfo.ioFlFndrInfo.fdType = 0x54455854UL;    /* 'TEXT' */
            cp.hFileInfo.ioFlFndrInfo.fdCreator = 0x504C4F53UL; /* 'PLOS' */
            PBSetCatInfoSync(&cp);
        }
    }
    return e;
}

static int alloc_vfs_slot(uint8_t type)
{
    for (int i = 3; i < MAX_VFS_FILES; i++) {
        if (g_files[i].type == VF_NONE) {
            memset(&g_files[i], 0, sizeof g_files[i]);
            g_files[i].type = type;
            g_files[i].refcnt = 1;
            g_files[i].pipe_id = -1;
            return i;
        }
    }
    return -23; /* -ENFILE */
}

static int alloc_proc_fd(int handle, int min_fd)
{
    if (!proc_current) return -9;
    for (int i = min_fd; i < MAX_PROC_FDS; i++) {
        if (proc_current->fds[i] < 0) {
            proc_current->fds[i] = handle;
            return i;
        }
    }
    return -24; /* -EMFILE */
}

static void release_handle(int h)
{
    if (h < 0 || h >= MAX_VFS_FILES) return;
    VFSFile *f = &g_files[h];
    if (f->type == VF_NONE) return;
    if (f->refcnt > 1) {
        f->refcnt--;
        return;
    }
    if (h < 3 && f->type == VF_CONSOLE) return; /* permanent console slots */
    if (f->type == VF_HFS && f->hfs_ref) {
        FSClose(f->hfs_ref);
    } else if (f->type == VF_PIPE_R) {
        pipe_release(f->pipe_id, 0);
    } else if (f->type == VF_PIPE_W) {
        pipe_release(f->pipe_id, 1);
    }
    memset(f, 0, sizeof *f);
}

static void addref_handle(int h)
{
    if (h < 0 || h >= MAX_VFS_FILES) return;
    VFSFile *f = &g_files[h];
    if (f->type == VF_NONE) return;
    if (f->refcnt < 250) f->refcnt++;
    /* Each independent open handle counts once inside pipe_id; refcnt tracks FD duplicates */
}

void vfs_init(void)
{
    memset(g_files, 0, sizeof g_files);
    memset(g_links, 0, sizeof g_links);
    memset(g_fifos, 0, sizeof g_fifos);
    for (int i = 0; i < 3; i++) {
        g_files[i].type = VF_CONSOLE;
        g_files[i].refcnt = 1;
        g_files[i].pipe_id = -1;
    }
    /* Ensure /tmp exists on HFS volume if writable */
    if (g_flags & PLUSOS_HAVE_FS) {
        HParamBlockRec hp;
        unsigned char pnm[16];
        c2p("tmp", pnm);
        memset(&hp, 0, sizeof hp);
        hp.ioParam.ioNamePtr = pnm;
        hp.ioParam.ioVRefNum = g_boot_vref;
        hp.fileParam.ioDirID = 2L;
        LONGINT created_id = 0;
        hp.fileParam.ioDirID = 2L;
        PBDirCreateSync(&hp);
        (void)created_id;
    }
}

void vfs_proc_init(struct Proc *p, struct Proc *parent, const int fds_override[3])
{
    if (!p) return;
    p->umask = parent ? parent->umask : 0022;
    if (parent && parent->cwd[0]) strlcpy(p->cwd, parent->cwd, sizeof p->cwd);
    else strlcpy(p->cwd, "/", sizeof p->cwd);
    if (parent && parent->root_dir[0]) strlcpy(p->root_dir, parent->root_dir, sizeof p->root_dir);
    else p->root_dir[0] = 0;

    for (int i = 0; i < MAX_PROC_FDS; i++) p->fds[i] = -1;
    for (int i = 0; i < 3; i++) {
        int h = -1;
        if (fds_override && fds_override[i] >= 0) {
            int pfd = fds_override[i];
            if (parent && pfd < MAX_PROC_FDS && parent->fds[pfd] >= 0)
                h = parent->fds[pfd];
        } else if (parent && parent->fds[i] >= 0) {
            h = parent->fds[i];
        }
        if (h < 0) h = i; /* default console slot 0/1/2 */
        p->fds[i] = h;
        addref_handle(h);
    }
}

void vfs_proc_exit(struct Proc *p)
{
    if (!p) return;
    for (int i = 0; i < MAX_PROC_FDS; i++) {
        if (p->fds[i] >= 0) {
            release_handle(p->fds[i]);
            p->fds[i] = -1;
        }
    }
}

int vfs_fd_is_console(int fd)
{
    if (!proc_current || fd < 0 || fd >= MAX_PROC_FDS) return 1;
    int h = proc_current->fds[fd];
    if (h < 0 || h >= MAX_VFS_FILES) return 0;
    return g_files[h].type == VF_CONSOLE;
}

int vfs_fd_read(int fd, void *buf, int32_t count)
{
    if (!buf || count < 0) return -22;
    if (count == 0) return 0;
    int h = (proc_current && fd >= 0 && fd < MAX_PROC_FDS) ? proc_current->fds[fd] : fd;
    if (h < 0 || h >= MAX_VFS_FILES || g_files[h].type == VF_NONE) return -9; /* -EBADF */
    VFSFile *f = &g_files[h];

    switch (f->type) {
    case VF_CONSOLE: {
        char *dst = (char *)buf;
        int32_t n = 0;
        while (n < count) {
            int k = 0;
            if (!term_getkey(1, &k)) k = kbd_get();
            if (k == '\r') k = '\n';
            if (k == 4 && n == 0) return 0; /* Ctrl-D EOF */
            dst[n++] = (char)k;
            if (k == '\n') break;
        }
        return n;
    }
    case VF_HFS: {
        LONGINT n = count;
        if (SetFPos(f->hfs_ref, 1, f->pos) != 0) return -5;
        OSErr e = FSRead(f->hfs_ref, &n, buf);
        if (n > 0) f->pos += (int32_t)n;
        if (e == 0 || e == -39 /* eofErr */) return (int)n;
        return -5;
    }
    case VF_PIPE_R:
        return pipe_read(f->pipe_id, buf, (int)count, (f->flags & PLUSOS_O_NONBLOCK) != 0);
    case VF_DEV_NULL:
        return 0;
    case VF_DEV_ZERO:
        memset(buf, 0, (size_t)count);
        return (int)count;
    case VF_DEV_RAND:
        entropy_get_bytes((uint8_t *)buf, (uint32_t)count);
        return (int)count;
    default:
        return -9;
    }
}

int vfs_fd_write(int fd, const void *buf, int32_t count)
{
    if (!buf || count < 0) return -22;
    if (count == 0) return 0;
    int h = (proc_current && fd >= 0 && fd < MAX_PROC_FDS) ? proc_current->fds[fd] : fd;
    if (h < 0 || h >= MAX_VFS_FILES || g_files[h].type == VF_NONE) return -9;
    VFSFile *f = &g_files[h];

    switch (f->type) {
    case VF_CONSOLE: {
        struct plusos_con_args ca = { PLUSOS_CON_WRITE, 0, 0, (const char *)buf, (int)count, 0, 0 };
        long r;
        if (term_console_op(&ca, &r)) return (int)count;
        con_write((const char *)buf, (size_t)count);
        return (int)count;
    }
    case VF_HFS: {
        if (f->flags & PLUSOS_O_APPEND) {
            LONGINT eof = 0;
            if (GetEOF(f->hfs_ref, &eof) == 0) f->pos = (int32_t)eof;
        }
        if (SetFPos(f->hfs_ref, 1, f->pos) != 0) return -5;
        LONGINT n = count;
        OSErr e = FSWrite(f->hfs_ref, &n, buf);
        if (n > 0) f->pos += (int32_t)n;
        return (e == 0) ? (int)n : -28; /* -ENOSPC / I/O err */
    }
    case VF_PIPE_W:
        return pipe_write(f->pipe_id, buf, (int)count, (f->flags & PLUSOS_O_NONBLOCK) != 0);
    case VF_DEV_NULL:
    case VF_DEV_ZERO:
    case VF_DEV_RAND:
        return (int)count;
    default:
        return -9;
    }
}

int vfs_pipe_alloc(int *rfd, int *wfd)
{
    int pr = -1, pw = -1;
    int pid_pipe = pipe_create(&pr, &pw, 0 /* VFS-refcounted */);
    if (pid_pipe < 0) return -23;
    int hr = alloc_vfs_slot(VF_PIPE_R);
    int hw = alloc_vfs_slot(VF_PIPE_W);
    if (hr < 0 || hw < 0) {
        if (hr >= 0) memset(&g_files[hr], 0, sizeof g_files[hr]);
        if (hw >= 0) memset(&g_files[hw], 0, sizeof g_files[hw]);
        pipe_close(pid_pipe);
        return -23;
    }
    g_files[hr].pipe_id = pid_pipe;
    g_files[hw].pipe_id = pid_pipe;
    int fr = alloc_proc_fd(hr, 0);
    int fw = (fr >= 0) ? alloc_proc_fd(hw, 0) : -1;
    if (fr < 0 || fw < 0) {
        if (fr >= 0 && proc_current) proc_current->fds[fr] = -1;
        release_handle(hr);
        release_handle(hw);
        return -24;
    }
    *rfd = fr;
    *wfd = fw;
    return 0;
}

static int vfs_open_path(const char *path, int flags, uint32_t mode)
{
    (void)mode;
    if (!path || !path[0]) return -2;
    char norm[128];
    vfs_normalize_path(path, norm, sizeof norm, 1);

    /* Virtual devices */
    if (strcmp(norm, "/dev/null") == 0) {
        int h = alloc_vfs_slot(VF_DEV_NULL);
        if (h < 0) return h;
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }
    if (strcmp(norm, "/dev/zero") == 0) {
        int h = alloc_vfs_slot(VF_DEV_ZERO);
        if (h < 0) return h;
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }
    if (strcmp(norm, "/dev/random") == 0 || strcmp(norm, "/dev/urandom") == 0) {
        int h = alloc_vfs_slot(VF_DEV_RAND);
        if (h < 0) return h;
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }
    if (strcmp(norm, "/dev/tty") == 0 || strcmp(norm, "/dev/console") == 0) {
        int h = (flags & PLUSOS_O_ACCMODE) == PLUSOS_O_RDONLY ? 0 : 1;
        addref_handle(h);
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }
    if (strcmp(norm, "/dev/stdin") == 0 || strcmp(norm, "/dev/stdout") == 0 || strcmp(norm, "/dev/stderr") == 0) {
        int src = (norm[8] == 'i') ? 0 : (norm[8] == 'o' ? 1 : 2);
        int h = proc_current ? proc_current->fds[src] : src;
        if (h < 0) return -9;
        addref_handle(h);
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }

    /* Check FIFO nodes */
    int fi = find_fifo(norm);
    if (fi >= 0) {
        int wr = (flags & PLUSOS_O_ACCMODE) != PLUSOS_O_RDONLY;
        int h = alloc_vfs_slot(wr ? VF_PIPE_W : VF_PIPE_R);
        if (h < 0) return h;
        g_files[h].flags = (uint16_t)flags;
        g_files[h].pipe_id = g_fifos[fi].pipe_id;
        pipe_addref(g_fifos[fi].pipe_id, wr);
        int fd = alloc_proc_fd(h, 0);
        if (fd < 0) { release_handle(h); return fd; }
        return fd;
    }

    long pdir = 0;
    char leaf[64];
    int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
    if (rc < 0) return rc;
    if (!leaf[0]) return -21; /* -EISDIR */

    CInfoPBRec cp;
    int exists = (hfs_get_info(pdir, leaf, &cp) == 0);
    if (exists && (cp.dirInfo.ioFlAttrib & 0x10)) return -21; /* -EISDIR */
    if (exists && (flags & PLUSOS_O_CREAT) && (flags & PLUSOS_O_EXCL)) return -17; /* -EEXIST */
    if (!exists) {
        if (!(flags & PLUSOS_O_CREAT)) return -2; /* -ENOENT */
        if (hfs_create_in_dir(pdir, leaf) != 0) return -5;
        perms_set(norm, (uint16_t)(mode & ~(proc_current ? proc_current->umask : 0022)),
                  proc_current ? proc_current->uid : 0, proc_current ? proc_current->gid : 0);
    }

    int acc = flags & PLUSOS_O_ACCMODE;
    int8_t perm = (acc == PLUSOS_O_RDONLY) ? 1 : (acc == PLUSOS_O_WRONLY ? 2 : 3);
    INTEGER ref = 0;
    if (hfs_open_in_dir(pdir, leaf, perm, &ref) != 0) {
        /* Fall back to shared rd/wr perm 0 if locked by single writer/reader */
        if (hfs_open_in_dir(pdir, leaf, 0, &ref) != 0) return -13;
    }
    if ((flags & PLUSOS_O_TRUNC) && acc != PLUSOS_O_RDONLY)
        SetEOF(ref, 0);

    int h = alloc_vfs_slot(VF_HFS);
    if (h < 0) { FSClose(ref); return h; }
    g_files[h].hfs_ref = ref;
    g_files[h].vref = g_boot_vref;
    g_files[h].flags = (uint16_t)flags;
    g_files[h].pos = 0;
    if (flags & PLUSOS_O_APPEND) {
        LONGINT eof = 0;
        if (GetEOF(ref, &eof) == 0) g_files[h].pos = (int32_t)eof;
    }
    strlcpy(g_files[h].path, norm, sizeof g_files[h].path);
    int fd = alloc_proc_fd(h, 0);
    if (fd < 0) { release_handle(h); return fd; }
    return fd;
}

static int vfs_stat_path(const char *path, struct plusos_stat *st, int follow_links)
{
    if (!path || !st) return -22;
    memset(st, 0, sizeof *st);
    char norm[128];
    vfs_normalize_path(path, norm, sizeof norm, follow_links);

    if (!follow_links) {
        int li = find_link(norm);
        if (li >= 0) {
            st->st_ino = hash_str(norm);
            st->st_mode = 0120777; /* S_IFLNK | 0777 */
            st->st_nlink = 1;
            st->st_size = (int32_t)strlen(g_links[li].target);
            st->st_blksize = 512;
            return 0;
        }
    }
    if (strcmp(norm, "/dev") == 0) {
        st->st_ino = 100; st->st_mode = 0040755; st->st_nlink = 2; st->st_blksize = 512;
        return 0;
    }
    if (strncmp(norm, "/dev/", 5) == 0 &&
        (strcmp(norm + 5, "null") == 0 || strcmp(norm + 5, "zero") == 0 ||
         strcmp(norm + 5, "tty") == 0 || strcmp(norm + 5, "console") == 0 ||
         strcmp(norm + 5, "random") == 0 || strcmp(norm + 5, "urandom") == 0)) {
        st->st_ino = hash_str(norm); st->st_mode = 0020666; st->st_nlink = 1; st->st_blksize = 512;
        return 0;
    }
    int fi = find_fifo(norm);
    if (fi >= 0) {
        st->st_ino = hash_str(norm);
        st->st_mode = 0010000 | (g_fifos[fi].mode & 0777);
        st->st_nlink = 1;
        st->st_blksize = 512;
        return 0;
    }

    long pdir = 0;
    char leaf[64];
    int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
    if (rc < 0) return rc;

    CInfoPBRec cp;
    if (hfs_get_info(pdir, leaf, &cp) < 0) return -2;

    st->st_dev = (uint32_t)(uint16_t)g_boot_vref;
    st->st_blksize = 512;
    st->st_nlink = 1;
    if (cp.dirInfo.ioFlAttrib & 0x10) {
        st->st_ino = (uint32_t)cp.dirInfo.ioDrDirID;
        st->st_mode = 0040755;
        st->st_size = (int32_t)cp.dirInfo.ioDrNmFls;
        st->st_mtime = mac_to_unix_time(cp.dirInfo.ioDrMdDat);
        st->st_ctime = mac_to_unix_time(cp.dirInfo.ioDrCrDat);
        st->st_atime = st->st_mtime;
    } else {
        st->st_ino = (uint32_t)cp.hFileInfo.ioDirID;
        st->st_mode = 0100644;
        st->st_size = cp.hFileInfo.ioFlLgLen;
        st->st_blocks = (st->st_size + 511) / 512;
        st->st_mtime = mac_to_unix_time(cp.hFileInfo.ioFlMdDat);
        st->st_ctime = mac_to_unix_time(cp.hFileInfo.ioFlCrDat);
        st->st_atime = st->st_mtime;
    }

    perms_rec pr;
    if (perms_get(norm, &pr) == 0) {
        st->st_mode = (st->st_mode & ~0777u) | (pr.mode & 0777u);
        st->st_uid = pr.uid;
        st->st_gid = pr.gid;
    }
    return 0;
}

long vfs_syscall(struct plusos_vfs_args *a)
{
    if (!a) return -22;
    switch (a->op) {
    case PLUSOS_VFS_OPEN:
        return vfs_open_path(a->path, a->flags, a->mode);
    case PLUSOS_VFS_CLOSE: {
        int fd = a->fd;
        if (!proc_current || fd < 0 || fd >= MAX_PROC_FDS || proc_current->fds[fd] < 0) return -9;
        release_handle(proc_current->fds[fd]);
        proc_current->fds[fd] = -1;
        return 0;
    }
    case PLUSOS_VFS_READ:
        return vfs_fd_read(a->fd, a->buf, a->count);
    case PLUSOS_VFS_WRITE:
        return vfs_fd_write(a->fd, a->buf, a->count);
    case PLUSOS_VFS_LSEEK: {
        int fd = a->fd;
        if (!proc_current || fd < 0 || fd >= MAX_PROC_FDS || proc_current->fds[fd] < 0) return -9;
        VFSFile *f = &g_files[proc_current->fds[fd]];
        if (f->type != VF_HFS) return -29; /* -ESPIPE */
        int32_t base = 0;
        if (a->flags == 1 /* SEEK_CUR */) base = f->pos;
        else if (a->flags == 2 /* SEEK_END */) {
            LONGINT eof = 0;
            GetEOF(f->hfs_ref, &eof);
            base = (int32_t)eof;
        }
        int32_t np = base + a->offset;
        if (np < 0) return -22;
        f->pos = np;
        return np;
    }
    case PLUSOS_VFS_DUP: {
        int fd = a->fd;
        if (!proc_current || fd < 0 || fd >= MAX_PROC_FDS || proc_current->fds[fd] < 0) return -9;
        int h = proc_current->fds[fd];
        addref_handle(h);
        int nfd = alloc_proc_fd(h, 0);
        if (nfd < 0) { release_handle(h); return nfd; }
        return nfd;
    }
    case PLUSOS_VFS_DUP2: {
        int oldfd = a->fd, newfd = a->fd2;
        if (!proc_current || oldfd < 0 || oldfd >= MAX_PROC_FDS || proc_current->fds[oldfd] < 0 ||
            newfd < 0 || newfd >= MAX_PROC_FDS) return -9;
        if (oldfd == newfd) return newfd;
        if (proc_current->fds[newfd] >= 0) release_handle(proc_current->fds[newfd]);
        int h = proc_current->fds[oldfd];
        addref_handle(h);
        proc_current->fds[newfd] = h;
        return newfd;
    }
    case PLUSOS_VFS_STAT:
        return vfs_stat_path(a->path, (struct plusos_stat *)a->buf, 1);
    case PLUSOS_VFS_LSTAT:
        return vfs_stat_path(a->path, (struct plusos_stat *)a->buf, 0);
    case PLUSOS_VFS_FSTAT: {
        int fd = a->fd;
        struct plusos_stat *st = (struct plusos_stat *)a->buf;
        if (!st || !proc_current || fd < 0 || fd >= MAX_PROC_FDS || proc_current->fds[fd] < 0) return -9;
        VFSFile *f = &g_files[proc_current->fds[fd]];
        if (f->type == VF_HFS && f->path[0])
            return vfs_stat_path(f->path, st, 1);
        memset(st, 0, sizeof *st);
        st->st_nlink = 1;
        st->st_blksize = 512;
        st->st_mode = (f->type == VF_PIPE_R || f->type == VF_PIPE_W) ? 0010600 : 0020666;
        return 0;
    }
    case PLUSOS_VFS_MKDIR: {
        char norm[128], leaf[64];
        vfs_normalize_path(a->path, norm, sizeof norm, 0);
        long pdir = 0;
        int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
        if (rc < 0) return rc;
        if (!leaf[0]) return -17;
        CInfoPBRec cp;
        if (hfs_get_info(pdir, leaf, &cp) == 0) return -17;
        HParamBlockRec hp;
        unsigned char pnm[64];
        c2p(leaf, pnm);
        memset(&hp, 0, sizeof hp);
        hp.ioParam.ioNamePtr = pnm;
        hp.ioParam.ioVRefNum = g_boot_vref;
        hp.fileParam.ioDirID = pdir;
        if (PBDirCreateSync(&hp) != 0) return -5;
        perms_set(norm, (uint16_t)(a->mode & ~(proc_current ? proc_current->umask : 0022)),
                  proc_current ? proc_current->uid : 0, proc_current ? proc_current->gid : 0);
        return 0;
    }
    case PLUSOS_VFS_RMDIR: {
        char norm[128], leaf[64];
        vfs_normalize_path(a->path, norm, sizeof norm, 0);
        long pdir = 0;
        int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
        if (rc < 0) return rc;
        if (!leaf[0]) return -16; /* busy root */
        CInfoPBRec cp;
        if (hfs_get_info(pdir, leaf, &cp) < 0) return -2;
        if (!(cp.dirInfo.ioFlAttrib & 0x10)) return -20;
        if (cp.dirInfo.ioDrNmFls > 0) return -39; /* -ENOTEMPTY */
        HParamBlockRec hp;
        unsigned char pnm[64];
        c2p(leaf, pnm);
        memset(&hp, 0, sizeof hp);
        hp.ioParam.ioNamePtr = pnm;
        hp.ioParam.ioVRefNum = g_boot_vref;
        hp.fileParam.ioDirID = pdir;
        return PBHDeleteSync(&hp) == 0 ? 0 : -5;
    }
    case PLUSOS_VFS_UNLINK: {
        char norm[128], leaf[64];
        vfs_normalize_path(a->path, norm, sizeof norm, 0);
        int li = find_link(norm);
        if (li >= 0) { g_links[li].active = 0; return 0; }
        int fi = find_fifo(norm);
        if (fi >= 0) {
            pipe_close(g_fifos[fi].pipe_id);
            g_fifos[fi].active = 0;
            return 0;
        }
        long pdir = 0;
        int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
        if (rc < 0) return rc;
        if (!leaf[0]) return -1;
        CInfoPBRec cp;
        if (hfs_get_info(pdir, leaf, &cp) < 0) return -2;
        if (cp.dirInfo.ioFlAttrib & 0x10) return -21; /* -EISDIR */
        HParamBlockRec hp;
        unsigned char pnm[64];
        c2p(leaf, pnm);
        memset(&hp, 0, sizeof hp);
        hp.ioParam.ioNamePtr = pnm;
        hp.ioParam.ioVRefNum = g_boot_vref;
        hp.fileParam.ioDirID = pdir;
        return PBHDeleteSync(&hp) == 0 ? 0 : -5;
    }
    case PLUSOS_VFS_RENAME: {
        char n1[128], n2[128], l1[64], l2[64];
        vfs_normalize_path(a->path, n1, sizeof n1, 0);
        vfs_normalize_path(a->path2, n2, sizeof n2, 0);
        if (strcmp(n1, n2) == 0) return 0;
        long d1 = 0, d2 = 0;
        int rc = hfs_resolve_parent(n1, &d1, l1, sizeof l1);
        if (rc < 0 || !l1[0]) return rc < 0 ? rc : -22;
        rc = hfs_resolve_parent(n2, &d2, l2, sizeof l2);
        if (rc < 0 || !l2[0]) return rc < 0 ? rc : -22;
        CInfoPBRec c1, c2;
        if (hfs_get_info(d1, l1, &c1) < 0) return -2;
        if (hfs_get_info(d2, l2, &c2) == 0 && !(c2.dirInfo.ioFlAttrib & 0x10)) {
            HParamBlockRec dp;
            unsigned char pnm[64];
            c2p(l2, pnm);
            memset(&dp, 0, sizeof dp);
            dp.ioParam.ioNamePtr = pnm;
            dp.ioParam.ioVRefNum = g_boot_vref;
            dp.fileParam.ioDirID = d2;
            PBHDeleteSync(&dp);
        }
        unsigned char pn1[64], pn2[64];
        c2p(l1, pn1); c2p(l2, pn2);
        if (d1 != d2) {
            CMovePBRec mp;
            memset(&mp, 0, sizeof mp);
            mp.ioNamePtr = pn1;
            mp.ioVRefNum = g_boot_vref;
            mp.ioDirID = d1;
            mp.ioNewDirID = d2;
            if (PBCatMoveSync(&mp) != 0) return -5;
            d1 = d2;
        }
        if (strcmp(l1, l2) != 0) {
            HParamBlockRec hp;
            memset(&hp, 0, sizeof hp);
            hp.ioParam.ioNamePtr = pn1;
            hp.ioParam.ioVRefNum = g_boot_vref;
            hp.ioParam.ioMisc = (Ptr)pn2;
            hp.fileParam.ioDirID = d1;
            if (PBHRenameSync(&hp) != 0) return -5;
        }
        return 0;
    }
    case PLUSOS_VFS_TRUNCATE: {
        int fd = vfs_open_path(a->path, PLUSOS_O_RDWR, 0644);
        if (fd < 0) return fd;
        VFSFile *f = &g_files[proc_current->fds[fd]];
        OSErr e = (f->type == VF_HFS) ? SetEOF(f->hfs_ref, a->offset) : -22;
        release_handle(proc_current->fds[fd]);
        proc_current->fds[fd] = -1;
        return e == 0 ? 0 : -5;
    }
    case PLUSOS_VFS_FTRUNCATE: {
        int fd = a->fd;
        if (!proc_current || fd < 0 || fd >= MAX_PROC_FDS || proc_current->fds[fd] < 0) return -9;
        VFSFile *f = &g_files[proc_current->fds[fd]];
        if (f->type != VF_HFS) return -22;
        return SetEOF(f->hfs_ref, a->offset) == 0 ? 0 : -5;
    }
    case PLUSOS_VFS_UTIME: {
        char norm[128], leaf[64];
        vfs_normalize_path(a->path, norm, sizeof norm, 1);
        long pdir = 0;
        int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
        if (rc < 0) return rc;
        CInfoPBRec cp;
        if (hfs_get_info(pdir, leaf, &cp) < 0) return -2;
        unsigned char pnm[64];
        c2p(leaf, pnm);
        cp.hFileInfo.ioNamePtr = pnm;
        cp.hFileInfo.ioVRefNum = g_boot_vref;
        if (cp.dirInfo.ioFlAttrib & 0x10) {
            cp.dirInfo.ioDrDirID = pdir;
            cp.dirInfo.ioDrMdDat = unix_to_mac_time(a->t2);
        } else {
            cp.hFileInfo.ioDirID = pdir;
            cp.hFileInfo.ioFlMdDat = unix_to_mac_time(a->t2);
        }
        return PBSetCatInfoSync(&cp) == 0 ? 0 : -5;
    }
    case PLUSOS_VFS_CHDIR: {
        char norm[128];
        vfs_normalize_path(a->path, norm, sizeof norm, 1);
        struct plusos_stat st;
        int rc = vfs_stat_path(norm, &st, 1);
        if (rc < 0) return rc;
        if ((st.st_mode & 0170000) != 0040000) return -20; /* -ENOTDIR */
        if (proc_current) strlcpy(proc_current->cwd, norm, sizeof proc_current->cwd);
        env_set("PWD", norm);
        return 0;
    }
    case PLUSOS_VFS_GETCWD: {
        char *dst = (char *)a->buf;
        if (!dst || a->count <= 0) return -22;
        const char *cwd = (proc_current && proc_current->cwd[0]) ? proc_current->cwd : "/";
        strlcpy(dst, cwd, (size_t)a->count);
        return 0;
    }
    case PLUSOS_VFS_READDIR: {
        struct plusos_dirent *de = (struct plusos_dirent *)a->buf;
        if (!de) return -22;
        char norm[128];
        vfs_normalize_path(a->path, norm, sizeof norm, 1);
        int idx = a->offset; /* 0-based index */
        if (strcmp(norm, "/dev") == 0) {
            static const char *const devs[] = { "null", "zero", "tty", "console", "stdin", "stdout", "stderr", "random", "urandom" };
            int nd = (int)(sizeof devs / sizeof devs[0]);
            if (idx < 0 || idx >= nd) return -2;
            de->d_ino = 1000u + (uint32_t)idx;
            de->d_type = 2; /* chr */
            strlcpy(de->d_name, devs[idx], sizeof de->d_name);
            return 0;
        }
        long pdir = 0;
        char leaf[64];
        int rc = hfs_resolve_parent(norm, &pdir, leaf, sizeof leaf);
        if (rc < 0) return rc;
        long target_dir = pdir;
        if (leaf[0]) {
            CInfoPBRec cp;
            if (hfs_get_info(pdir, leaf, &cp) < 0) return -2;
            if (!(cp.dirInfo.ioFlAttrib & 0x10)) return -20;
            target_dir = cp.dirInfo.ioDrDirID;
        }
        CInfoPBRec ep;
        unsigned char nm[64];
        memset(&ep, 0, sizeof ep);
        ep.dirInfo.ioNamePtr = nm;
        ep.dirInfo.ioVRefNum = g_boot_vref;
        ep.dirInfo.ioDrDirID = target_dir;
        ep.dirInfo.ioFDirIndex = (int16_t)(idx + 1);
        if (PBGetCatInfoSync(&ep) == 0) {
            p2c(nm, de->d_name, sizeof de->d_name);
            de->d_ino = (ep.dirInfo.ioFlAttrib & 0x10) ? (uint32_t)ep.dirInfo.ioDrDirID : (uint32_t)ep.hFileInfo.ioDirID;
            de->d_type = (ep.dirInfo.ioFlAttrib & 0x10) ? 4 : 8;
            return 0;
        }
        return -2;
    }
    case PLUSOS_VFS_SYMLINK: {
        char norm[128];
        vfs_normalize_path(a->path2, norm, sizeof norm, 0);
        if (!a->path || !a->path[0]) return -22;
        if (find_link(norm) >= 0) return -17;
        for (int i = 0; i < MAX_SYMLINKS; i++) {
            if (!g_links[i].active) {
                g_links[i].active = 1;
                strlcpy(g_links[i].link_path, norm, sizeof g_links[i].link_path);
                strlcpy(g_links[i].target, a->path, sizeof g_links[i].target);
                return 0;
            }
        }
        return -28;
    }
    case PLUSOS_VFS_READLINK: {
        char norm[128];
        vfs_normalize_path(a->path, norm, sizeof norm, 0);
        int li = find_link(norm);
        if (li < 0) return -22;
        if (!a->buf || a->count <= 0) return -22;
        strlcpy((char *)a->buf, g_links[li].target, (size_t)a->count);
        return (long)strlen((const char *)a->buf);
    }
    case PLUSOS_VFS_LINK: {
        /* HFS has no native Unix hard links; materialize target contents as new HFS file */
        int fsrc = vfs_open_path(a->path, PLUSOS_O_RDONLY, 0);
        if (fsrc < 0) return fsrc;
        int fdst = vfs_open_path(a->path2, PLUSOS_O_WRONLY | PLUSOS_O_CREAT | PLUSOS_O_EXCL, 0644);
        if (fdst < 0) {
            release_handle(proc_current->fds[fsrc]); proc_current->fds[fsrc] = -1;
            return fdst;
        }
        uint8_t buf[256];
        for (;;) {
            int r = vfs_fd_read(fsrc, buf, sizeof buf);
            if (r <= 0) break;
            if (vfs_fd_write(fdst, buf, r) != r) break;
        }
        release_handle(proc_current->fds[fsrc]); proc_current->fds[fsrc] = -1;
        release_handle(proc_current->fds[fdst]); proc_current->fds[fdst] = -1;
        return 0;
    }
    case PLUSOS_VFS_MKFIFO: {
        char norm[128];
        vfs_normalize_path(a->path, norm, sizeof norm, 0);
        if (find_fifo(norm) >= 0) return -17;
        for (int i = 0; i < MAX_FIFOS; i++) {
            if (!g_fifos[i].active) {
                int r = 0, w = 0;
                int pid = pipe_create(&r, &w, 0);
                if (pid < 0) return -23;
                g_fifos[i].active = 1;
                g_fifos[i].pipe_id = pid;
                g_fifos[i].mode = a->mode ? (a->mode & 0777) : 0644;
                strlcpy(g_fifos[i].path, norm, sizeof g_fifos[i].path);
                return 0;
            }
        }
        return -28;
    }
    case PLUSOS_VFS_STATFS: {
        struct plusos_statfs *sf = (struct plusos_statfs *)a->buf;
        if (!sf) return -22;
        memset(sf, 0, sizeof *sf);
        if (!(g_flags & PLUSOS_HAVE_FS)) return -19;
        HParamBlockRec hp;
        unsigned char vn[32];
        memset(&hp, 0, sizeof hp);
        hp.volumeParam.ioNamePtr = vn;
        hp.volumeParam.ioVRefNum = g_boot_vref;
        if (PBHGetVInfoSync(&hp) != 0) return -5;
        sf->f_bsize  = hp.volumeParam.ioVAlBlkSiz;
        sf->f_blocks = (uint16_t)hp.volumeParam.ioVNmAlBlks;
        sf->f_bfree  = (uint16_t)hp.volumeParam.ioVFrBlk;
        sf->f_bavail = sf->f_bfree;
        sf->f_files  = hp.volumeParam.ioVFilCnt + hp.volumeParam.ioVDirCnt;
        sf->f_namelen = 31;
        p2c(vn, sf->f_volname, sizeof sf->f_volname);
        return 0;
    }
    case PLUSOS_VFS_SYNC:
        if (g_flags & PLUSOS_HAVE_FS) FlushVol(0, g_boot_vref);
        return 0;
    default:
        return -38; /* -ENOSYS */
    }
}
