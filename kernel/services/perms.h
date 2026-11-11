/* perms.h - PlusOS file permissions (Phase 3).
 *
 * HFS has no Unix mode / owner / group fields, so we keep them in a sidecar
 * file /etc/perms.db. One line per path:
 *
 *     path:mode:uid:gid
 *     /etc/shadow:0600:0:0
 *     /home/bob:0755:1000:1000
 *     /home/bob/Documents:0755:1000:1000
 *
 * ADVISORY ENFORCEMENT. Without an MMU and with userland programs that call
 * the Mac ROM File Manager directly (not through a kernel syscall), the
 * kernel cannot *physically* block a read or write. What this file gives:
 *
 *   - a single source of truth for mode/owner/group of named paths
 *   - a consult-the-table helper that cat, passwd, ls -l, etc. voluntarily
 *     call before acting
 *   - chmod / chown commands that update the table
 *
 * A path with no entry in the table is treated as mode 0644, owned by root,
 * group root - the same assumption classic Mac OS makes about every file. */
#ifndef PLUSOS_PERMS_H
#define PLUSOS_PERMS_H

#include <stdint.h>
#include "kconfig.h"

#define PERMS_PATH_MAX   48

/* Unix-shaped mode bits. We use octal digits that match POSIX so chmod 755
 * reads the way a reader expects. */
#define PERMS_IRUSR 0400
#define PERMS_IWUSR 0200
#define PERMS_IXUSR 0100
#define PERMS_IRGRP 0040
#define PERMS_IWGRP 0020
#define PERMS_IXGRP 0010
#define PERMS_IROTH 0004
#define PERMS_IWOTH 0002
#define PERMS_IXOTH 0001

/* Access modes passed to perms_check(). */
#define PERMS_R 4
#define PERMS_W 2
#define PERMS_X 1

typedef struct {
    char     path[PERMS_PATH_MAX];
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
} perms_rec;

#if CONFIG_PERMS

/* Called at boot from the module table. Reads /etc/perms.db if present. */
void perms_init(void);

/* Lookup. 0 on hit (fills out), -1 if path is not in the table. */
int  perms_get(const char *path, perms_rec *out);

/* Replace or insert an entry and write the table back to disk.
 * 0 on success, <0 on permission failure or disk error. */
int  perms_set(const char *path, uint16_t mode, uint16_t uid, uint16_t gid);

/* Convenience: change only one field. uid 0 may change any file; non-root
 * may only change files they own, and only chmod (not chown). */
int  perms_chmod(const char *path, uint16_t mode, int caller_uid);
int  perms_chown(const char *path, uint16_t uid, uint16_t gid, int caller_uid);

/* Advisory check. want is PERMS_R | PERMS_W | PERMS_X (bitwise).
 * Returns 1 if (caller_uid, caller_gid) has that access, 0 otherwise.
 * uid 0 always wins. Paths not in the table default to 0644 root:root. */
int  perms_check(const char *path, int caller_uid, int caller_gid, int want);

/* Iteration (for ls -l et al.). */
int  perms_count(void);
int  perms_at(int index, perms_rec *out);

#else  /* CONFIG_PERMS == 0 -- compile the calls away */

static inline void perms_init(void) {}
static inline int  perms_get(const char *p, perms_rec *o) { (void)p; (void)o; return -1; }
static inline int  perms_set(const char *p, uint16_t m, uint16_t u, uint16_t g) { (void)p; (void)m; (void)u; (void)g; return -1; }
static inline int  perms_chmod(const char *p, uint16_t m, int c) { (void)p; (void)m; (void)c; return -1; }
static inline int  perms_chown(const char *p, uint16_t u, uint16_t g, int c) { (void)p; (void)u; (void)g; (void)c; return -1; }
static inline int  perms_check(const char *p, int cu, int cg, int w) { (void)p; (void)cu; (void)cg; (void)w; return 1; }
static inline int  perms_count(void) { return 0; }
static inline int  perms_at(int i, perms_rec *o) { (void)i; (void)o; return -1; }

#endif
#endif
