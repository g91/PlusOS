/* clip.c - PlusOS System-Wide Clipboard / Scrap Manager Utility
 *
 * Usage:
 *   clip                Show current clipboard contents & metadata
 *   clip copy <text...> Copy text into the persistent system clipboard
 *   clip paste          Print raw clipboard text
 *   clip clear          Zero the clipboard ("Clipboard File")
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct plusos_scrap_args s;
    memset(&s, 0, sizeof s);

    if (argc >= 2 && (!strcasecmp(argv[1], "clear") || !strcasecmp(argv[1], "-c"))) {
        s.op = PLUSOS_SCRAP_CLEAR;
        plusos_syscall(SYS_scrap, &s);
        printf("Clipboard cleared.\n");
        return 0;
    }

    if (argc >= 2 && (!strcasecmp(argv[1], "copy") || !strcasecmp(argv[1], "set"))) {
        char buf[1024];
        int pos = 0;
        buf[0] = 0;
        for (int i = 2; i < argc && pos < (int)sizeof(buf) - 1; i++) {
            int n = snprintf(buf + pos, sizeof(buf) - (size_t)pos, "%s%s", (i > 2) ? " " : "", argv[i]);
            if (n > 0) pos += n;
        }
        s.op   = PLUSOS_SCRAP_SET;
        s.type = 0x54455854UL; /* 'TEXT' */
        s.buf  = buf;
        s.size = (int)strlen(buf);
        int n  = (int)plusos_syscall(SYS_scrap, &s);
        printf("Copied %d bytes to Clipboard File.\n", n);
        return 0;
    }

    char out[1024];
    s.op   = PLUSOS_SCRAP_GET;
    s.buf  = out;
    s.size = sizeof out;
    int len = (int)plusos_syscall(SYS_scrap, &s);
    if (argc >= 2 && !strcasecmp(argv[1], "paste")) {
        if (len > 0) printf("%s\n", out);
        return 0;
    }

    printf("PlusOS Clipboard (ScrapCount #%d, %d bytes, type '%c%c%c%c'):\n",
           s.count, len,
           (char)(s.type >> 24), (char)(s.type >> 16), (char)(s.type >> 8), (char)s.type);
    if (len > 0) printf("  \"%s\"\n", out);
    else         printf("  (empty - use 'clip copy <text>' to store text)\n");
    return 0;
}
