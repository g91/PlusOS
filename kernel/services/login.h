/* login.h - PlusOS kernel-side first-run wizard and login prompt. */
#ifndef PLUSOS_LOGIN_H
#define PLUSOS_LOGIN_H
#include "kconfig.h"

#if CONFIG_USERS
/* If /etc/passwd is missing, walk the user through creating the root account
 * (default password "Apple") and their own account. Writes /etc/passwd and
 * /etc/shadow and seeds /home/<name> with Desktop, Documents and Settings. */
void login_firstrun(void);

/* Prompt "login:" / "password:" at the kernel console. Loops on bad
 * credentials. Sets users_set_current(uid) on success and returns. */
void login_prompt(void);
#else
static inline void login_firstrun(void) {}
static inline void login_prompt(void)   {}
#endif

#endif
