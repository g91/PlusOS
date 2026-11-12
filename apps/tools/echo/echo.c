/* echo - display a line of text. */
#include <stdio.h>
#include <ctype.h>

static int emit_escaped(const char *s)
{
    for (const char *p = s; *p; p++) {
        if (*p != '\\') { putchar((unsigned char)*p); continue; }
        p++;
        if (!*p) { putchar('\\'); break; }
        switch (*p) {
        case '\\': putchar('\\'); break; case 'a': putchar('\a'); break;
        case 'b': putchar('\b'); break;  case 'c': return 1;
        case 'e': putchar(0x1B); break;  case 'f': putchar('\f'); break;
        case 'n': putchar('\n'); break;  case 'r': putchar('\r'); break;
        case 't': putchar('\t'); break;  case 'v': putchar('\v'); break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
            int v = 0, cnt = 0;
            if (*p == '0') p++;
            while (*p >= '0' && *p <= '7' && cnt < 3) { v = v * 8 + (*p++ - '0'); cnt++; }
            p--; putchar(v & 0xff); break;
        }
        case 'x': {
            int v = 0, cnt = 0; p++;
            while (isxdigit((unsigned char)*p) && cnt < 2) {
                int d = isdigit((unsigned char)*p) ? (*p - '0') : (tolower((unsigned char)*p) - 'a' + 10);
                v = v * 16 + d; p++; cnt++;
            }
            p--; putchar(v & 0xff); break;
        }
        default: putchar('\\'); putchar((unsigned char)*p); break;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int no_nl = 0, interp = 0, i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1]) {
        int valid = 1;
        for (const char *p = argv[i] + 1; *p; p++) if (*p != 'n' && *p != 'e' && *p != 'E') { valid = 0; break; }
        if (!valid) break;
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'n') no_nl = 1; else if (*p == 'e') interp = 1; else if (*p == 'E') interp = 0;
        }
        i++;
    }
    for (int k = i; k < argc; k++) {
        if (k > i) putchar(' ');
        if (interp) { if (emit_escaped(argv[k])) return 0; }
        else fputs(argv[k], stdout);
    }
    if (!no_nl) putchar('\n');
    return 0;
}
