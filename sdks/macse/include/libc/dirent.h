/* dirent.h - POSIX directory traversal */
#ifndef PLUSOS_DIRENT_H
#define PLUSOS_DIRENT_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10

struct dirent {
    ino_t   d_ino;
    uint8_t d_type;
    char    d_name[64];
};

typedef struct __plusos_dir {
    char path[128];
    int  index;
    struct dirent cur;
} DIR;

DIR *opendir(const char *path);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);

#ifdef __cplusplus
}
#endif
#endif
