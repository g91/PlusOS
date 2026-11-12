/* posix.c - PlusOS POSIX C runtime wrappers (VFS, FILE*, time, getopt, signals) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <utime.h>
#include <signal.h>
#include <termios.h>
#include <stdarg.h>
#include <plusos/syscall.h>

int errno = 0;

char *optarg = 0;
int   optind = 1;
int   opterr = 1;
int   optopt = '?';

static long vfs_call(struct plusos_vfs_args *a)
{
    long rc = plusos_syscall(SYS_vfs, a);
    if (rc < 0) { errno = (int)(-rc); return -1; }
    return rc;
}

static long posix_call(struct plusos_posix_args *a)
{
    long rc = plusos_syscall(SYS_posix, a);
    if (rc < 0) { errno = (int)(-rc); return -1; }
    return rc;
}

/* ---- Low-level POSIX I/O ------------------------------------------------- */
int open(const char *path, int flags, ...)
{
    mode_t mode = 0644;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, unsigned int);
        va_end(ap);
    }
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_OPEN;
    a.path = path;
    a.flags = flags;
    a.mode = mode;
    return (int)vfs_call(&a);
}

int close(int fd)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_CLOSE;
    a.fd = fd;
    return (int)vfs_call(&a);
}

ssize_t read(int fd, void *buf, size_t count)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_READ;
    a.fd = fd;
    a.buf = buf;
    a.count = (int32_t)count;
    return (ssize_t)vfs_call(&a);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_WRITE;
    a.fd = fd;
    a.buf = (void *)buf;
    a.count = (int32_t)count;
    return (ssize_t)vfs_call(&a);
}

off_t lseek(int fd, off_t offset, int whence)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_LSEEK;
    a.fd = fd;
    a.offset = offset;
    a.flags = whence;
    return (off_t)vfs_call(&a);
}

int dup(int oldfd)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_DUP;
    a.fd = oldfd;
    return (int)vfs_call(&a);
}

int dup2(int oldfd, int newfd)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_DUP2;
    a.fd = oldfd;
    a.fd2 = newfd;
    return (int)vfs_call(&a);
}

int pipe(int fds[2])
{
    if (!fds) { errno = EINVAL; return -1; }
    struct plusos_posix_args a = { 0 };
    a.op = PLUSOS_POSIX_PIPE;
    if (posix_call(&a) < 0) return -1;
    fds[0] = a.a;
    fds[1] = a.b;
    return 0;
}

int isatty(int fd)
{
    struct plusos_posix_args a = { 0 };
    a.op = PLUSOS_POSIX_ISATTY;
    a.a = fd;
    long r = plusos_syscall(SYS_posix, &a);
    return (r > 0) ? 1 : 0;
}

/* ---- Filesystem metadata & operations ------------------------------------ */
static void copy_stat(struct stat *dst, const struct plusos_stat *src)
{
    dst->st_dev     = src->st_dev;
    dst->st_ino     = src->st_ino;
    dst->st_mode    = src->st_mode;
    dst->st_nlink   = src->st_nlink;
    dst->st_uid     = src->st_uid;
    dst->st_gid     = src->st_gid;
    dst->st_rdev    = src->st_rdev;
    dst->st_size    = src->st_size;
    dst->st_blksize = src->st_blksize;
    dst->st_blocks  = src->st_blocks;
    dst->st_atime   = (time_t)src->st_atime;
    dst->st_mtime   = (time_t)src->st_mtime;
    dst->st_ctime   = (time_t)src->st_ctime;
}

int stat(const char *path, struct stat *st)
{
    if (!st) { errno = EINVAL; return -1; }
    struct plusos_stat kst;
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_STAT;
    a.path = path;
    a.buf = &kst;
    if (vfs_call(&a) < 0) return -1;
    copy_stat(st, &kst);
    return 0;
}

int lstat(const char *path, struct stat *st)
{
    if (!st) { errno = EINVAL; return -1; }
    struct plusos_stat kst;
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_LSTAT;
    a.path = path;
    a.buf = &kst;
    if (vfs_call(&a) < 0) return -1;
    copy_stat(st, &kst);
    return 0;
}

int fstat(int fd, struct stat *st)
{
    if (!st) { errno = EINVAL; return -1; }
    struct plusos_stat kst;
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_FSTAT;
    a.fd = fd;
    a.buf = &kst;
    if (vfs_call(&a) < 0) return -1;
    copy_stat(st, &kst);
    return 0;
}

int mkdir(const char *path, mode_t mode)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_MKDIR;
    a.path = path;
    a.mode = mode;
    return (int)vfs_call(&a);
}

