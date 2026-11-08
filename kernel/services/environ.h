/* environ.h - PlusOS Environment Variables Subsystem */
#ifndef PLUSOS_ENVIRON_H
#define PLUSOS_ENVIRON_H

#include <stdint.h>
#include <plusos/syscall.h>

#define MAX_ENV_VARS 32
#define ENV_KEY_LEN  24
#define ENV_VAL_LEN  64

void env_init(void);
int  env_get(const char *key, char *val, int max_len);
int  env_set(const char *key, const char *val);
int  env_unset(const char *key);
int  env_list(char *buf, int max_len);

#endif
