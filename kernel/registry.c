/* registry.c - the PlusOS registry: a system-wide store of key = value
 * settings that survives restarts.
 *
 * Programs read and write it through syscalls (SYS_regget / SYS_regset /
 * SYS_regenum). It is kept in memory and saved to a plain text file, "PlusOS
 * Registry", on the boot volume - one "key=value" line per entry - so it can
 * also be read or fixed from a Mac. Keys are short dotted names such as
 * "desktop.wallpaper" or "icon.trash"; setting an empty value deletes a key.
 */
#include "kernel.h"
#include <plusos/syscall.h>

#define REG_MAX  96
#define REG_KEY  32
#define REG_VAL  64

typedef struct { char k[REG_KEY]; char v[REG_VAL]; } Entry;

static Entry reg[REG_MAX];
static int   nreg;
static int   loaded;
static const unsigned char reg_file[] = "\x0F" "PlusOS Registry";
static char  iobuf[REG_MAX * (REG_KEY + REG_VAL + 2)];

static void copy_str(char *d, const char *s, int size)
{
    int i = 0;
    for (; s[i] && i < size - 1; i++) d[i] = (s[i] == '\n' || s[i] == '\r') ? ' ' : s[i];
    d[i] = 0;
}

static int find(const char *k)
{
    for (int i = 0; i < nreg; i++) if (!strcmp(reg[i].k, k)) return i;
    return -1;
}

static void load(void)
{
    if (loaded) return;
    loaded = 1;
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    INTEGER ref;
    if (FSOpen(reg_file, g_boot_vref, &ref) != 0) return;
    LONGINT n = sizeof iobuf - 1;
    FSRead(ref, &n, iobuf);                      /* eofErr on a short file is fine */
    FSClose(ref);
    if (n < 0) n = 0;
    iobuf[n] = 0;
    char *p = iobuf;
    while (*p && nreg < REG_MAX) {
        char *line = p;
        while (*p && *p != '\n' && *p != '\r') p++;
        if (*p) *p++ = 0;
        char *eq = line;
        while (*eq && *eq != '=') eq++;
        if (*eq != '=' || eq == line) continue;
        *eq = 0;
        copy_str(reg[nreg].k, line, REG_KEY);
        copy_str(reg[nreg].v, eq + 1, REG_VAL);
        nreg++;
    }
}

static int save(void)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return -1;
    int len = 0;
    for (int i = 0; i < nreg; i++) {
        const char *k = reg[i].k, *v = reg[i].v;
        while (*k && len < (int)sizeof iobuf - 2) iobuf[len++] = *k++;
        iobuf[len++] = '=';
        while (*v && len < (int)sizeof iobuf - 2) iobuf[len++] = *v++;
        iobuf[len++] = '\n';
    }
    INTEGER ref;
    if (FSOpen(reg_file, g_boot_vref, &ref) != 0) {
        Create(reg_file, g_boot_vref, 0x504C4F53 /* 'PLOS' */, 0x54455854 /* 'TEXT' */);
        if (FSOpen(reg_file, g_boot_vref, &ref) != 0) return -1;
    }
    LONGINT n = len;
    int rc = (SetFPos(ref, 1, 0) == 0 && FSWrite(ref, &n, iobuf) == 0) ? 0 : -1;
    SetEOF(ref, len);
    FSClose(ref);
    FlushVol(0, g_boot_vref);
    return rc;
}

int reg_get(const char *key, char *out, int size)
{
    load();
    int i = find(key);
    if (i < 0) return -1;
    copy_str(out, reg[i].v, size);
    return (int)strlen(out);
}

int reg_set(const char *key, const char *value)
{
    load();
    if (!key || !key[0]) return -1;
    int i = find(key);
    if (!value || !value[0]) {                   /* empty value = delete */
        if (i < 0) return 0;
        reg[i] = reg[--nreg];
        return save();
    }
    if (i >= 0 && !strcmp(reg[i].v, value)) return 0;   /* unchanged: no disk write */
    if (i < 0) {
        if (nreg >= REG_MAX) return -1;
        i = nreg++;
        copy_str(reg[i].k, key, REG_KEY);
    }
    copy_str(reg[i].v, value, REG_VAL);
    return save();
}

int reg_enum(int index, char *key, int ksize, char *value, int vsize)
{
    load();
    if (index < 0 || index >= nreg) return -1;
    copy_str(key, reg[index].k, ksize);
    copy_str(value, reg[index].v, vsize);
    return 0;
}