int rmdir(const char *path)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_RMDIR;
    a.path = path;
    return (int)vfs_call(&a);
}

int unlink(const char *path)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_UNLINK;
    a.path = path;
    return (int)vfs_call(&a);
}

int remove(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return rmdir(path);
    return unlink(path);
}

int rename(const char *oldpath, const char *newpath)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_RENAME;
    a.path = oldpath;
    a.path2 = newpath;
    return (int)vfs_call(&a);
}

int chdir(const char *path)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_CHDIR;
    a.path = path;
    return (int)vfs_call(&a);
}

char *getcwd(char *buf, size_t size)
{
    if (!buf) {
        size = 128;
        buf = (char *)malloc(size);
        if (!buf) { errno = ENOMEM; return 0; }
    }
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_GETCWD;
    a.buf = buf;
    a.count = (int32_t)size;
    if (vfs_call(&a) < 0) return 0;
    return buf;
}

int chroot(const char *path)
{
    struct plusos_posix_args a = { 0 };
    a.op = PLUSOS_POSIX_CHROOT;
    a.name = path;
    return (int)posix_call(&a);
}

int link(const char *target, const char *linkpath)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_LINK;
    a.path = target;
    a.path2 = linkpath;
    return (int)vfs_call(&a);
}

int symlink(const char *target, const char *linkpath)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_SYMLINK;
    a.path = target;
    a.path2 = linkpath;
    return (int)vfs_call(&a);
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_READLINK;
    a.path = path;
    a.buf = buf;
    a.count = (int32_t)bufsiz;
    return (ssize_t)vfs_call(&a);
}

int truncate(const char *path, off_t length)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_TRUNCATE;
    a.path = path;
    a.offset = length;
    return (int)vfs_call(&a);
}

int ftruncate(int fd, off_t length)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_FTRUNCATE;
    a.fd = fd;
    a.offset = length;
    return (int)vfs_call(&a);
}

int chmod(const char *path, mode_t mode)
{
    struct plusos_perms_args p;
    memset(&p, 0, sizeof p);
    p.op = PLUSOS_PERMS_CHMOD;
    p.mode = (int)(mode & 0777);
    strlcpy(p.path, path, sizeof p.path);
    long rc = plusos_syscall(SYS_perms, &p);
    if (rc < 0) { errno = EACCES; return -1; }
    return 0;
}

int chown(const char *path, uid_t uid, gid_t gid)
{
    struct plusos_perms_args p;
    memset(&p, 0, sizeof p);
    p.op = PLUSOS_PERMS_CHOWN;
    p.uid = (int)uid;
    p.gid = (int)gid;
    strlcpy(p.path, path, sizeof p.path);
    long rc = plusos_syscall(SYS_perms, &p);
    if (rc < 0) { errno = EACCES; return -1; }
    return 0;
}

int access(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) < 0) return -1;
    if (mode == F_OK) return 0;
    struct plusos_perms_args p;
    memset(&p, 0, sizeof p);
    p.op = PLUSOS_PERMS_CHECK;
    p.want = mode & 7;
    strlcpy(p.path, path, sizeof p.path);
    if (plusos_syscall(SYS_perms, &p) == 0) { errno = EACCES; return -1; }
    return 0;
}

int mkfifo(const char *path, mode_t mode)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_MKFIFO;
    a.path = path;
    a.mode = mode;
    return (int)vfs_call(&a);
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    (void)dev;
    if ((mode & S_IFMT) == S_IFIFO || mode == 0) return mkfifo(path, mode ? mode : 0644);
    errno = EPERM;
    return -1;
}

mode_t umask(mode_t mask)
{
    struct plusos_posix_args a = { 0 };
    a.op = PLUSOS_POSIX_UMASK;
    a.a = (int)(mask & 0777);
    return (mode_t)plusos_syscall(SYS_posix, &a);
}

void sync(void)
{
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_SYNC;
    plusos_syscall(SYS_vfs, &a);
}

int utime(const char *filename, const struct utimbuf *times)
{
    time_t now = time(0);
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_UTIME;
    a.path = filename;
    a.t1 = times ? (uint32_t)times->actime : (uint32_t)now;
    a.t2 = times ? (uint32_t)times->modtime : (uint32_t)now;
    return (int)vfs_call(&a);
}

/* ---- Directory reading --------------------------------------------------- */
DIR *opendir(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0) return 0;
    if (!S_ISDIR(st.st_mode)) { errno = ENOTDIR; return 0; }
    DIR *d = (DIR *)calloc(1, sizeof(DIR));
    if (!d) { errno = ENOMEM; return 0; }
    strlcpy(d->path, path, sizeof d->path);
    d->index = 0;
    return d;
}

