/* unistd.h - POSIX standard operating system API */
#ifndef PLUSOS_UNISTD_H
#define PLUSOS_UNISTD_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

extern char *optarg;
extern int   optind;
extern int   opterr;
extern int   optopt;

int     getopt(int argc, char *const argv[], const char *optstring);

int     open(const char *path, int flags, ...);
int     close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t   lseek(int fd, off_t offset, int whence);
int     dup(int oldfd);
int     dup2(int oldfd, int newfd);
int     pipe(int fds[2]);
int     isatty(int fd);

int     chdir(const char *path);
char   *getcwd(char *buf, size_t size);
int     chroot(const char *path);
int     unlink(const char *path);
int     rmdir(const char *path);
int     link(const char *target, const char *linkpath);
int     symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
int     truncate(const char *path, off_t length);
int     ftruncate(int fd, off_t length);
int     access(const char *path, int mode);
int     chown(const char *path, uid_t uid, gid_t gid);
void    sync(void);

unsigned int sleep(unsigned int seconds);
int     usleep(unsigned long usec);
pid_t   getpid(void);
pid_t   getppid(void);
uid_t   getuid(void);
uid_t   geteuid(void);
gid_t   getgid(void);
gid_t   getegid(void);
int     gethostname(char *name, size_t len);
int     sethostname(const char *name, size_t len);
long    gethostid(void);
int     nice(int inc);
int     spawnv(const char *cmd, char *const argv[]);

#ifdef __cplusplus
}
#endif
#endif
