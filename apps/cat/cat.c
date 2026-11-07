/* cat - print a text file from the boot volume. */
#include <plusos/plusos.h>
#include <plusrom/FileMgr.h>

static void p(const char *name, INTEGER vref)
{
    unsigned char pn[256];
    int n = 0;
    while (name[n] && n < 254) { pn[n + 1] = name[n]; n++; }
    pn[0] = n;
    INTEGER ref;
    if (FSOpen(pn, vref, &ref) != 0) { printf("cat: %s: not found\n", name); return; }
    char buf[256];
    for (;;) {
        LONGINT got = sizeof buf;
        OSErr e = FSRead(ref, &got, buf);
        if (got > 0) plusos->con_write(buf, (size_t)got);
        if (e != 0) break;                  /* 0 = ok; -39 (eofErr) ends it */
    }
    FSClose(ref);
}

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: cat <file>\n"); return 1; }
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    for (int i = 1; i < argc; i++) p(argv[i], vref);
    return 0;
}
