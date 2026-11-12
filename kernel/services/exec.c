/* exec.c - load and run PlusOS programs (.pex).
 *
 * Programs are looked up as files on the boot volume through the ROM File
 * Manager (copy them there from Windows with the disk tool or `make install`),
 * then in the raw app store that install.py writes (works even without a file
 * system). The image is read into a block from the ROM heap and called as
 *     int entry(int argc, char **argv, const PlusOSAPI *api)
 * Its crt0 relocates it for that address before running main().
 */
#include "kernel.h"
#include "proc.h"
#include "posix.h"
#include "vfs.h"
#include "environ.h"

/* loader errors live outside the range of normal program exit codes */
enum { E_NOTFOUND = -1001, E_FORMAT = -1002, E_NOMEM = -1003, E_READ = -1004 };

const char *exec_error(int code)
{
    switch (code) {
    case E_NOTFOUND: return "not found";
    case E_FORMAT:   return "not a PlusOS program";
    case E_NOMEM:    return "out of memory";
    case E_READ:     return "disk read error";
    default:         return "error";
    }
}

/* 68020/030/040+: freshly loaded code must not run from stale cache lines. */
void cache_flush(void)
{
    uint8_t cpu = LM_BYTE(0x12F);
    if (cpu >= 4) {
        __asm__ volatile(".short 0x4E71,0xF4F8" ::: "memory");  /* nop; cpusha %bc */
    } else if (cpu >= 2) {
        __asm__ volatile(".short 0x4E7A,0x0002\n\t"             /* movec cacr,d0 */
                         "ori.w #0x0808,%%d0\n\t"
                         ".short 0x4E7B,0x0002"                 /* movec d0,cacr */
                         ::: "d0", "memory");
    }
}

static int check_header(const PexHeader *h)
{
    return h->magic == PEX_MAGIC && h->format == PEX_FORMAT && h->kind == PEX_KIND_APP &&
           h->file_size >= PEX_HEADER_SIZE && h->mem_size >= h->file_size;
}

static int has_colon(const char *s) { for (; *s; s++) if (*s == ':') return 1; return 0; }
static int has_slash(const char *s) { for (; *s; s++) if (*s == '/') return 1; return 0; }

static void posix_to_hfs_name(const char *posix, char *hfs, int cap)
{
    int j = 0;
    if (posix[0] == '/') {
        if (j < cap - 1) hfs[j++] = ':';
        for (int i = 1; posix[i] && j < cap - 1; i++)
            hfs[j++] = (posix[i] == '/') ? ':' : posix[i];
    } else {
        for (int i = 0; posix[i] && j < cap - 1; i++)
            hfs[j++] = (posix[i] == '/') ? ':' : posix[i];
    }
    hfs[j] = 0;
}

static int try_open_posix_path(const char *ppath, INTEGER *ref)
{
    char resolved[128], hpath[128];
    Str255 pname;
    vfs_normalize_path(ppath, resolved, sizeof resolved, 1);
    posix_to_hfs_name(resolved, hpath, sizeof hpath);
    c2pstr_buf(pname, hpath);
    return FSOpen(pname, g_boot_vref, ref) == 0 ? 0 : -1;
}

static int open_prog(const char *name, INTEGER *ref)
{
    Str255 pname;
    if (!has_colon(name)) {
        if (has_slash(name)) {
            if (try_open_posix_path(name, ref) == 0) return 0;
        } else {
            char cand[128], path_env[96];
            if (env_get("PATH", path_env, sizeof path_env) > 0 && path_env[0]) {
                const char *p = path_env;
                while (*p) {
                    const char *sep = strchr(p, ':');
                    int dlen = sep ? (int)(sep - p) : (int)strlen(p);
                    if (dlen > 0 && dlen < 90) {
                        snprintf(cand, sizeof cand, "%.*s/%s", dlen, p, name);
                        if (try_open_posix_path(cand, ref) == 0) return 0;
                    }
                    if (!sep) break;
                    p = sep + 1;
                }
            }
            snprintf(cand, sizeof cand, "/bin/%s", name);
            if (try_open_posix_path(cand, ref) == 0) return 0;
        }
    }
    c2pstr_buf(pname, name);
    return FSOpen(pname, g_boot_vref, ref) == 0 ? 0 : -1;
}

static int load_hfs(const char *name, uint8_t **out)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return E_NOTFOUND;
    INTEGER ref;
    if (open_prog(name, &ref) != 0) return E_NOTFOUND;
    PexHeader h;
    LONGINT n = sizeof h;
    int rc = E_READ;
    if (FSRead(ref, &n, &h) == 0 && n == (LONGINT)sizeof h) {
        rc = E_FORMAT;
        if (check_header(&h)) {
            rc = E_NOMEM;
            uint8_t *mem = kmalloc(h.mem_size);
            if (mem) {
                n = (LONGINT)h.file_size;
                if (SetFPos(ref, 1, 0) == 0 && FSRead(ref, &n, mem) == 0 && n == (LONGINT)h.file_size) {
                    *out = mem;
                    rc = 0;
                } else {
                    kfree(mem);
                    rc = E_READ;
                }
            }
        }
    }
    FSClose(ref);
    return rc;
}

