/* environ.c - PlusOS Environment Variables Subsystem */
#include "kernel.h"
#include "environ.h"

typedef struct {
    char key[ENV_KEY_LEN];
    char val[ENV_VAL_LEN];
    int  active;
} EnvVar;

static EnvVar env_table[MAX_ENV_VARS];

void env_init(void)
{
    memset(env_table, 0, sizeof env_table);
    env_set("PATH", ":bin:");
    env_set("USER", "admin");
    env_set("SHELL", "shell");
    env_set("TERM", "pluscon");
    env_set("OS", "PlusOS");
    env_set("ARCH", "m68k");
    env_set("HOME", ":");
}

int env_get(const char *key, char *val, int max_len)
{
    if (!key || !val || max_len <= 0) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].active && strcmp(env_table[i].key, key) == 0) {
            int len = (int)strlen(env_table[i].val);
            strlcpy(val, env_table[i].val, (size_t)max_len);
            int_restore(sr);
            return len;
        }
    }
    int_restore(sr);
    return -1;
}

int env_set(const char *key, const char *val)
{
    if (!key || !*key) return -1;
    if (!val || !*val) return env_unset(key);

    uint16_t sr = int_disable();
    /* Overwrite existing */
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].active && strcmp(env_table[i].key, key) == 0) {
            strlcpy(env_table[i].val, val, sizeof(env_table[i].val));
            int_restore(sr);
            return 0;
        }
    }
    /* Add new */
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (!env_table[i].active) {
            env_table[i].active = 1;
            strlcpy(env_table[i].key, key, sizeof(env_table[i].key));
            strlcpy(env_table[i].val, val, sizeof(env_table[i].val));
            int_restore(sr);
            return 0;
        }
    }
    int_restore(sr);
    return -1;
}

int env_unset(const char *key)
{
    if (!key) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].active && strcmp(env_table[i].key, key) == 0) {
            env_table[i].active = 0;
            int_restore(sr);
            return 0;
        }
    }
    int_restore(sr);
    return -1;
}

int env_list(char *buf, int max_len)
{
    if (!buf || max_len <= 0) return 0;
    uint16_t sr = int_disable();
    int pos = 0;
    buf[0] = '\0';
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].active) {
            int needed = snprintf(buf + pos, (size_t)(max_len - pos), "%s=%s\n",
                                  env_table[i].key, env_table[i].val);
            if (needed < 0 || pos + needed >= max_len) break;
            pos += needed;
        }
    }
    int_restore(sr);
    return pos;
}
