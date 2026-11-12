/* tsort - perform topological sort. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static char *g_nodes[128];
static int g_indeg[128];
static unsigned char g_adj[128][128];
static unsigned char g_done[128];

int main(int argc, char **argv)
{
    FILE *f = (argc > 1 && strcmp(argv[1], "-") != 0) ? fopen(argv[1], "rb") : stdin;
    if (!f) { fprintf(stderr, "tsort: %s: %s\n", argv[1], strerror(errno)); return 1; }
    int n_nodes = 0, have_first = 0, c, len = 0;
    char tok1[64], tok2[64], word[64];
    for (;;) {
        c = fgetc(f);
        if (c == EOF || isspace((unsigned char)c)) {
            if (len > 0) {
                word[len] = '\0'; len = 0;
                if (!have_first) { strlcpy(tok1, word, sizeof tok1); have_first = 1; }
                else {
                    strlcpy(tok2, word, sizeof tok2); have_first = 0;
                    int u = -1, v = -1;
                    for (int k = 0; k < n_nodes; k++) if (strcmp(g_nodes[k], tok1) == 0) { u = k; break; }
                    if (u < 0 && n_nodes < 128) { u = n_nodes; g_nodes[n_nodes++] = strdup(tok1); }
                    for (int k = 0; k < n_nodes; k++) if (strcmp(g_nodes[k], tok2) == 0) { v = k; break; }
                    if (v < 0 && n_nodes < 128) { v = n_nodes; g_nodes[n_nodes++] = strdup(tok2); }
                    if (u >= 0 && v >= 0 && u != v && !g_adj[u][v]) { g_adj[u][v] = 1; g_indeg[v]++; }
                }
            }
            if (c == EOF) break;
        } else if (len < 63) word[len++] = (char)c;
    }
    if (f != stdin) fclose(f);
    int printed = 0;
    while (printed < n_nodes) {
        int pick = -1;
        for (int k = 0; k < n_nodes; k++) if (!g_done[k] && g_indeg[k] == 0) { pick = k; break; }
        if (pick < 0) {
            for (int k = 0; k < n_nodes; k++) if (!g_done[k]) { pick = k; break; }
            fprintf(stderr, "tsort: input contains a loop\n");
        }
        g_done[pick] = 1; printed++;
        puts(g_nodes[pick]);
        for (int k = 0; k < n_nodes; k++) if (g_adj[pick][k]) { g_adj[pick][k] = 0; g_indeg[k]--; }
    }
    for (int k = 0; k < n_nodes; k++) free(g_nodes[k]);
    return 0;
}