static int load_raw(const char *name, uint8_t **out)
{
    uint8_t *dir = kmalloc(2048);                 /* 4-block directory, up to 85 entries */
    if (!dir) return E_NOMEM;
    int rc = E_NOTFOUND;
    if (blk_read(APPS_BLOCK, 4, dir) == 0 && !memcmp(dir, "PLDR", 4)) {
        uint32_t count = *(uint32_t *)(dir + 4);
        for (uint32_t i = 0; i < count && i < 85; i++) {
            uint8_t *e = dir + 8 + i * 24;
            if (strncasecmp((char *)e, name, 16) != 0) continue;
            uint32_t start = *(uint32_t *)(e + 16), size = *(uint32_t *)(e + 20);
            uint32_t blocks = (size + 511) / 512;
            uint8_t first[512];
            rc = E_READ;
            if (blk_read(start, 1, first) != 0) break;
            const PexHeader *h = (const PexHeader *)first;
            rc = E_FORMAT;
            if (!check_header(h)) break;
            uint32_t need = h->mem_size > blocks * 512 ? h->mem_size : blocks * 512;
            uint8_t *mem = kmalloc(need);
            rc = E_NOMEM;
            if (!mem) break;
            if (blk_read(start, blocks, mem) != 0) { kfree(mem); rc = E_READ; break; }
            *out = mem;
            rc = 0;
            break;
        }
    }
    kfree(dir);
    return rc;
}

int exec_run(const char *name, int argc, char **argv)
{
    uint8_t *mem = 0;
    int rc = load_hfs(name, &mem);
    if (rc == E_NOTFOUND)
        rc = load_raw(name, &mem);
    if (rc == E_NOTFOUND || rc == E_FORMAT) {
        int mrc = mac_try_run(name, argc, argv);
        if (mrc != E_NOTFOUND) return mrc;
    }
    if (rc) return rc;

    cache_flush();
    typedef int (*entry_fn)(int, char **, const PlusOSAPI *);
    entry_fn entry = (entry_fn)mem;
    int result = entry(argc, argv, &g_api);
    kfree(mem);
    return result;
}

#if CONFIG_MACLOADER
static void mac_proc_runner(void *arg)
{
    char *name = (char *)arg;
    char *argv[2] = { name, 0 };
    mac_try_run(name, 1, argv);
    kfree(name);
}
#endif

/* Load a program and run it as a ring-2 (user mode) process, or as a classic
 * Macintosh 'APPL' ('CODE' resource) process. Returns the new pid, or a
 * negative E_* loader error. */
int exec_spawn(const char *name, int argc, char **argv)
{
    return exec_spawn_fds(name, argc, argv, 0);
}

int exec_spawn_fds(const char *name, int argc, char **argv, const int fds_override[3])
{
    uint8_t *mem = 0;
    int rc = load_hfs(name, &mem);
    if (rc == E_NOTFOUND)
        rc = load_raw(name, &mem);
#if CONFIG_MACLOADER
    if (rc == E_NOTFOUND || rc == E_FORMAT) {
        /* Check if `name` is a classic Macintosh 'APPL' ('CODE' resource) */
        INTEGER rf_ref;
        Str255 pname;
        c2pstr_buf(pname, name);
        int has_rf = 0;
        if (g_flags & PLUSOS_HAVE_FS) {
            if (OpenRF(pname, g_boot_vref, &rf_ref) == 0) {
                LONGINT eof = 0;
                if (GetEOF(rf_ref, &eof) == 0 && eof >= 64) has_rf = 1;
                FSClose(rf_ref);
            }
            if (!has_rf && rc == E_FORMAT) has_rf = 1;
        }
        if (has_rf) {
            char *ncopy = kmalloc(64);
            if (!ncopy) return E_NOMEM;
            strlcpy(ncopy, name, 64);
            const char *leaf = name;
            for (const char *s = name; *s; s++) if (*s == ':') leaf = s + 1;
            int pid = proc_create_kernel(leaf, mac_proc_runner, ncopy);
            if (pid < 0) { kfree(ncopy); return E_NOMEM; }
            return pid;
        }
    }
#endif
    if (rc) return rc;

    cache_flush();                      /* kernel flushes (supervisor) so the app
                                           need not run a privileged movec */
    const char *leaf = name;                      /* ":Desktop:hello" -> "hello" */
    for (const char *s = name; *s; s++) if (*s == ':') leaf = s + 1;
    int pid = proc_create_user_fds(leaf, mem, 0, argc, argv, fds_override);
    if (pid < 0) { kfree(mem); return E_NOMEM; }
    for (int i = 0; i < 16; i++) {                /* record image size for the task manager */
        Proc *p = proc_table_entry(i);
        if (p->pid == pid) { p->mem_bytes += ((const PexHeader *)mem)->mem_size; break; }
    }
    return pid;
}
