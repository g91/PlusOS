/* expr - evaluate expressions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **g_av;
static int g_ac, g_idx;
typedef struct { int is_num; long long num; char *str; } Val;

static Val eval_or(void);
static int as_num(const Val *v, long long *out)
{
    if (v->is_num) { *out = v->num; return 1; }
    if (!v->str || !v->str[0]) return 0;
    char *end = NULL; long long n = strtoll(v->str, &end, 10);
    if (*end == '\0') { *out = n; return 1; }
    return 0;
}
static int is_true(const Val *v) { long long n; return as_num(v, &n) ? (n != 0) : (v->str && v->str[0]); }

static int match_here(const char *re, const char *txt, const char **endp);
static int match_star(const char *re, int ch, const char *txt, const char **endp)
{
    const char *t = txt;
    while (*t && (ch == '.' || *t == ch)) t++;
    while (t >= txt) { if (match_here(re, t, endp)) return 1; t--; }
    return 0;
}
static int match_here(const char *re, const char *txt, const char **endp)
{
    if (!re[0]) { *endp = txt; return 1; }
    if (re[1] == '*') return match_star(re + 2, (unsigned char)re[0], txt, endp);
    if (re[0] == '$' && !re[1]) { *endp = txt; return *txt == '\0'; }
    if (*txt && (re[0] == '.' || re[0] == *txt)) return match_here(re + 1, txt + 1, endp);
    return 0;
}

static Val eval_prim(void)
{
    Val r = { 1, 0, NULL };
    if (g_idx >= g_ac) return r;
    const char *tok = g_av[g_idx];
    if (strcmp(tok, "(") == 0) {
        g_idx++; r = eval_or();
        if (g_idx < g_ac && strcmp(g_av[g_idx], ")") == 0) g_idx++;
        return r;
    }
    if (strcmp(tok, "length") == 0 && g_idx + 1 < g_ac) {
        g_idx++; Val v = eval_prim(); r.is_num = 1;
        if (v.str) r.num = (long long)strlen(v.str);
        else { char b[24]; snprintf(b, sizeof b, "%lld", v.num); r.num = (long long)strlen(b); }
        free(v.str); return r;
    }
    if (strcmp(tok, "substr") == 0 && g_idx + 3 < g_ac) {
        g_idx++; Val sv = eval_prim(), pv = eval_prim(), lv = eval_prim();
        long long pos = 1, len = 0; as_num(&pv, &pos); as_num(&lv, &len);
        const char *s = sv.str ? sv.str : ""; long long sl = (long long)strlen(s);
        r.is_num = 0;
        if (pos < 1 || pos > sl || len <= 0) r.str = strdup("");
        else {
            long long take = (pos - 1 + len > sl) ? (sl - (pos - 1)) : len;
            r.str = malloc((size_t)take + 1); memcpy(r.str, s + pos - 1, (size_t)take); r.str[take] = '\0';
        }
        free(sv.str); free(pv.str); free(lv.str); return r;
    }
    g_idx++; r.is_num = 0; r.str = strdup(tok);
    long long n; if (as_num(&r, &n)) { r.is_num = 1; r.num = n; }
    return r;
}

static Val eval_colon(void)
{
    Val a = eval_prim();
    while (g_idx < g_ac && strcmp(g_av[g_idx], ":") == 0) {
        g_idx++; Val b = eval_prim();
        const char *text = a.str ? a.str : "", *pat = b.str ? b.str : "", *endm = NULL;
        Val res = { 1, 0, NULL };
        if (match_here(pat, text, &endm)) res.num = (long long)(endm - text);
        free(a.str); free(b.str); a = res;
    }
    return a;
}

static Val eval_mul(void)
{
    Val a = eval_colon();
    while (g_idx < g_ac) {
        const char *op = g_av[g_idx];
        if (strcmp(op, "*") && strcmp(op, "/") && strcmp(op, "%")) break;
        g_idx++; Val b = eval_colon();
        long long va = 0, vb = 0; as_num(&a, &va); as_num(&b, &vb);
        free(a.str); free(b.str); a.is_num = 1; a.str = NULL;
        if (op[0] == '*') a.num = va * vb;
        else if (vb == 0) { fprintf(stderr, "expr: division by zero\n"); a.num = 0; }
        else a.num = (op[0] == '/') ? (va / vb) : (va % vb);
    }
    return a;
}

static Val eval_add(void)
{
    Val a = eval_mul();
    while (g_idx < g_ac) {
        const char *op = g_av[g_idx]; if (strcmp(op, "+") && strcmp(op, "-")) break;
        g_idx++; Val b = eval_mul();
        long long va = 0, vb = 0; as_num(&a, &va); as_num(&b, &vb);
        free(a.str); free(b.str); a.is_num = 1; a.str = NULL;
        a.num = (op[0] == '+') ? (va + vb) : (va - vb);
    }
    return a;
}

static Val eval_cmp(void)
{
    Val a = eval_add();
    while (g_idx < g_ac) {
        const char *op = g_av[g_idx];
        int k = !strcmp(op, "=") || !strcmp(op, "==") ? 1 : !strcmp(op, "!=") ? 2 :
                !strcmp(op, "<") ? 3 : !strcmp(op, "<=") ? 4 : !strcmp(op, ">") ? 5 : !strcmp(op, ">=") ? 6 : 0;
        if (!k) break;
        g_idx++; Val b = eval_add();
        long long va, vb;
        int c = (as_num(&a, &va) && as_num(&b, &vb)) ? ((va < vb) ? -1 : (va > vb) ? 1 : 0)
                                                    : strcmp(a.str ? a.str : "", b.str ? b.str : "");
        free(a.str); free(b.str); a.is_num = 1; a.str = NULL;
        a.num = (k==1)?(c==0):(k==2)?(c!=0):(k==3)?(c<0):(k==4)?(c<=0):(k==5)?(c>0):(c>=0);
    }
    return a;
}

static Val eval_and(void)
{
    Val a = eval_cmp();
    while (g_idx < g_ac && strcmp(g_av[g_idx], "&") == 0) {
        g_idx++; Val b = eval_cmp();
        if (!is_true(&a) || !is_true(&b)) { free(a.str); free(b.str); a.is_num = 1; a.num = 0; a.str = NULL; }
        else free(b.str);
    }
    return a;
}

static Val eval_or(void)
{
    Val a = eval_and();
    while (g_idx < g_ac && strcmp(g_av[g_idx], "|") == 0) {
        g_idx++; Val b = eval_and();
        if (is_true(&a)) free(b.str); else { free(a.str); a = b; }
    }
    return a;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "expr: missing operand\n"); return 2; }
    g_av = argv; g_ac = argc; g_idx = 1;
    Val r = eval_or();
    int zero;
    if (r.is_num && !r.str) { printf("%lld\n", r.num); zero = (r.num == 0); }
    else { printf("%s\n", r.str ? r.str : ""); zero = (!r.str || !r.str[0] || (r.is_num && r.num == 0)); }
    free(r.str);
    return zero ? 1 : 0;
}
