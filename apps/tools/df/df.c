/* df - report file system disk space usage. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>

static void fmt_human(unsigned long long bytes, char *out, size_t cap, int si)
{
    static const char *const u[] = { "", "K", "M", "G", "T" };
    unsigned long base = si ? 1000UL : 1024UL;
    if (bytes < base) { snprintf(out, cap, "%llu", bytes); return; }
    int idx = 0; unsigned long long w = bytes; unsigned long rem = 0;
    while (w >= base && idx < 4) { rem = (unsigned long)(w % base); w /= base; idx++; }
    if (w < 10) snprintf(out, cap, "%llu.%lu%s", w, (rem * 10UL) / base, u[idx]);
    else snprintf(out, cap, "%llu%s", w, u[idx]);
}

int main(int argc, char **argv)
{
    int human = 0, si = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) human = 1;
        else if (strcmp(argv[i], "-H") == 0) { human = 1; si = 1; }
    }
    struct plusos_statfs sfs = { 0 };
    struct plusos_vfs_args va = { 0 };
    va.op = PLUSOS_VFS_STATFS; va.path = "/"; va.buf = &sfs;
    if (plusos_syscall(SYS_vfs, &va) != 0) { perror("df"); return 1; }
    unsigned long long bsz = sfs.f_bsize ? sfs.f_bsize : 512ULL;
    unsigned long long tot = (unsigned long long)sfs.f_blocks * bsz;
    unsigned long long avail = (unsigned long long)sfs.f_bavail * bsz;
    unsigned long long used = tot > avail ? tot - avail : 0;
    unsigned pct = tot ? (unsigned)((used * 100ULL + tot - 1) / tot) : 0;
    if (human) {
        char st[16], su[16], sa[16];
        fmt_human(tot, st, sizeof st, si);
        fmt_human(used, su, sizeof su, si);
        fmt_human(avail, sa, sizeof sa, si);
        printf("Filesystem      Size  Used Avail Use%% Mounted on\n");
        printf("%-14s %5s %5s %5s %3u%% /\n", sfs.f_volname[0] ? sfs.f_volname : "hfs0", st, su, sa, pct);
    } else {
        printf("Filesystem     1K-blocks      Used Available Use%% Mounted on\n");
        printf("%-14s %9llu %9llu %9llu %3u%% /\n",
               sfs.f_volname[0] ? sfs.f_volname : "hfs0", tot / 1024ULL, used / 1024ULL, avail / 1024ULL, pct);
    }
    return 0;
}