struct dirent *readdir(DIR *dirp)
{
    if (!dirp) { errno = EBADF; return 0; }
    struct plusos_dirent kde;
    struct plusos_vfs_args a = { 0 };
    a.op = PLUSOS_VFS_READDIR;
    a.path = dirp->path;
    a.offset = dirp->index;
    a.buf = &kde;
    if (plusos_syscall(SYS_vfs, &a) < 0) return 0;
    dirp->index++;
    dirp->cur.d_ino = kde.d_ino;
    dirp->cur.d_type = kde.d_type;
    strlcpy(dirp->cur.d_name, kde.d_name, sizeof dirp->cur.d_name);
    return &dirp->cur;
}

int closedir(DIR *dirp)
{
    if (!dirp) { errno = EBADF; return -1; }
    free(dirp);
    return 0;
}

void rewinddir(DIR *dirp)
{
    if (dirp) dirp->index = 0;
}

/* ---- Buffered FILE* stdio ------------------------------------------------ */
#define MAX_STD_FILES 20
struct __plusos_file {
    int     fd;
    uint8_t used;
    uint8_t eof;
    uint8_t err;
    uint8_t ungot_valid;
    uint8_t ungot;
    uint8_t wr;
    uint8_t own_fd;
};

static struct __plusos_file g_std_pool[MAX_STD_FILES] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 }, /* stdin */
    { 1, 1, 0, 0, 0, 0, 1, 0 }, /* stdout */
    { 2, 1, 0, 0, 0, 0, 1, 0 }  /* stderr */
};

FILE *const stdin  = &g_std_pool[0];
FILE *const stdout = &g_std_pool[1];
FILE *const stderr = &g_std_pool[2];

static FILE *alloc_std_file(int fd, int wr, int own)
{
    for (int i = 3; i < MAX_STD_FILES; i++) {
        if (!g_std_pool[i].used) {
            memset(&g_std_pool[i], 0, sizeof g_std_pool[i]);
            g_std_pool[i].fd = fd;
            g_std_pool[i].used = 1;
            g_std_pool[i].wr = (uint8_t)wr;
            g_std_pool[i].own_fd = (uint8_t)own;
            return &g_std_pool[i];
        }
    }
    errno = EMFILE;
    return 0;
}

FILE *fdopen(int fd, const char *mode)
{
    if (fd < 0 || !mode) { errno = EINVAL; return 0; }
    int wr = (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'));
    return alloc_std_file(fd, wr, 1);
}

FILE *fopen(const char *path, const char *mode)
{
    if (!path || !mode) { errno = EINVAL; return 0; }
    int plus = (strchr(mode, '+') != 0);
    int flags = O_RDONLY;
    int wr = 0;
    if (mode[0] == 'r') {
        flags = plus ? O_RDWR : O_RDONLY;
        wr = plus;
    } else if (mode[0] == 'w') {
        flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
        wr = 1;
    } else if (mode[0] == 'a') {
        flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
        wr = 1;
    } else {
        errno = EINVAL;
        return 0;
    }
    int fd = open(path, flags, 0644);
    if (fd < 0) return 0;
    FILE *f = alloc_std_file(fd, wr, 1);
    if (!f) { close(fd); return 0; }
    return f;
}

FILE *freopen(const char *path, const char *mode, FILE *stream)
{
    if (!stream) return fopen(path, mode);
    if (stream->own_fd && stream->fd > 2) close(stream->fd);
    FILE *nf = fopen(path, mode);
    if (!nf) return 0;
    int newfd = nf->fd;
    nf->used = 0;
    if (dup2(newfd, stream->fd) < 0) { close(newfd); return 0; }
    close(newfd);
    stream->eof = stream->err = stream->ungot_valid = 0;
    return stream;
}

int fclose(FILE *stream)
{
    if (!stream || !stream->used) { errno = EBADF; return EOF; }
    int rc = 0;
    if (stream->own_fd) rc = close(stream->fd);
    if (stream >= &g_std_pool[3]) stream->used = 0;
    return rc < 0 ? EOF : 0;
}

int fflush(FILE *stream)
{
    (void)stream;
    return 0;
}

int fileno(FILE *stream) { return stream ? stream->fd : -1; }
int feof(FILE *stream) { return stream ? stream->eof : 0; }
int ferror(FILE *stream) { return stream ? stream->err : 0; }
void clearerr(FILE *stream) { if (stream) { stream->eof = 0; stream->err = 0; } }

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!stream || !ptr || !size || !nmemb) return 0;
    size_t want = size * nmemb;
    size_t total = 0;
    uint8_t *dst = (uint8_t *)ptr;
    if (stream->ungot_valid && want > 0) {
        *dst++ = stream->ungot;
        stream->ungot_valid = 0;
        total++;
    }
    while (total < want) {
        size_t chunk = want - total;
        if (chunk > 512) chunk = 512;
        ssize_t n = read(stream->fd, dst + total, chunk);
        if (n < 0) { stream->err = 1; break; }
        if (n == 0) { stream->eof = 1; break; }
        total += (size_t)n;
    }
    return total / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!stream || !ptr || !size || !nmemb) return 0;
    size_t want = size * nmemb;
    size_t total = 0;
    const uint8_t *src = (const uint8_t *)ptr;
    while (total < want) {
        size_t chunk = want - total;
        if (chunk > 512) chunk = 512;
        ssize_t n = write(stream->fd, src + total, chunk);
        if (n <= 0) { stream->err = 1; break; }
        total += (size_t)n;
    }
    return total / size;
}

