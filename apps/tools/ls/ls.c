/* ls - list directory contents. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

typedef struct { char name[64]; struct stat st; int has_st; } LsEntry;
typedef struct {
    int long_fmt, all, almost_all, one_col, human, si, rec, classify, slash_dir, inode, num_id, dir_self, show_size;
} LsOpts;

static int g_sort_mode = 0, g_rev = 0;

static void fmt_perm(mode_t m, char out[11])
{
    out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISFIFO(m) ? 'p' : S_ISCHR(m) ? 'c' : S_ISBLK(m) ? 'b' : '-';
    out[1] = (m & S_IRUSR) ? 'r' : '-'; out[2] = (m & S_IWUSR) ? 'w' : '-'; out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-'; out[5] = (m & S_IWGRP) ? 'w' : '-'; out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-'; out[8] = (m & S_IWOTH) ? 'w' : '-'; out[9] = (m & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

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

static void uid_name(uid_t uid, char *out, size_t cap)
{
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND_UID; u.uid = (int)uid;
    if (plusos_syscall(SYS_users, &u) == 0 && u.name[0]) strlcpy(out, u.name, cap);
    else snprintf(out, cap, "%u", (unsigned)uid);
}

static void join_p(char *out, size_t cap, const char *dir, const char *name)
{
    size_t d = strlen(dir);
    snprintf(out, cap, "%s%s%s", dir, (d && dir[d - 1] != '/' && dir[d - 1] != ':') ? "/" : "", name);
}

static int ls_cmp(const void *a, const void *b)
{
    const LsEntry *ea = a, *eb = b;
    int r = 0;
    if (g_sort_mode == 1) r = (ea->st.st_size < eb->st.st_size) ? 1 : (ea->st.st_size > eb->st.st_size) ? -1 : 0;
    else if (g_sort_mode == 2) r = (ea->st.st_mtime < eb->st.st_mtime) ? 1 : (ea->st.st_mtime > eb->st.st_mtime) ? -1 : 0;
    if (r == 0) r = strcasecmp(ea->name, eb->name);
    if (r == 0) r = strcmp(ea->name, eb->name);
    return g_rev ? -r : r;
}

static void print_entry(const char *fullpath, const LsEntry *e, const LsOpts *o)
{
    char suf = 0;
    if (o->classify) {
        if (S_ISDIR(e->st.st_mode)) suf = '/';
        else if (S_ISLNK(e->st.st_mode)) suf = '@';
        else if (S_ISFIFO(e->st.st_mode)) suf = '|';
        else if (e->st.st_mode & 0111) suf = '*';
    } else if (o->slash_dir && S_ISDIR(e->st.st_mode)) suf = '/';

    if (o->long_fmt) {
        char pstr[11], uname[20], gname[20], sbuf[16], tbuf[20];
        fmt_perm(e->st.st_mode, pstr);
        if (o->num_id) { snprintf(uname, sizeof uname, "%u", (unsigned)e->st.st_uid); snprintf(gname, sizeof gname, "%u", (unsigned)e->st.st_gid); }
        else { uid_name(e->st.st_uid, uname, sizeof uname); uid_name(e->st.st_gid, gname, sizeof gname); }
        if (o->human) fmt_human((unsigned long long)e->st.st_size, sbuf, sizeof sbuf, o->si);
        else snprintf(sbuf, sizeof sbuf, "%lld", (long long)e->st.st_size);
        time_t mt = e->st.st_mtime;
        struct tm *tm = localtime(&mt);
        if (tm) strftime(tbuf, sizeof tbuf, "%b %d %H:%M", tm); else strlcpy(tbuf, "Jan 01 00:00", sizeof tbuf);
        if (o->inode) printf("%6lu ", (unsigned long)e->st.st_ino);
        if (o->show_size) printf("%4ld ", (long)((e->st.st_size + 1023) / 1024));
        printf("%s %2u %-8s %-8s %7s %s %s", pstr, (unsigned)(e->st.st_nlink ? e->st.st_nlink : 1),
               uname, gname, sbuf, tbuf, e->name);
        if (suf) putchar(suf);
        if (S_ISLNK(e->st.st_mode) && fullpath) {
            char lt[128]; ssize_t ln = readlink(fullpath, lt, sizeof lt - 1);
            if (ln >= 0) { lt[ln] = '\0'; printf(" -> %s", lt); }
        }
        putchar('\n');
    } else {
        if (o->inode) printf("%lu ", (unsigned long)e->st.st_ino);
        if (o->show_size) printf("%ld ", (long)((e->st.st_size + 1023) / 1024));
        printf("%s", e->name);
        if (suf) putchar(suf);
    }
}

static int ls_dir(const char *path, const LsOpts *o, int print_header)
{
    DIR *d = opendir(path);
    if (!d) { fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno)); return 1; }
    int cap = 32, n = 0;
    LsEntry *ents = malloc((size_t)cap * sizeof *ents);
    if (!ents) { closedir(d); return 1; }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!o->all && !o->almost_all && de->d_name[0] == '.') continue;
        if (!o->all && o->almost_all && (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))) continue;
        if (n >= cap) {
            cap *= 2; LsEntry *ne = realloc(ents, (size_t)cap * sizeof *ents);
            if (!ne) break;
            ents = ne;
        }
        memset(&ents[n], 0, sizeof ents[n]);
        strlcpy(ents[n].name, de->d_name, sizeof ents[n].name);
        char *fp = malloc(192);
        if (fp) {
            join_p(fp, 192, path, de->d_name);
            if (lstat(fp, &ents[n].st) == 0) ents[n].has_st = 1; else ents[n].st.st_ino = de->d_ino;
            free(fp);
        }
        n++;
    }
    closedir(d);
    if (g_sort_mode != 3 && n > 1) qsort(ents, (size_t)n, sizeof *ents, ls_cmp);
    if (print_header) printf("%s:\n", path);
    if (o->long_fmt) {
        unsigned long total_kb = 0;
        for (int i = 0; i < n; i++) total_kb += (unsigned long)((ents[i].st.st_size + 1023) / 1024);
        printf("total %lu\n", total_kb);
        for (int i = 0; i < n; i++) {
            char *fp = malloc(192);
            if (fp) { join_p(fp, 192, path, ents[i].name); print_entry(fp, &ents[i], o); free(fp); }
        }
    } else if (o->one_col || !isatty(1)) {
        for (int i = 0; i < n; i++) { print_entry(NULL, &ents[i], o); putchar('\n'); }
    } else {
        int maxw = 0;
        for (int i = 0; i < n; i++) { int l = (int)strlen(ents[i].name) + 2; if (l > maxw) maxw = l; }
        if (maxw < 8) maxw = 8;
        int cols = 80 / maxw; if (cols < 1) cols = 1;
        for (int i = 0; i < n; i++) {
            print_entry(NULL, &ents[i], o);
            if ((i + 1) % cols == 0 || i + 1 == n) putchar('\n');
            else {
                int l = (int)strlen(ents[i].name) + (o->classify || o->slash_dir ? 1 : 0);
                for (int sp = l; sp < maxw; sp++) putchar(' ');
            }
        }
    }
    int rc = 0;
    if (o->rec) {
        for (int i = 0; i < n; i++) {
            if (!S_ISDIR(ents[i].st.st_mode) || !strcmp(ents[i].name, ".") || !strcmp(ents[i].name, "..")) continue;
            char *sub = malloc(192);
            if (!sub) break;
            join_p(sub, 192, path, ents[i].name);
            putchar('\n');
            rc |= ls_dir(sub, o, 1);
            free(sub);
        }
    }
    free(ents);
    return rc;
}

int main(int argc, char **argv)
{
    LsOpts o = { 0 };
    int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strncmp(argv[i], "--color", 7) == 0) continue;
        for (const char *p = argv[i] + 1; *p; p++) {
            switch (*p) {
            case 'l': o.long_fmt = 1; o.one_col = 0; break;
            case '1': o.one_col = 1; o.long_fmt = 0; break;
            case 'C': case 'x': o.one_col = 0; o.long_fmt = 0; break;
            case 'a': o.all = 1; break;
            case 'A': o.almost_all = 1; break;
            case 'h': o.human = 1; break;
            case 'H': o.human = 1; o.si = 1; break;
            case 'R': o.rec = 1; break;
            case 'r': g_rev = 1; break;
            case 'S': g_sort_mode = 1; break;
            case 't': g_sort_mode = 2; break;
            case 'U': case 'f': g_sort_mode = 3; if (*p == 'f') o.all = 1; break;
            case 'F': o.classify = 1; break;
            case 'p': o.slash_dir = 1; break;
            case 'i': o.inode = 1; break;
            case 'n': o.num_id = 1; o.long_fmt = 1; break;
            case 'd': o.dir_self = 1; break;
            case 's': o.show_size = 1; break;
            default: break;
            }
        }
    }
    int narg = argc - i;
    if (narg <= 0) return ls_dir(".", &o, o.rec);
    int rc = 0;
    for (int k = i; k < argc; k++) {
        struct stat st;
        if (lstat(argv[k], &st) != 0) { fprintf(stderr, "ls: cannot access '%s': %s\n", argv[k], strerror(errno)); rc = 1; continue; }
        if (S_ISDIR(st.st_mode) && !o.dir_self) { if (k > i) putchar('\n'); rc |= ls_dir(argv[k], &o, narg > 1 || o.rec); }
        else { LsEntry e = { 0 }; strlcpy(e.name, argv[k], sizeof e.name); e.st = st; e.has_st = 1; print_entry(argv[k], &e, &o); if (!o.long_fmt) putchar('\n'); }
    }
    return rc;
}
