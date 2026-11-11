/* ls - list files on the boot volume.
 *
 * Usage: ls [-l] [PATH]
 *
 * With no PATH, lists /bin (the commands folder). "ls /" lists the volume
 * root. Any /unix/style/path walks the tree in the usual way. -l shows the
 * advisory mode/owner/size from /etc/perms.db alongside each entry. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>

#define ROOT_DIR 2L                 /* fsRtDirID */

static void p2c(const unsigned char *p, char *c)
{
    int n = p[0];
    for (int i = 0; i < n; i++) c[i] = p[i + 1];
    c[n] = 0;
}

/* find the dirID of a folder named `name` under parent; 0 if not found */
static long find_dir_under(INTEGER vref, long parent, const char *name)
{
    for (int idx = 1; idx < 256; idx++) {
        CInfoPBRec pb;
        unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = parent;
        pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char c[64];
        p2c(nm, c);
        if ((pb.dirInfo.ioFlAttrib & 0x10) && !strcasecmp(c, name))
            return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

/* Walk a /unix/style/path from root; returns the final dir id, or 0. */
static long walk_path(INTEGER vref, const char *path)
{
    long dir = ROOT_DIR;
    const char *p = path;
    if (*p == '/') p++;
    while (*p) {
        char seg[32]; int n = 0;
        while (*p && *p != '/' && n < 31) seg[n++] = *p++;
        seg[n] = 0;
        if (*p == '/') p++;
        if (!seg[0]) continue;
        dir = find_dir_under(vref, dir, seg);
        if (!dir) return 0;
    }
    return dir;
}

/* Build an absolute Unix path for a direntry under `parent_path`. */
static void make_subpath(const char *parent, const char *leaf, char *out, int cap)
{
    int o = 0;
    for (int i = 0; parent[i] && o < cap - 1; i++) out[o++] = parent[i];
    if (o == 0 || out[o - 1] != '/') if (o < cap - 1) out[o++] = '/';
    for (int i = 0; leaf[i] && o < cap - 1; i++) out[o++] = leaf[i];
    out[o] = 0;
}

static void mode_str(int mode, char *out)
{
    static const char rwx[] = "rwx";
    for (int i = 0; i < 9; i++) out[i] = (mode & (1 << (8 - i))) ? rwx[i % 3] : '-';
    out[9] = 0;
}

static int perms_lookup(const char *path, int *mode, int *uid, int *gid)
{
    struct plusos_perms_args p = { 0 };
    p.op = PLUSOS_PERMS_GET;
    for (int i = 0; i < 47 && path[i]; i++) p.path[i] = path[i];
    if (plusos_syscall(SYS_perms, &p) != 0) return -1;
    if (mode) *mode = p.mode;
    if (uid)  *uid  = p.uid;
    if (gid)  *gid  = p.gid;
    return 0;
}

static void uid_name(int uid, char *out, int cap)
{
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND_UID; u.uid = uid;
    if (plusos_syscall(SYS_users, &u) == 0) {
        for (int i = 0; i < cap - 1 && u.name[i]; i++) out[i] = u.name[i];
        out[cap - 1] = 0;
        int k = 0; while (out[k] && k < cap - 1) k++; out[k] = 0;
        return;
    }
    snprintf(out, cap, "%d", uid);
}

static void list(INTEGER vref, long dir, const char *abs_path, int long_form)
{
    printf("%s:\n", abs_path[0] ? abs_path : "/");
    int count = 0;
    for (int idx = 1; idx < 256; idx++) {
        CInfoPBRec pb;
        unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm;
        pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir;
        pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char c[64];
        p2c(nm, c);
        int isdir = (pb.hFileInfo.ioFlAttrib & 0x10) != 0;
        if (long_form) {
            char child_path[96];
            make_subpath(abs_path[0] ? abs_path : "/", c, child_path, sizeof child_path);
            int mode = isdir ? 0755 : 0644, uid = 0, gid = 0;
            perms_lookup(child_path, &mode, &uid, &gid);
            char ms[10]; mode_str(mode, ms);
            char owner[16]; uid_name(uid, owner, sizeof owner);
            unsigned long size = isdir ? 0 : (unsigned long)pb.hFileInfo.ioFlLgLen;
            printf("  %c%s %-10s %6lu  %s\n", isdir ? 'd' : '-', ms, owner, size, c);
        } else {
            if (isdir) printf("  %-24s <dir>\n", c);
            else       printf("  %-24s %6lu\n", c, (unsigned long)pb.hFileInfo.ioFlLgLen);
        }
        count++;
    }
    if (!count) printf("  (empty)\n");
}

int main(int argc, char **argv)
{
    int long_form = 0;
    const char *path = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'l' && argv[i][2] == 0) long_form = 1;
        else if (!path) path = argv[i];
    }

    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;

    /* No arg -> list $PWD (Unix-style "list the current directory"), or /
     * if nothing is set. Previously we listed /bin, which was surprising. */
    char pwd[96] = "";
    if (!path) {
        struct plusos_env_args e = { PLUSOS_ENV_GET, "PWD", pwd, sizeof pwd };
        plusos_syscall(SYS_getenv, &e);
        if (!pwd[0]) { pwd[0] = '/'; pwd[1] = 0; }
        path = pwd;
    }
    long dir = (path[0] == '/' && path[1] == 0) ? ROOT_DIR : walk_path(vref, path);
    if (!dir) { printf("ls: %s: not a directory\n", path); return 1; }
    list(vref, dir, path, long_form);
    return 0;
}