int fgetc(FILE *stream)
{
    uint8_t c;
    if (fread(&c, 1, 1, stream) == 1) return c;
    return EOF;
}

int ungetc(int c, FILE *stream)
{
    if (!stream || c == EOF || stream->ungot_valid) return EOF;
    stream->ungot = (uint8_t)c;
    stream->ungot_valid = 1;
    stream->eof = 0;
    return (unsigned char)c;
}

char *fgets(char *s, int size, FILE *stream)
{
    if (!s || size <= 0 || !stream) return 0;
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return 0;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = 0;
    return s;
}

int fputc(int c, FILE *stream)
{
    uint8_t ch = (uint8_t)c;
    return (fwrite(&ch, 1, 1, stream) == 1) ? ch : EOF;
}

int fputs(const char *s, FILE *stream)
{
    if (!s || !stream) return EOF;
    size_t len = strlen(s);
    return (fwrite(s, 1, len, stream) == len) ? 0 : EOF;
}

long getline(char **lineptr, size_t *n, FILE *stream)
{
    if (!lineptr || !n || !stream) { errno = EINVAL; return -1; }
    size_t cap = (*lineptr && *n) ? *n : 128;
    char *buf = *lineptr ? *lineptr : (char *)malloc(cap);
    if (!buf) { errno = ENOMEM; return -1; }
    size_t len = 0;
    for (;;) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (len == 0) { *lineptr = buf; *n = cap; return -1; }
            break;
        }
        if (len + 2 >= cap) {
            size_t ncap = cap * 2;
            char *nb = (char *)realloc(buf, ncap);
            if (!nb) { errno = ENOMEM; *lineptr = buf; *n = cap; return -1; }
            buf = nb;
            cap = ncap;
        }
        buf[len++] = (char)c;
        if (c == '\n') break;
    }
    buf[len] = 0;
    *lineptr = buf;
    *n = cap;
    return (long)len;
}

typedef void (*out_fn)(int c, void *ctx);
extern int __plusos_vformat(out_fn out, void *ctx, const char *fmt, va_list ap);

struct file_fmt_ctx {
    FILE  *f;
    char   buf[128];
    int    n;
};

static void file_flush_ctx(struct file_fmt_ctx *fc)
{
    if (fc->n > 0) {
        fwrite(fc->buf, 1, (size_t)fc->n, fc->f);
        fc->n = 0;
    }
}

static void file_out(int c, void *ctx)
{
    struct file_fmt_ctx *fc = (struct file_fmt_ctx *)ctx;
    fc->buf[fc->n++] = (char)c;
    if (fc->n >= (int)sizeof fc->buf || c == '\n') file_flush_ctx(fc);
}

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
    if (!stream) { errno = EBADF; return -1; }
    struct file_fmt_ctx fc;
    fc.f = stream;
    fc.n = 0;
    int r = __plusos_vformat(file_out, &fc, fmt, ap);
    file_flush_ctx(&fc);
    return r;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int fseek(FILE *stream, long offset, int whence)
{
    if (!stream) { errno = EBADF; return -1; }
    stream->ungot_valid = 0;
    stream->eof = 0;
    return (lseek(stream->fd, (off_t)offset, whence) < 0) ? -1 : 0;
}

long ftell(FILE *stream)
{
    if (!stream) { errno = EBADF; return -1; }
    long p = (long)lseek(stream->fd, 0, SEEK_CUR);
    if (p >= 0 && stream->ungot_valid && p > 0) p--;
    return p;
}

