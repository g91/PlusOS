/* ls - list files on the boot volume. With no argument it lists the "bin"
 * folder (where the commands live); "ls /" lists the volume root. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>

#define ROOT_DIR 2L                 /* fsRtDirID */

static void p2c(const unsigned char *p, char *c)
{
    int n = p[0];
    for (int i = 0; i < n; i++) c[i] = p[i + 1];
    c[n] = 0;
}

/* find the dirID of a folder named `name` in the root; 0 if not found */
static long find_dir(INTEGER vref, const char *name)
{
    for (int idx = 1; idx < 128; idx++) {
        CInfoPBRec pb;
        unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = ROOT_DIR;
        pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char c[64];
        p2c(nm, c);
        if ((pb.dirInfo.ioFlAttrib & 0x10) && !strcasecmp(c, name))
            return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

static void list(INTEGER vref, long dir, const char *label)
{
    printf("%s:\n", label);
    int count = 0;
    for (int idx = 1; idx < 256; idx++) {
        CInfoPBRec pb;
        unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm;
        pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir;
        pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        char c[64];
        p2c(nm, c);
        int isdir = (pb.hFileInfo.ioFlAttrib & 0x10) != 0;
        if (isdir) printf("  %-24s <dir>\n", c);
        else       printf("  %-24s %6lu\n", c, (unsigned long)pb.hFileInfo.ioFlLgLen);
        count++;
    }
    if (!count) printf("  (empty)\n");
}

int main(int argc, char **argv)
{
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    int want_root = argc >= 2 && (argv[1][0] == '/' || !strcasecmp(argv[1], "root"));
    if (want_root) { list(vref, ROOT_DIR, si.volume); return 0; }
    long bin = find_dir(vref, "bin");
    if (bin) list(vref, bin, "bin");
    else { printf("no bin folder; listing the volume root\n"); list(vref, ROOT_DIR, si.volume); }
    return 0;
}
