/* basic.c - PlusOS Retro BASIC Interpreter with Full-Screen Graphics
 *
 * Interactive BASIC environment supporting:
 *   Statements: PRINT, LET, INPUT, IF..THEN, GOTO, GOSUB, RETURN,
 *               FOR V = A TO B .. NEXT V, REM, CLS, BEEP, SLEEP, END
 *   Graphics:   PLOT x,y  |  LINE x1,y1,x2,y2  |  BOX x1,y1,x2,y2
 *               CIRCLE x,y,r  |  HEXAGON x,y,r,deg,shade  |  NGON x,y,r,s,deg,shade
 *               PRESENT  |  WAITKEY
 *   Commands:   LIST, RUN, NEW, DEMO1, DEMO2, DEMO3, SAVE <file>, LOAD <file>, BYE
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 120
#define LINE_LEN  80

typedef struct {
    int  num;
    char text[LINE_LEN];
} BasicLine;

static BasicLine prog[MAX_LINES];
static int       nlines = 0;
static long      vars[26];
static int       gfx_active = 0;

typedef struct {
    int  var;
    long limit;
    long step;
    int  line_idx;
} ForFrame;

static ForFrame for_stack[8];
static int      for_sp = 0;
static int      gosub_stack[16];
static int      gosub_sp = 0;

static void ensure_gfx(void)
{
    if (!gfx_active) {
        gfx_target(gfx_backbuffer());
        gfx_fill(0, 0, 512, 342, 0);
        gfx_active = 1;
    }
}

static void insert_line(int num, const char *txt)
{
    while (*txt == ' ') txt++;
    for (int i = 0; i < nlines; i++) {
        if (prog[i].num == num) {
            if (!*txt) {
                memmove(&prog[i], &prog[i + 1], (size_t)(nlines - i - 1) * sizeof(BasicLine));
                nlines--;
            } else {
                strlcpy(prog[i].text, txt, LINE_LEN);
            }
            return;
        }
        if (prog[i].num > num) {
            if (!*txt || nlines >= MAX_LINES) return;
            memmove(&prog[i + 1], &prog[i], (size_t)(nlines - i) * sizeof(BasicLine));
            prog[i].num = num;
            strlcpy(prog[i].text, txt, LINE_LEN);
            nlines++;
            return;
        }
    }
    if (*txt && nlines < MAX_LINES) {
        prog[nlines].num = num;
        strlcpy(prog[nlines].text, txt, LINE_LEN);
        nlines++;
    }
}

static long eval_expr(const char **ps);

static long eval_factor(const char **ps)
{
    const char *s = *ps;
    while (*s == ' ') s++;
    long val = 0;
    if (*s == '-') {
        s++;
        *ps = s;
        return -eval_factor(ps);
    }
    if (*s == '(') {
        s++;
        *ps = s;
        val = eval_expr(ps);
        s = *ps;
        while (*s == ' ') s++;
        if (*s == ')') s++;
        *ps = s;
        return val;
    }
    if (!strncasecmp(s, "RND(", 4)) {
        s += 4;
        *ps = s;
        long m = eval_expr(ps);
        s = *ps;
        if (*s == ')') s++;
        *ps = s;
        return m > 0 ? (rand() % m) : 0;
    }
    if (isalpha((unsigned char)*s)) {
        int v = toupper((unsigned char)*s) - 'A';
        while (isalpha((unsigned char)*s)) s++;
        *ps = s;
        return vars[v];
    }
    val = strtol(s, (char **)&s, 10);
    *ps = s;
    return val;
}

static long eval_term(const char **ps)
{
    long v = eval_factor(ps);
    for (;;) {
        const char *s = *ps;
        while (*s == ' ') s++;
        if (*s == '*') { s++; *ps = s; v *= eval_factor(ps); }
        else if (*s == '/') { s++; *ps = s; long d = eval_factor(ps); v = d ? v / d : 0; }
        else if (*s == '%') { s++; *ps = s; long d = eval_factor(ps); v = d ? v % d : 0; }
        else { *ps = s; break; }
    }
    return v;
}

static long eval_expr(const char **ps)
{
    long v = eval_term(ps);
    for (;;) {
        const char *s = *ps;
        while (*s == ' ') s++;
        if (*s == '+') { s++; *ps = s; v += eval_term(ps); }
        else if (*s == '-') { s++; *ps = s; v -= eval_term(ps); }
        else { *ps = s; break; }
    }
    return v;
}

static int parse_args(const char *s, long *out, int max_args)
{
    int n = 0;
    while (*s && n < max_args) {
        out[n++] = eval_expr(&s);
        while (*s == ' ') s++;
        if (*s == ',') s++;
        else break;
    }
    return n;
}

static int find_line_idx(int num)
{
    for (int i = 0; i < nlines; i++)
        if (prog[i].num == num) return i;
    return -1;
}

static void run_program(void)
{
    for_sp = 0;
    gosub_sp = 0;
    gfx_active = 0;
    int pc = 0;

    while (pc >= 0 && pc < nlines) {
        int kp_ = plusos->key_poll();
        if (kp_) {
            int k = (int)kp_;
            if (k == 27 || k == 3) { printf("\nBreak at line %d\n", prog[pc].num); break; }
        }
        const char *s = prog[pc].text;
        while (*s == ' ') s++;
        if (!*s || !strncasecmp(s, "REM", 3)) { pc++; continue; }
        if (!strncasecmp(s, "END", 3) || !strncasecmp(s, "STOP", 4)) break;
        if (!strncasecmp(s, "CLS", 3)) {
            if (gfx_active) gfx_fill(0, 0, 512, 342, 0);
            else con_clear();
            pc++; continue;
        }
        if (!strncasecmp(s, "PRESENT", 7)) {
            if (gfx_active) gfx_present();
            pc++; continue;
        }
        if (!strncasecmp(s, "WAITKEY", 7)) {
            if (gfx_active) gfx_present();
            plusos->key_get();
            pc++; continue;
        }
        if (!strncasecmp(s, "BEEP", 4)) {
            struct plusos_sound_args sa = { PLUSOS_SND_BEEP, 0, 10, 0, 0, 0 };
            plusos_syscall(SYS_sound, &sa);
            pc++; continue;
        }
        if (!strncasecmp(s, "SLEEP", 5)) {
            s += 5;
            long t = eval_expr(&s);
            plusos->sleep_ticks(t > 0 ? (int)t : 1);
            pc++; continue;
        }
        if (!strncasecmp(s, "PLOT", 4)) {
            long a[2];
            if (parse_args(s + 4, a, 2) >= 2) { ensure_gfx(); gfx_pixel((int)a[0], (int)a[1], 1); }
            pc++; continue;
        }
        if (!strncasecmp(s, "LINE", 4)) {
            long a[4];
            if (parse_args(s + 4, a, 4) >= 4) { ensure_gfx(); gfx_line((int)a[0], (int)a[1], (int)a[2], (int)a[3], 1); }
            pc++; continue;
        }
        if (!strncasecmp(s, "BOX", 3)) {
            long a[5];
            int na = parse_args(s + 3, a, 5);
            if (na >= 4) {
                ensure_gfx();
                if (na == 5 && a[4] > 0) gfx_dither_rect((int)a[0], (int)a[1], (int)a[2], (int)a[3], (int)a[4]);
                gfx_frame((int)a[0], (int)a[1], (int)a[2], (int)a[3]);
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "CIRCLE", 6)) {
            long a[4];
            int na = parse_args(s + 6, a, 4);
            if (na >= 3) {
                ensure_gfx();
                if (na == 4 && a[3]) gfx_fill_circle((int)a[0], (int)a[1], (int)a[2], 1);
                else gfx_circle((int)a[0], (int)a[1], (int)a[2], 1);
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "HEXAGON", 7)) {
            long a[5] = { 256, 171, 50, 0, 6 };
            int na = parse_args(s + 7, a, 5);
            if (na >= 3) {
                ensure_gfx();
                gfx_hexagon((int)a[0], (int)a[1], (int)a[2], (int)a[3], (int)a[4], 1);
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "NGON", 4)) {
            long a[6] = { 256, 171, 50, 6, 0, 8 };
            int na = parse_args(s + 4, a, 6);
            if (na >= 4) {
                ensure_gfx();
                if (a[5] > 0) gfx_ngon_fill((int)a[0], (int)a[1], (int)a[2], (int)a[3], (int)a[4], (int)a[5]);
                gfx_ngon_frame((int)a[0], (int)a[1], (int)a[2], (int)a[3], (int)a[4], 1);
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "PRINT", 5)) {
            s += 5;
            int newline = 1;
            while (*s) {
                while (*s == ' ') s++;
                if (!*s) break;
                if (*s == '"') {
                    s++;
                    while (*s && *s != '"') putchar(*s++);
                    if (*s == '"') s++;
                } else {
                    printf("%ld", eval_expr(&s));
                }
                while (*s == ' ') s++;
                if (*s == ',') { putchar('\t'); s++; newline = 1; }
                else if (*s == ';') { s++; newline = 0; }
                else break;
            }
            if (newline) putchar('\n');
            pc++; continue;
        }
        if (!strncasecmp(s, "INPUT", 5)) {
            s += 5;
            while (*s == ' ') s++;
            if (isalpha((unsigned char)*s)) {
                int v = toupper((unsigned char)*s) - 'A';
                char ibuf[32];
                printf("? ");
                plusos->key_line(ibuf, sizeof ibuf);
                vars[v] = atol(ibuf);
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "GOTO", 4)) {
            s += 4;
            int target = (int)eval_expr(&s);
            pc = find_line_idx(target);
            continue;
        }
        if (!strncasecmp(s, "GOSUB", 5)) {
            s += 5;
            int target = (int)eval_expr(&s);
            if (gosub_sp < 16) gosub_stack[gosub_sp++] = pc + 1;
            pc = find_line_idx(target);
            continue;
        }
        if (!strncasecmp(s, "RETURN", 6)) {
            if (gosub_sp > 0) pc = gosub_stack[--gosub_sp];
            else break;
            continue;
        }
        if (!strncasecmp(s, "FOR", 3)) {
            s += 3;
            while (*s == ' ') s++;
            int v = toupper((unsigned char)*s++) - 'A';
            while (*s == ' ' || *s == '=') s++;
            long start_v = eval_expr(&s);
            while (*s == ' ') s++;
            if (!strncasecmp(s, "TO", 2)) s += 2;
            long limit_v = eval_expr(&s);
            long step_v = 1;
            while (*s == ' ') s++;
            if (!strncasecmp(s, "STEP", 4)) { s += 4; step_v = eval_expr(&s); }
            vars[v] = start_v;
            if (for_sp < 8) {
                for_stack[for_sp].var = v;
                for_stack[for_sp].limit = limit_v;
                for_stack[for_sp].step = step_v;
                for_stack[for_sp].line_idx = pc + 1;
                for_sp++;
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "NEXT", 4)) {
            if (for_sp > 0) {
                ForFrame *f = &for_stack[for_sp - 1];
                vars[f->var] += f->step;
                if ((f->step >= 0 && vars[f->var] <= f->limit) ||
                    (f->step < 0 && vars[f->var] >= f->limit)) {
                    pc = f->line_idx;
                    continue;
                }
                for_sp--;
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "IF", 2)) {
            s += 2;
            long lhs = eval_expr(&s);
            while (*s == ' ') s++;
            int op = 0; /* 1:=, 2:<>, 3:<, 4:>, 5:<=, 6:>= */
            if (s[0] == '<' && s[1] == '>') { op = 2; s += 2; }
            else if (s[0] == '<' && s[1] == '=') { op = 5; s += 2; }
            else if (s[0] == '>' && s[1] == '=') { op = 6; s += 2; }
            else if (s[0] == '=') { op = 1; s++; }
            else if (s[0] == '<') { op = 3; s++; }
            else if (s[0] == '>') { op = 4; s++; }
            long rhs = eval_expr(&s);
            int cond = (op == 1 && lhs == rhs) || (op == 2 && lhs != rhs) ||
                       (op == 3 && lhs < rhs)  || (op == 4 && lhs > rhs)  ||
                       (op == 5 && lhs <= rhs) || (op == 6 && lhs >= rhs);
            if (cond) {
                while (*s == ' ') s++;
                if (!strncasecmp(s, "THEN", 4)) s += 4;
                while (*s == ' ') s++;
                if (isdigit((unsigned char)*s)) { pc = find_line_idx(atoi(s)); continue; }
                if (!strncasecmp(s, "GOTO", 4)) { s += 4; pc = find_line_idx((int)eval_expr(&s)); continue; }
            }
            pc++; continue;
        }
        if (!strncasecmp(s, "LET", 3)) s += 3;
        while (*s == ' ') s++;
        if (isalpha((unsigned char)*s)) {
            int v = toupper((unsigned char)*s++) - 'A';
            while (*s == ' ' || *s == '=') s++;
            vars[v] = eval_expr(&s);
        }
        pc++;
    }
    if (gfx_active) {
        gfx_present();
        con_clear();
        gfx_active = 0;
    }
}

