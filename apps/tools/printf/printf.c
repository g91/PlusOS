/* printf - format and print data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int emit_escaped_b(const char *s)
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
            int v = 0, cnt = 0; if (*p == '0') p++;
            while (*p >= '0' && *p <= '7' && cnt < 3) { v = v * 8 + (*p++ - '0'); cnt++; }
            p--; putchar(v & 0xff); break;
        }
        default: putchar('\\'); putchar((unsigned char)*p); break;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int i = 1; if (i < argc && strcmp(argv[i], "--") == 0) i++;
    if (i >= argc) { fprintf(stderr, "printf: missing operand\n"); return 1; }
    const char *fmt = argv[i++]; int argi = i;
    do {
        int start_argi = argi;
        for (const char *p = fmt; *p; p++) {
            if (*p == '\\') {
                p++; if (!*p) { putchar('\\'); break; }
                switch (*p) {
                case '\\': putchar('\\'); break; case 'a': putchar('\a'); break;
                case 'b': putchar('\b'); break;  case 'e': putchar(0x1B); break;
                case 'f': putchar('\f'); break;  case 'n': putchar('\n'); break;
                case 'r': putchar('\r'); break;  case 't': putchar('\t'); break;
                case 'v': putchar('\v'); break;  case '"': putchar('"'); break;
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
                    int v = 0, cnt = 0; while (*p >= '0' && *p <= '7' && cnt < 3) { v = v * 8 + (*p++ - '0'); cnt++; }
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
                default: putchar((unsigned char)*p); break;
                }
                continue;
            }
            if (*p != '%') { putchar((unsigned char)*p); continue; }
            const char *spec_start = p++;
            if (*p == '%') { putchar('%'); continue; }
            while (*p == '-' || *p == '+' || *p == '0' || *p == ' ' || *p == '#') p++;
            while (isdigit((unsigned char)*p)) p++;
            if (*p == '.') { p++; while (isdigit((unsigned char)*p)) p++; }
            char conv = *p; if (!conv) break;
            const char *arg = (argi < argc) ? argv[argi++] : "";
            char onefmt[32];
            if (conv == 's') {
                int k = 0; for (const char *q = spec_start; q <= p && k < 30; q++) onefmt[k++] = *q;
                onefmt[k] = '\0'; printf(onefmt, arg);
            } else if (conv == 'b') { if (emit_escaped_b(arg)) return 0; }
            else if (conv == 'c') putchar((unsigned char)arg[0]);
            else if (conv == 'd' || conv == 'i') {
                long long v = (arg[0] == '\'' || arg[0] == '"') ? (unsigned char)arg[1] : strtoll(arg, NULL, 0);
                int k = 0; for (const char *q = spec_start; q < p && k < 28; q++) onefmt[k++] = *q;
                onefmt[k++] = 'l'; onefmt[k++] = 'l'; onefmt[k++] = conv; onefmt[k] = '\0';
                printf(onefmt, v);
            } else if (conv == 'u' || conv == 'o' || conv == 'x' || conv == 'X') {
                unsigned long long v = (arg[0] == '\'' || arg[0] == '"') ? (unsigned char)arg[1] : strtoull(arg, NULL, 0);
                int k = 0; for (const char *q = spec_start; q < p && k < 28; q++) onefmt[k++] = *q;
                onefmt[k++] = 'l'; onefmt[k++] = 'l'; onefmt[k++] = conv; onefmt[k] = '\0';
                printf(onefmt, v);
            } else { putchar('%'); putchar(conv); }
        }
        if (argi == start_argi) break;
    } while (argi < argc);
    return 0;
}
