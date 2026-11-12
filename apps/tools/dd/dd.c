/* dd - convert and copy a file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int parse_sz(const char *s, unsigned long long *out)
{
    if (!s || !*s) return -1;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s) return -1;
    unsigned long long mul = 1;
    if (*end) {
        switch (tolower((unsigned char)*end)) {
        case 'b': mul = 512ULL; end++; break;
        case 'c': mul = 1ULL; end++; break;
        case 'w': mul = 2ULL; end++; break;
        case 'k': mul = 1024ULL; end++; break;
        case 'm': mul = 1024ULL * 1024ULL; end++; break;
        case 'g': mul = 1024ULL * 1024ULL * 1024ULL; end++; break;
        default: return -1;
        }
    }
    *out = v * mul;
    return 0;
}

int main(int argc, char **argv)
{
    const char *infile = NULL, *outfile = NULL;
    size_t ibs = 512, obs = 512;
    unsigned long long count = (unsigned long long)-1, skip = 0, seek = 0;
    int notrunc = 0, conv_uc = 0, conv_lc = 0, conv_sync = 0, noxfer = 0, status_none = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "if=", 3) == 0) infile = argv[i] + 3;
        else if (strncmp(argv[i], "of=", 3) == 0) outfile = argv[i] + 3;
        else if (strncmp(argv[i], "bs=", 3) == 0) { unsigned long long v = 0; if (parse_sz(argv[i] + 3, &v) || !v) return 1; ibs = obs = (size_t)v; }
        else if (strncmp(argv[i], "ibs=", 4) == 0) { unsigned long long v = 0; if (parse_sz(argv[i] + 4, &v) || !v) return 1; ibs = (size_t)v; }
        else if (strncmp(argv[i], "obs=", 4) == 0) { unsigned long long v = 0; if (parse_sz(argv[i] + 4, &v) || !v) return 1; obs = (size_t)v; }
        else if (strncmp(argv[i], "count=", 6) == 0) parse_sz(argv[i] + 6, &count);
        else if (strncmp(argv[i], "skip=", 5) == 0) parse_sz(argv[i] + 5, &skip);
        else if (strncmp(argv[i], "seek=", 5) == 0) parse_sz(argv[i] + 5, &seek);
        else if (strncmp(argv[i], "conv=", 5) == 0) {
            if (strstr(argv[i] + 5, "notrunc")) notrunc = 1;
            if (strstr(argv[i] + 5, "ucase")) conv_uc = 1;
            if (strstr(argv[i] + 5, "lcase")) conv_lc = 1;
            if (strstr(argv[i] + 5, "sync")) conv_sync = 1;
        } else if (strcmp(argv[i], "status=none") == 0) status_none = 1;
        else if (strcmp(argv[i], "status=noxfer") == 0) noxfer = 1;
    }

    int infd = infile ? open(infile, O_RDONLY) : 0;
    if (infd < 0) { fprintf(stderr, "dd: opening '%s': %s\n", infile, strerror(errno)); return 1; }
    int outfd = 1;
    if (outfile) {
        outfd = open(outfile, O_WRONLY | O_CREAT | (notrunc ? 0 : O_TRUNC), 0666);
        if (outfd < 0) { fprintf(stderr, "dd: opening '%s': %s\n", outfile, strerror(errno)); if (infile) close(infd); return 1; }
    }
    size_t bsz = ibs > obs ? ibs : obs;
    unsigned char *buf = malloc(bsz);
    if (!buf) { if (infile) close(infd); if (outfile) close(outfd); return 1; }
    if (skip > 0 && lseek(infd, (off_t)(skip * ibs), SEEK_SET) < 0) {
        for (unsigned long long s = 0; s < skip; s++) if (read(infd, buf, ibs) <= 0) break;
    }
    if (seek > 0) lseek(outfd, (off_t)(seek * obs), SEEK_SET);

    unsigned long long fi = 0, pi = 0, fo = 0, po = 0, tot = 0;
    for (unsigned long long blk = 0; blk < count; blk++) {
        ssize_t n = read(infd, buf, ibs);
        if (n <= 0) break;
        if ((size_t)n == ibs) fi++; else pi++;
        if (conv_sync && (size_t)n < ibs) { memset(buf + n, 0, ibs - (size_t)n); n = (ssize_t)ibs; }
        if (conv_uc) for (ssize_t k = 0; k < n; k++) buf[k] = (unsigned char)toupper(buf[k]);
        if (conv_lc) for (ssize_t k = 0; k < n; k++) buf[k] = (unsigned char)tolower(buf[k]);
        ssize_t off = 0;
        while (off < n) {
            size_t chunk = (size_t)(n - off) > obs ? obs : (size_t)(n - off);
            ssize_t w = write(outfd, buf + off, chunk);
            if (w <= 0) break;
            if ((size_t)w == obs) fo++; else po++;
            tot += (unsigned long long)w; off += w;
        }
    }
    free(buf);
    if (infile) close(infd);
    if (outfile) close(outfd);
    if (!status_none) {
        fprintf(stderr, "%llu+%llu records in\n%llu+%llu records out\n", fi, pi, fo, po);
        if (!noxfer) fprintf(stderr, "%llu bytes copied\n", tot);
    }
    return 0;
}