void rewind(FILE *stream)
{
    if (stream) { clearerr(stream); fseek(stream, 0, SEEK_SET); }
}

const char *strerror(int errnum)
{
    switch (errnum) {
    case 0: return "Success";
    case EPERM: return "Operation not permitted";
    case ENOENT: return "No such file or directory";
    case ESRCH: return "No such process";
    case EINTR: return "Interrupted system call";
    case EIO: return "Input/output error";
    case EBADF: return "Bad file descriptor";
    case ENOMEM: return "Cannot allocate memory";
    case EACCES: return "Permission denied";
    case EEXIST: return "File exists";
    case ENODEV: return "No such device";
    case ENOTDIR: return "Not a directory";
    case EISDIR: return "Is a directory";
    case EINVAL: return "Invalid argument";
    case EMFILE: return "Too many open files";
    case ENOSPC: return "No space left on device";
    case ESPIPE: return "Illegal seek";
    case EPIPE: return "Broken pipe";
    case ENOTEMPTY: return "Directory not empty";
    case ENOSYS: return "Function not implemented";
    default: return "Unknown error";
    }
}

void perror(const char *s)
{
    if (s && s[0]) fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else fprintf(stderr, "%s\n", strerror(errno));
}

/* ---- Process, Identity, Env, Time & Termios ------------------------------ */
unsigned int sleep(unsigned int seconds)
{
    if (seconds > 0) plusos_syscall(SYS_sleep, (void *)(intptr_t)(seconds * 60u));
    return 0;
}

int usleep(unsigned long usec)
{
    unsigned long ticks = (usec + 16665UL) / 16666UL;
    if (ticks < 1 && usec > 0) ticks = 1;
    if (ticks > 0) plusos_syscall(SYS_sleep, (void *)(intptr_t)ticks);
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (rem) { rem->tv_sec = 0; rem->tv_nsec = 0; }
    if (!req) { errno = EINVAL; return -1; }
    unsigned long ticks = (unsigned long)req->tv_sec * 60UL + (unsigned long)(req->tv_nsec / 16666667L);
    if (ticks == 0 && (req->tv_sec > 0 || req->tv_nsec > 0)) ticks = 1;
    if (ticks > 0) plusos_syscall(SYS_sleep, (void *)(intptr_t)ticks);
    return 0;
}

pid_t getpid(void)  { return (pid_t)plusos_syscall(SYS_getpid, 0); }
pid_t getppid(void) { struct plusos_posix_args a = { PLUSOS_POSIX_GETPPID }; return (pid_t)plusos_syscall(SYS_posix, &a); }
uid_t getuid(void)  { struct plusos_posix_args a = { PLUSOS_POSIX_GETUID }; return (uid_t)plusos_syscall(SYS_posix, &a); }
uid_t geteuid(void) { return getuid(); }
gid_t getgid(void)  { struct plusos_posix_args a = { PLUSOS_POSIX_GETGID }; return (gid_t)plusos_syscall(SYS_posix, &a); }
gid_t getegid(void) { return getgid(); }

int gethostname(char *name, size_t len)
{
    struct plusos_posix_args a = { PLUSOS_POSIX_GETHOSTNAME, 0, 0, 0, 0, 0, 0, {-1,-1,-1}, name, (int32_t)len, 0 };
    return (int)posix_call(&a);
}

int sethostname(const char *name, size_t len)
{
    (void)len;
    struct plusos_posix_args a = { PLUSOS_POSIX_SETHOSTNAME, 0, 0, 0, name, 0, 0, {-1,-1,-1}, 0, 0, 0 };
    return (int)posix_call(&a);
}

long gethostid(void)
{
    struct plusos_posix_args a = { PLUSOS_POSIX_HOSTID };
    return plusos_syscall(SYS_posix, &a);
}

int uname(struct utsname *buf)
{
    if (!buf) { errno = EINVAL; return -1; }
    struct plusos_utsname ku;
    struct plusos_posix_args a = { PLUSOS_POSIX_UNAME, 0, 0, 0, 0, 0, 0, {-1,-1,-1}, &ku, sizeof ku, 0 };
    if (posix_call(&a) < 0) return -1;
    memcpy(buf, &ku, sizeof *buf);
    return 0;
}

int nice(int inc)
{
    struct plusos_prio_args pa = { PLUSOS_PRIO_GET, 0, 0 };
    int cur = (int)plusos_syscall(SYS_priority, &pa);
    int nxt = cur + inc;
    if (nxt < -20) nxt = -20;
    if (nxt > 19) nxt = 19;
    pa.op = PLUSOS_PRIO_SET;
    pa.priority = nxt;
    plusos_syscall(SYS_priority, &pa);
    return nxt;
}