static void load_demo1(void)
{
    nlines = 0;
    insert_line(10, "REM DEMO1: FULL-SCREEN HEXAGONS & POLYGONS SHOWCASE");
    insert_line(20, "CLS");
    insert_line(30, "FOR I = 1 TO 6");
    insert_line(40, "X = I * 72");
    insert_line(50, "HEXAGON X, 110, 32, I * 15, I * 2");
    insert_line(60, "NGON X, 230, 32, I + 2, I * 20, 14 - I * 2");
    insert_line(70, "NEXT I");
    insert_line(80, "BOX 10, 10, 502, 332, 0");
    insert_line(90, "WAITKEY");
    printf("Loaded DEMO1 (Hexagons & Polygons Graphics Demo). Type RUN!\n");
}

static void load_demo2(void)
{
    nlines = 0;
    insert_line(10, "REM DEMO2: ANIMATED ROTATING HEXAGON & CONCENTRIC RINGS");
    insert_line(20, "FOR A = 0 TO 180 STEP 6");
    insert_line(30, "CLS");
    insert_line(40, "HEXAGON 256, 171, 90, A, 4");
    insert_line(50, "NGON 256, 171, 55, 4, 360 - A, 10");
    insert_line(60, "CIRCLE 256, 171, 120, 0");
    insert_line(70, "PRESENT");
    insert_line(80, "NEXT A");
    insert_line(90, "WAITKEY");
    printf("Loaded DEMO2 (Animated Full-Screen Hexagon & Square). Type RUN!\n");
}

