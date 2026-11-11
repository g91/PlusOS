/* users.h - PlusOS user accounts.
 *
 * Phase 1: user records on disk in a Unix-shaped layout.
 *
 *   /etc/passwd  - text, one line per user:  name:uid:gid:home:shell:flags
 *   /etc/shadow  - text, one line per user:  name:salt:hash
 *   /home/<name> - home directory with Desktop, Documents, Settings
 *   /root        - root's home
 *
 * No permission enforcement yet - that's Phase 3. This file lets apps ask
 * "does this user exist?", "does this password match?", "is anybody set up
 * yet?", and get back a user record. The login and firstrun apps are the
 * only callers today.
 *
 * Without an MMU the hash is a safety-against-shoulder-surfing measure, not
 * a cryptographic defence. The kernel runs in one address space with every
 * program, so anything can poke anything if it tries hard enough. Treat
 * accounts as a courtesy layer, like classic Mac user accounts. */
#ifndef PLUSOS_USERS_H
#define PLUSOS_USERS_H

#include <stdint.h>
#include "kconfig.h"

#define USER_NAME_MAX   16
#define USER_HOME_MAX   40
#define USER_SHELL_MAX  16
#define USER_SALT_LEN   8
#define USER_HASH_LEN   16              /* 64 hex-ish chars worth of digest */

/* flags bitfield (passwd, column 6) */
#define USER_F_SUDOER   0x01            /* may use sudo (Phase 4) */

typedef struct {
    uint16_t uid;
    uint16_t gid;
    uint8_t  flags;
    char     name [USER_NAME_MAX];
    char     home [USER_HOME_MAX];
    char     shell[USER_SHELL_MAX];
} user_rec;

#if CONFIG_USERS

/* Called from the module table at boot. Loads /etc/passwd into the in-memory
 * table. If the file does not exist users_firstrun_needed() returns 1. */
void users_init(void);

/* 1 if /etc/passwd is absent or has no accounts - run the firstrun wizard. */
int  users_firstrun_needed(void);

/* Look up a user by name or uid. Returns 0 on success, <0 if not found. */
int  users_find(const char *name, user_rec *out);
int  users_find_uid(int uid,       user_rec *out);

/* Returns 1 if (name, password) matches an account in /etc/shadow, 0 otherwise.
 * password may be "" for accounts that chose no password at creation. */
int  users_authenticate(const char *name, const char *password);

/* Append a new user. Creates /etc/passwd, /etc/shadow, /home, and the user's
 * home with Desktop, Documents, Settings. Returns 0 on success.
 *
 * is_root selects uid 0 and home /root instead of /home/<name>.
 * Pass "" as password for a no-password account. */
int  users_add(const char *name, const char *password, int is_sudoer, int is_root);

/* The currently logged-in user. Starts at 0 ("not logged in") until login
 * sets it. Login and sudo are the only callers of set_current. */
int  users_get_current(void);
void users_set_current(int uid);

/* How many accounts are loaded. */
int  users_count(void);

/* The nth account (0..users_count()-1). Returns 0 or <0. */
int  users_at(int index, user_rec *out);

/* Replace the password of an existing account. "" = no-password account.
 * Returns 0 on success, <0 if the user doesn't exist or the disk write fails. */
int  users_set_password(const char *name, const char *new_pw);

/* 1 if `name` appears in /etc/sudoers. /etc/sudoers is a plain text file,
 * one username per line (# starts a comment). The firstrun wizard and
 * adduser write the file; sudo reads it. */
int  users_is_sudoer(const char *name);

/* Rewrite /etc/sudoers from whatever is in /etc/passwd: every account with
 * USER_F_SUDOER gets a line. Called after users_add / users_delete so the
 * two files stay in sync. */
int  users_write_sudoers(void);

/* Remove an account from /etc/passwd and /etc/shadow. Home directory is
 * left behind (classic Unix: userdel keeps the data, userdel -r removes it;
 * we only do the former here). Returns 0 or -1. */
int  users_delete(const char *name);

/* Translate a Unix-style path ("/etc/passwd", "~/Documents", "./foo") into
 * the HFS colon form the ROM File Manager expects (":etc:passwd", etc.).
 * ~ expands to the current user's home. A bare "~" means the home itself.
 * Relative paths are passed through unchanged (no $PWD tracking yet).
 * Returns 0 on success, -1 if cap was too small.
 *
 * This is best-effort: it is a convenience for commands like cat / ls / cd
 * that take paths. Programs talking to the Mac ROM directly still use HFS
 * colon syntax natively. */
int  hfs_path_translate(const char *in, char *out, int cap);

#else  /* CONFIG_USERS == 0 - compile the calls away */

static inline void users_init(void) {}
static inline int  users_firstrun_needed(void) { return 0; }
static inline int  users_find(const char *n, user_rec *o) { (void)n; (void)o; return -1; }
static inline int  users_find_uid(int u, user_rec *o)     { (void)u; (void)o; return -1; }
static inline int  users_authenticate(const char *n, const char *p) { (void)n; (void)p; return 0; }
static inline int  users_add(const char *n, const char *p, int s, int r) { (void)n; (void)p; (void)s; (void)r; return -1; }
static inline int  users_get_current(void) { return 0; }
static inline void users_set_current(int uid) { (void)uid; }
static inline int  users_count(void) { return 0; }
static inline int  users_at(int i, user_rec *o) { (void)i; (void)o; return -1; }

#endif
#endif