int kill(pid_t pid, int sig)
{
    struct plusos_posix_args a = { PLUSOS_POSIX_KILL_SIG, (int)pid, sig };
    return (int)posix_call(&a);
}

unsigned int alarm(unsigned int seconds)
{
    return (unsigned int)plusos_syscall(SYS_alarm, (void *)(uintptr_t)seconds);
}

int tcgetattr(int fd, struct termios *tp)
{
    if (!isatty(fd)) { errno = ENOTTY; return -1; }
    struct plusos_posix_args a = { PLUSOS_POSIX_STTY_GET, fd, 0, 0, 0, 0, 0, {-1,-1,-1}, tp, sizeof *tp, 0 };
    return (int)posix_call(&a);
}

int tcsetattr(int fd, int optional_actions, const struct termios *tp)
{
    (void)optional_actions;
    if (!isatty(fd)) { errno = ENOTTY; return -1; }
    struct plusos_posix_args a = { PLUSOS_POSIX_STTY_SET, fd, 0, 0, 0, 0, 0, {-1,-1,-1}, (void *)tp, sizeof *tp, 0 };
    return (int)posix_call(&a);
}

int spawnv(const char *cmd, char *const argv[])
{
    int argc = 0;
    if (argv) while (argv[argc]) argc++;
    struct plusos_spawn_args sa = { cmd, argc, (char **)argv };
    return (int)plusos_syscall(SYS_spawn, &sa);
}

int system(const char *command)
{
    if (!command) return 1;
    char *av[4] = { (char *)"shell", (char *)"-c", (char *)command, 0 };
    int pid = spawnv("shell", av);
    if (pid < 0) return -1;
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}

/* ---- Environment helpers ------------------------------------------------- */
static char env_ret_buf[128];
char *getenv(const char *name)
{
    if (!name) return 0;
    struct plusos_env_args e = { PLUSOS_ENV_GET, name, env_ret_buf, sizeof env_ret_buf };
    if (plusos_syscall(SYS_getenv, &e) < 0) return 0;
    return env_ret_buf;
}

int setenv(const char *name, const char *value, int overwrite)
{
    if (!name || !*name) { errno = EINVAL; return -1; }
    if (!overwrite && getenv(name)) return 0;
    struct plusos_env_args e = { PLUSOS_ENV_SET, name, (char *)value, 0 };
    return (int)plusos_syscall(SYS_setenv, &e);
}

int unsetenv(const char *name)
{
    struct plusos_env_args e = { PLUSOS_ENV_UNSET, name, 0, 0 };
    return (int)plusos_syscall(SYS_setenv, &e);
}

/* ---- String / Number / Temp helpers -------------------------------------- */
char *strdup(const char *s)
{
    if (!s) return 0;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

char *strndup(const char *s, size_t n)
{
    if (!s) return 0;
    size_t l = strnlen(s, n);
    char *p = (char *)malloc(l + 1);
    if (p) { memcpy(p, s, l); p[l] = 0; }
    return p;
}

unsigned long long strtoull(const char *s, char **end, int base)
{
    while (isspace((unsigned char)*s)) s++;
    if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; base = 16; }
    else if (base == 0 && s[0] == '0') base = 8;
    else if (base == 0) base = 10;
    unsigned long long v = 0;
    for (;; s++) {
        int d;
        if (isdigit((unsigned char)*s)) d = *s - '0';
        else if (isalpha((unsigned char)*s)) d = tolower((unsigned char)*s) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned)base + (unsigned)d;
    }
    if (end) *end = (char *)s;
    return v;
}

long long strtoll(const char *s, char **end, int base)
{
    while (isspace((unsigned char)*s)) s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    unsigned long long v = strtoull(s, end, base);
    return neg ? -(long long)v : (long long)v;
}

void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *))
{
    size_t lo = 0, hi = n;
    const uint8_t *b = (const uint8_t *)base;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const void *p = b + mid * size;
        int r = cmp(key, p);
        if (r == 0) return (void *)p;
        if (r < 0) hi = mid;
        else lo = mid + 1;
    }
    return 0;
}