static void load_demo3(void)
{
    nlines = 0;
    insert_line(10, "REM DEMO3: SQUARES, CUBES & FACTORIAL TABLE");
    insert_line(20, "F = 1");
    insert_line(30, "FOR N = 1 TO 10");
    insert_line(40, "F = F * N");
    insert_line(50, "S = N * N");
    insert_line(60, "C = S * N");
    insert_line(70, "PRINT \"N=\"; N; \"  SQR=\"; S; \"  CUBE=\"; C; \"  FACT=\"; F");
    insert_line(80, "NEXT N");
    printf("Loaded DEMO3 (Math & Factorials Table). Type RUN!\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    memset(vars, 0, sizeof vars);
    load_demo1();

    printf("\nPlusOS Retro BASIC v2.0 (with PlusGFX Polygons & Hexagons)\n");
    printf("Commands: LIST, RUN, NEW, DEMO1, DEMO2, DEMO3, BYE\n");
    printf("Graphics: PLOT, LINE, BOX, CIRCLE, HEXAGON, NGON, PRESENT, WAITKEY\n\n");

    char line[128];
    for (;;) {
        printf("READY.\n> ");
        if (plusos->key_line(line, sizeof line) <= 0) continue;
        char *p = line;
        while (*p == ' ') p++;
        if (!*p) continue;

        if (isdigit((unsigned char)*p)) {
            int num = atoi(p);
            while (isdigit((unsigned char)*p)) p++;
            insert_line(num, p);
            continue;
        }
        if (!strcasecmp(p, "BYE") || !strcasecmp(p, "QUIT") || !strcasecmp(p, "EXIT"))
            break;
        if (!strcasecmp(p, "NEW")) { nlines = 0; printf("Program cleared.\n"); continue; }
        if (!strcasecmp(p, "LIST")) {
            for (int i = 0; i < nlines; i++)
                printf("%4d %s\n", prog[i].num, prog[i].text);
            continue;
        }
        if (!strcasecmp(p, "RUN"))   { run_program(); continue; }
        if (!strcasecmp(p, "DEMO1")) { load_demo1(); continue; }
        if (!strcasecmp(p, "DEMO2")) { load_demo2(); continue; }
        if (!strcasecmp(p, "DEMO3")) { load_demo3(); continue; }

        /* Direct immediate-mode execution of a single statement */
        BasicLine saved = prog[0];
        int saved_n = nlines;
        prog[0].num = 0;
        strlcpy(prog[0].text, p, LINE_LEN);
        nlines = 1;
        run_program();
        prog[0] = saved;
        nlines = saved_n;
    }
    return 0;
}
