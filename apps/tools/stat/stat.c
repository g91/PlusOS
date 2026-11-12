/* stat - display file or file system status. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

static void format_perm_bits(mode_t m, char out[11])
{
    out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISFIFO(m) ? 'p' :
             S_ISCHR(m) ? 'c' : S_ISBLK(m) ? 'b' : '-';
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_ISUID) ? ((m & S_IXUSR) ? 's' : 'S') : ((m & S_IXUSR) ? 'x' : '-');
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_ISGID) ? ((m & S_IXGRP) ? 's' : 'S') : ((m & S_IXGRP) ? 'x' : '-');
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_ISVTX) ? ((m & S_IXOTH) ? 't' : 'T') : ((m & S_IXOTH) ? 'x' : '-');
    out[10] = '\0';
}

static const char *type_word(mode_t m)
{
    if (S_ISDIR(m)) return "directory";
    if (S_ISLNK(m)) return "symbolic link";
    if (S_ISFIFO(m)) return "fifo";
    if (S_ISCHR(m)) return "character special file";
    if (S_ISBLK(m)) return "block special file";
    return "regular file";
}

static void uid_name(uid_t uid, char *out, size_t cap)
{
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND_UID; u.uid = (int)uid;
    if (plusos_syscall(SYS_users, &u) == 0 && u.name[0]) strlcpy(out, u.name, cap);
    else snprintf(out, cap, "%u", (unsigned)uid);
}

int main(int argc, char **argv)
{
    int deref = 0, terse = 0, i = 1;
    const char *fmt = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-L") == 0) { deref = 1; continue; }
        if (strcmp(argv[i], "-t") == 0) { terse = 1; continue; }
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) { fmt = argv[++i]; continue; }
        if (strncmp(argv[i], "--format=", 9) == 0) { fmt = argv[i] + 9; continue; }
    }
    if (i >= argc) { fprintf(stderr, "stat: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        struct stat st;
        if ((deref ? stat(argv[i], &st) : lstat(argv[i], &st)) != 0) {
            fprintf(stderr, "stat: cannot stat '%s': %s\n", argv[i], strerror(errno));
            rc = 1; continue;
        }
        char pstr[11], uname[24], gname[24];
        format_perm_bits(st.st_mode, pstr);
        uid_name(st.st_uid, uname, sizeof uname);
        uid_name(st.st_gid, gname, sizeof gname);
        if (fmt) {
            for (const char *p = fmt; *p; p++) {
                if (*p != '%') { putchar(*p); continue; }
                p++;
                switch (*p) {
                case 'n': printf("%s", argv[i]); break;
                case 's': printf("%lld", (long long)st.st_size); break;
                case 'f': printf("%x", (unsigned)st.st_mode); break;
                case 'a': printf("%o", (unsigned)(st.st_mode & 0777)); break;
                case 'A': printf("%s", pstr); break;
                case 'F': printf("%s", type_word(st.st_mode)); break;
                case 'u': printf("%u", (unsigned)st.st_uid); break;
                case 'U': printf("%s", uname); break;
                case 'g': printf("%u", (unsigned)st.st_gid); break;
                case 'G': printf("%s", gname); break;
                case 'i': printf("%lu", (unsigned long)st.st_ino); break;
                case 'h': printf("%u", (unsigned)st.st_nlink); break;
                case 'b': printf("%ld", (long)st.st_blocks); break;
                case 'B': printf("%ld", (long)st.st_blksize); break;
                case 'X': printf("%lu", (unsigned long)st.st_atime); break;
                case 'Y': printf("%lu", (unsigned long)st.st_mtime); break;
                case 'Z': printf("%lu", (unsigned long)st.st_ctime); break;
                case '%': putchar('%'); break;
                default: putchar('%'); putchar(*p); break;
                }
            }
            putchar('\n');
        } else if (terse) {
            printf("%s %lld %ld %x %u %u %lu %lu %u 0 0 %lu %lu %lu %ld\n",
                   argv[i], (long long)st.st_size, (long)st.st_blocks, (unsigned)st.st_mode,
                   (unsigned)st.st_uid, (unsigned)st.st_gid, (unsigned long)st.st_dev,
                   (unsigned long)st.st_ino, (unsigned)st.st_nlink,
                   (unsigned long)st.st_atime, (unsigned long)st.st_mtime, (unsigned long)st.st_ctime,
                   (long)st.st_blksize);
        } else {
            char tbuf[32];
            time_t mt = st.st_mtime;
            struct tm *tm = localtime(&mt);
            if (tm) strftime(tbuf, sizeof tbuf, "%Y-%m-%d %H:%M:%S", tm);
            else snprintf(tbuf, sizeof tbuf, "%lu", (unsigned long)mt);
            printf("  File: %s\n", argv[i]);
            printf("  Size: %-10lld Blocks: %-10ld IO Block: %-6ld %s\n",
                   (long long)st.st_size, (long)st.st_blocks, (long)st.st_blksize, type_word(st.st_mode));
            printf("Inode: %-11lu Links: %u\n", (unsigned long)st.st_ino, (unsigned)st.st_nlink);
            printf("Access: (%04o/%s)  Uid: (%u/%s)   Gid: (%u/%s)\n",
                   (unsigned)(st.st_mode & 07777), pstr, (unsigned)st.st_uid, uname,
                   (unsigned)st.st_gid, gname);
            printf("Modify: %s\n", tbuf);
        }
    }
    return rc;
}