static uint32_t mk_seed = 0x42A1u;
static int fill_template(char *tmpl)
{
    size_t n = strlen(tmpl);
    if (n < 6 || strcmp(tmpl + n - 6, "XXXXXX") != 0) { errno = EINVAL; return -1; }
    static const char abc[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mk_seed = mk_seed * 1664525u + 1013904223u + (uint32_t)getpid();
    uint32_t x = mk_seed;
    for (size_t i = n - 6; i < n; i++) {
        tmpl[i] = abc[x % 62u];
        x /= 7u;
        if (x < 62u) x ^= (mk_seed += 0x9E37u);
    }
    return 0;
}

int mkstemp(char *tmpl)
{
    for (int tries = 0; tries < 64; tries++) {
        char orig[128];
        strlcpy(orig, tmpl, sizeof orig);
        if (fill_template(tmpl) < 0) return -1;
        int fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) return fd;
        strlcpy(tmpl, orig, strlen(orig) + 1);
    }
    errno = EEXIST;
    return -1;
}

char *mkdtemp(char *tmpl)
{
    for (int tries = 0; tries < 64; tries++) {
        char orig[128];
        strlcpy(orig, tmpl, sizeof orig);
        if (fill_template(tmpl) < 0) return 0;
        if (mkdir(tmpl, 0700) == 0) return tmpl;
        strlcpy(tmpl, orig, strlen(orig) + 1);
    }
    errno = EEXIST;
    return 0;
}

char *realpath(const char *path, char *resolved_path)
{
    struct stat st;
    if (stat(path, &st) < 0) return 0;
    char oldcwd[128];
    if (!getcwd(oldcwd, sizeof oldcwd)) strlcpy(oldcwd, "/", sizeof oldcwd);
    char out[128];
    if (S_ISDIR(st.st_mode)) {
        if (chdir(path) < 0) return 0;
        getcwd(out, sizeof out);
        chdir(oldcwd);
    } else {
        const char *sl = strrchr(path, '/');
        if (sl && sl > path) {
            char dir[128];
            size_t dl = (size_t)(sl - path);
            if (dl >= sizeof dir) dl = sizeof dir - 1;
            memcpy(dir, path, dl); dir[dl] = 0;
            if (chdir(dir) < 0) return 0;
            getcwd(out, sizeof out);
            chdir(oldcwd);
            if (strcmp(out, "/") != 0) strlcpy(out + strlen(out), sl, sizeof out - strlen(out));
            else strlcpy(out + 1, sl + 1, sizeof out - 1);
        } else if (sl == path) {
            strlcpy(out, path, sizeof out);
        } else {
            strlcpy(out, oldcwd, sizeof out);
            if (strcmp(out, "/") != 0) strlcpy(out + strlen(out), "/", sizeof out - strlen(out));
            strlcpy(out + strlen(out), path, sizeof out - strlen(out));
        }
    }
    if (!resolved_path) return strdup(out);
    strlcpy(resolved_path, out, 128);
    return resolved_path;
}

/* ---- Time & strftime ----------------------------------------------------- */
static struct tm g_tm;

time_t time(time_t *tloc)
{
    struct plusos_posix_args a = { PLUSOS_POSIX_TIME };
    time_t t = (time_t)plusos_syscall(SYS_posix, &a);
    if (tloc) *tloc = t;
    return t;
}

int stime(const time_t *t)
{
    if (!t) { errno = EINVAL; return -1; }
    struct plusos_posix_args a = { PLUSOS_POSIX_SETTIME };
    a.uval = (uint32_t)*t;
    return (int)posix_call(&a);
}

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

struct tm *gmtime(const time_t *timep)
{
    static const int mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    uint32_t t = timep ? (uint32_t)*timep : 0;
    uint32_t days = t / 86400UL;
    uint32_t rem  = t % 86400UL;
    g_tm.tm_hour = (int)(rem / 3600UL); rem %= 3600UL;
    g_tm.tm_min  = (int)(rem / 60UL);
    g_tm.tm_sec  = (int)(rem % 60UL);
    g_tm.tm_wday = (int)((days + 4UL) % 7UL); /* 1970-01-01 was Thursday (4) */
    int y = 1970;
    for (;;) {
        uint32_t dy = is_leap(y) ? 366UL : 365UL;
        if (days < dy) break;
        days -= dy;
        y++;
    }
    g_tm.tm_year = y - 1900;
    g_tm.tm_yday = (int)days;
    int m = 0;
    while (m < 11) {
        int dm = mdays[m] + (m == 1 && is_leap(y) ? 1 : 0);
        if ((int)days < dm) break;
        days -= (uint32_t)dm;
        m++;
    }
    g_tm.tm_mon = m;
    g_tm.tm_mday = (int)days + 1;
    g_tm.tm_isdst = 0;
    return &g_tm;
}

struct tm *localtime(const time_t *timep) { return gmtime(timep); }

time_t mktime(struct tm *tm)
{
    static const int mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    int y = tm->tm_year + 1900;
    uint32_t days = 0;
    for (int i = 1970; i < y; i++) days += is_leap(i) ? 366UL : 365UL;
    for (int m = 0; m < tm->tm_mon && m < 12; m++)
        days += (uint32_t)(mdays[m] + (m == 1 && is_leap(y) ? 1 : 0));
    days += (uint32_t)(tm->tm_mday - 1);
    return (time_t)(days * 86400UL + (uint32_t)tm->tm_hour * 3600UL +
                    (uint32_t)tm->tm_min * 60UL + (uint32_t)tm->tm_sec);
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
    static const char *const wd[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
    static const char *const mn[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
    size_t o = 0;
    for (; *fmt && o + 1 < max; fmt++) {
        if (*fmt != '%') { s[o++] = *fmt; continue; }
        fmt++;
        char tmp[24];
        const char *add = 0;
        switch (*fmt) {
        case 'Y': snprintf(tmp, sizeof tmp, "%04d", tm->tm_year + 1900); add = tmp; break;
        case 'y': snprintf(tmp, sizeof tmp, "%02d", (tm->tm_year + 1900) % 100); add = tmp; break;
        case 'm': snprintf(tmp, sizeof tmp, "%02d", tm->tm_mon + 1); add = tmp; break;
        case 'd': snprintf(tmp, sizeof tmp, "%02d", tm->tm_mday); add = tmp; break;
        case 'e': snprintf(tmp, sizeof tmp, "%2d", tm->tm_mday); add = tmp; break;
        case 'H': snprintf(tmp, sizeof tmp, "%02d", tm->tm_hour); add = tmp; break;
        case 'M': snprintf(tmp, sizeof tmp, "%02d", tm->tm_min); add = tmp; break;
        case 'S': snprintf(tmp, sizeof tmp, "%02d", tm->tm_sec); add = tmp; break;
        case 'a': add = (tm->tm_wday >= 0 && tm->tm_wday < 7) ? wd[tm->tm_wday] : ""; break;
        case 'b': add = (tm->tm_mon >= 0 && tm->tm_mon < 12) ? mn[tm->tm_mon] : ""; break;
        case 'F': snprintf(tmp, sizeof tmp, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday); add = tmp; break;
        case 'T': snprintf(tmp, sizeof tmp, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec); add = tmp; break;
        case 's': snprintf(tmp, sizeof tmp, "%ld", (long)mktime((struct tm *)tm)); add = tmp; break;
        case 'j': snprintf(tmp, sizeof tmp, "%03d", tm->tm_yday + 1); add = tmp; break;
        case 'u': snprintf(tmp, sizeof tmp, "%d", tm->tm_wday ? tm->tm_wday : 7); add = tmp; break;
        case 'w': snprintf(tmp, sizeof tmp, "%d", tm->tm_wday); add = tmp; break;
        case '%': add = "%"; break;
        default: tmp[0] = '%'; tmp[1] = *fmt; tmp[2] = 0; add = tmp; break;
        }
        while (add && *add && o + 1 < max) s[o++] = *add++;
    }
    if (max > 0) s[o] = 0;
    return o;
}

/* ---- POSIX getopt() ------------------------------------------------------ */
int getopt(int argc, char *const argv[], const char *optstring)
{
    static int sp = 1;
    if (optind == 0) { optind = 1; sp = 1; }
    if (sp == 1) {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) { optind++; return -1; }
    }
    int c = (unsigned char)argv[optind][sp];
    const char *cp = strchr(optstring, c);
    if (c == ':' || !cp) {
        optopt = c;
        if (argv[optind][++sp] == '\0') { optind++; sp = 1; }
        return '?';
    }
    if (cp[1] == ':') {
        if (argv[optind][sp + 1] != '\0') {
            optarg = &argv[optind++][sp + 1];
        } else if (++optind >= argc) {
            sp = 1;
            optopt = c;
            return (optstring[0] == ':') ? ':' : '?';
        } else {
            optarg = argv[optind++];
        }
        sp = 1;
    } else {
        if (argv[optind][++sp] == '\0') { sp = 1; optind++; }
        optarg = 0;
    }
    return c;
}

static sighandler_t g_sighandlers[24];
sighandler_t signal(int signum, sighandler_t handler)
{
    if (signum <= 0 || signum >= 24) return SIG_ERR;
    sighandler_t old = g_sighandlers[signum];
    g_sighandlers[signum] = handler;
    return old;
}
