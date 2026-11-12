// pp.cpp - source loading, tokenizer and C preprocessor.
#include "cc.h"
#include <ctime>

namespace p68 {

Options opt;
std::vector<std::string> ppDependencies;

// ------------------------------------------------------------ diagnostics
std::string format(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

void fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "p68-cc: error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static void vreport(const Token *tok, const char *kind, const char *fmt, va_list ap) {
    if (tok && tok->file)
        fprintf(stderr, "%s:%d: %s: ", tok->file->name.c_str(), tok->line, kind);
    else
        fprintf(stderr, "p68-cc: %s: ", kind);
    vfprintf(stderr, fmt, ap);
    if (tok && tok->kind != Token::END && !tok->text.empty()) fprintf(stderr, " (near '%s')", tok->text.c_str());
    fprintf(stderr, "\n");
}

void errorAt(const Token *tok, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vreport(tok, "error", fmt, ap);
    va_end(ap);
    exit(1);
}

void warnAt(const Token *tok, const char *fmt, ...) {
    if (!opt.warnings || (tok && tok->file && tok->file->system)) return;
    va_list ap;
    va_start(ap, fmt);
    vreport(tok, "warning", fmt, ap);
    va_end(ap);
}

bool readFile(const std::string &path, std::string &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.clear();
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}

// ------------------------------------------------------------ source files
static std::vector<SrcFile *> allFiles;

SrcFile *loadSource(const std::string &path) {
    std::string raw;
    if (!readFile(path, raw)) return nullptr;
    SrcFile *f = new SrcFile;
    f->name = path;
    f->id = (int)allFiles.size();
    // normalise newlines, strip UTF-8 BOM, make sure the text ends with a newline
    std::string &t = f->text;
    size_t i = 0;
    if (raw.size() >= 3 && (unsigned char)raw[0] == 0xEF && (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF) i = 3;
    t.reserve(raw.size() + 2);
    for (; i < raw.size(); i++) {
        if (raw[i] == '\r') {
            if (i + 1 < raw.size() && raw[i + 1] == '\n') continue;
            t += '\n';
        } else
            t += raw[i];
    }
    if (t.empty() || t.back() != '\n') t += '\n';
    allFiles.push_back(f);
    ppDependencies.push_back(path);
    return f;
}

// ------------------------------------------------------------ tokenizer
static bool isIdent1(int c) { return isalpha(c) || c == '_' || c == '$' || c >= 0x80; }
static bool isIdent2(int c) { return isIdent1(c) || isdigit(c); }

static const char *punct3[] = {"<<=", ">>=", "...", "->*", nullptr};
static const char *punct2[] = {"==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=", "++", "--", "%=", "&=",
                               "|=", "^=", "&&", "||", "<<", ">>", "##", "::", ".*", nullptr};

static int readEscape(const char *&p) {
    // p points after the backslash
    int c = (unsigned char)*p++;
    switch (c) {
    case 'a': return 7;
    case 'b': return 8;
    case 't': return 9;
    case 'n': return 10;
    case 'v': return 11;
    case 'f': return 12;
    case 'r': return 13;
    case 'e': return 27;
    case 'x': {
        int v = 0;
        while (isxdigit((unsigned char)*p)) {
            int d = *p++;
            v = v * 16 + (isdigit(d) ? d - '0' : (tolower(d) - 'a' + 10));
        }
        return v & 0xFF;
    }
    default:
        if (c >= '0' && c <= '7') {
            int v = c - '0';
            for (int i = 0; i < 2 && *p >= '0' && *p <= '7'; i++) v = v * 8 + (*p++ - '0');
            return v & 0xFF;
        }
        return c;
    }
}

Token *tokenizeFile(SrcFile *f) {
    // Phase 2: splice backslash-newline, keeping a line map
    const std::string &src = f->text;
    std::string buf;
    std::vector<int> lineOf; // line number of each char of buf
    buf.reserve(src.size());
    lineOf.reserve(src.size());
    int line = 1;
    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == '\\') {
            size_t j = i + 1;
            while (j < src.size() && (src[j] == ' ' || src[j] == '\t')) j++;
            if (j < src.size() && src[j] == '\n') { i = j; line++; continue; }
        }
        buf += src[i];
        lineOf.push_back(line);
        if (src[i] == '\n') line++;
    }
    buf += '\0';
    lineOf.push_back(line);

    Token head;
    Token *cur = &head;
    const char *base = buf.c_str();
    const char *p = base;
    bool bol = true, space = false;
    bool asmMode = opt.asmPreprocess;
    auto add = [&](Token::Kind k, const char *s, const char *e) -> Token * {
        Token *t = new Token;
        t->kind = k;
        t->text.assign(s, e - s);
        t->file = f;
        t->line = lineOf[s - base];
        t->bol = bol;
        t->space = space;
        bol = false;
        space = false;
        cur->next = t;
        cur = t;
        return t;
    };
    while (*p) {
        if (*p == '\n') { p++; bol = true; space = false; continue; }
        if (*p == ' ' || *p == '\t' || *p == '\f' || *p == '\v') { p++; space = true; continue; }
        if (p[0] == '/' && p[1] == '/' && !asmMode) {
            while (*p && *p != '\n') p++;
            space = true;
            continue;
        }
        if (p[0] == '/' && p[1] == '/' && asmMode) {
            while (*p && *p != '\n') p++;
            space = true;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            const char *q = strstr(p + 2, "*/");
            if (!q) {
                Token t; t.file = f; t.line = lineOf[p - base];
                errorAt(&t, "unterminated comment");
            }
            p = q + 2;
            space = true;
            continue;
        }
        if (asmMode && *p == '|' ) { // GAS m68k line comment
            while (*p && *p != '\n') p++;
            space = true;
            continue;
        }
        // numbers (pp-number)
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
            const char *s = p++;
            for (;;) {
                if (p[0] && p[1] && strchr("eEpP", p[0]) && strchr("+-", p[1]) && !asmMode) p += 2;
                else if (isalnum((unsigned char)*p) || *p == '.' || *p == '_') p++;
                else break;
            }
            add(Token::PPNUM, s, p);
            continue;
        }
        // string / char literals (with optional prefix)
        {
            const char *q = p;
            if (!asmMode) {
                if (q[0] == 'u' && q[1] == '8') q += 2;
                else if (*q == 'L' || *q == 'u' || *q == 'U') q++;
            }
            if (*q == '"' || (*q == '\'' )) {
                char quote = *q;
                const char *e = q + 1;
                bool ok = true;
                for (;;) {
                    if (*e == '\0' || *e == '\n') { ok = false; break; }
                    if (*e == '\\' && e[1] && e[1] != '\n') { e += 2; continue; }
                    if (*e == quote) break;
                    e++;
                }
                if (ok) {
                    Token *t = add(Token::STR, p, e + 1);
                    (void)t;
                    p = e + 1;
                    continue;
                }
                if (!asmMode && q == p) {
                    Token t; t.file = f; t.line = lineOf[p - base];
                    errorAt(&t, "unterminated %s literal", quote == '"' ? "string" : "character");
                }
                if (q == p) { add(Token::OTHER, p, p + 1); p++; continue; }
            }
        }
        if (isIdent1((unsigned char)*p)) {
            const char *s = p++;
            while (isIdent2((unsigned char)*p)) p++;
            add(Token::IDENT, s, p);
            continue;
        }
        // punctuators
        {
            bool done = false;
            for (int i = 0; punct3[i] && !done; i++)
                if (!strncmp(p, punct3[i], 3)) { add(Token::PUNCT, p, p + 3); p += 3; done = true; }
            for (int i = 0; punct2[i] && !done; i++)
                if (!strncmp(p, punct2[i], 2) && (opt.cxx || (strcmp(punct2[i], "::") && strcmp(punct2[i], ".*")))) { add(Token::PUNCT, p, p + 2); p += 2; done = true; }
            if (done) continue;
        }
        if (ispunct((unsigned char)*p)) { add(Token::PUNCT, p, p + 1); p++; continue; }
        add(Token::OTHER, p, p + 1);
        p++;
    }
    Token *eof = new Token;
    eof->kind = Token::END;
    eof->file = f;
    eof->line = line;
    eof->bol = true;
    cur->next = eof;
    return head.next;
}

static Token *tokenizeString(const std::string &text, const char *fname = "<built-in>") {
    SrcFile *f = new SrcFile;
    f->name = fname;
    f->text = text + "\n";
    f->system = true;
    bool save = opt.asmPreprocess;
    opt.asmPreprocess = false;
    Token *t = tokenizeFile(f);
    opt.asmPreprocess = save;
    return t;
}

// ------------------------------------------------------------ hidesets
struct Hideset {
    const std::string *name;
    Hideset *next;
};
static std::set<std::string> internPool;
static const std::string *intern(const std::string &s) { return &*internPool.insert(s).first; }
static Hideset *hsNew(const std::string *n, Hideset *next = nullptr) { return new Hideset{n, next}; }
static bool hsContains(Hideset *h, const std::string *n) {
    for (; h; h = h->next)
        if (h->name == n) return true;
    return false;
}
static Hideset *hsUnion(Hideset *a, Hideset *b) {
    if (!a) return b;
    Hideset head{nullptr, nullptr};
    Hideset *cur = &head;
    for (; a; a = a->next) cur = cur->next = hsNew(a->name);
    cur->next = b;
    return head.next;
}
static Hideset *hsIntersect(Hideset *a, Hideset *b) {
    Hideset head{nullptr, nullptr};
    Hideset *cur = &head;
    for (; a; a = a->next)
        if (hsContains(b, a->name)) cur = cur->next = hsNew(a->name);
    return head.next;
}

// ------------------------------------------------------------ macros
struct Macro {
    std::string name;
    bool funcLike = false;
    bool variadic = false;
    std::vector<std::string> params;
    std::string vaName; // "__VA_ARGS__" or GNU named variadic
    Token *body = nullptr;
    int builtin = 0; // 1 __FILE__ 2 __LINE__ 3 __COUNTER__ 4 __BASE_FILE__
};
static std::unordered_map<std::string, Macro *> macros;
static std::set<std::string> onceFiles;
static std::unordered_map<std::string, std::string> guardOf; // file -> include guard macro
static SrcFile *baseFile = nullptr;
static int counterValue = 0;

static Token *copyTok(const Token *t) {
    Token *n = new Token(*t);
    n->next = nullptr;
    return n;
}
static Token *newEof(const Token *like) {
    Token *t = new Token;
    t->kind = Token::END;
    if (like) { t->file = like->file; t->line = like->line; }
    return t;
}
static Token *append(Token *a, Token *b) {
    if (!a || a->kind == Token::END) return b;
    Token head;
    Token *cur = &head;
    for (; a && a->kind != Token::END; a = a->next) cur = cur->next = copyTok(a);
    cur->next = b;
    return head.next;
}
static Token *skipLine(Token *tok) {
    while (!tok->bol && tok->kind != Token::END) tok = tok->next;
    return tok;
}
// copy tokens up to end of line; returns list terminated by END; *rest = next line
static Token *copyLine(Token *tok, Token **rest) {
    Token head;
    Token *cur = &head;
    for (; !tok->bol && tok->kind != Token::END; tok = tok->next) cur = cur->next = copyTok(tok);
    cur->next = newEof(tok);
    *rest = tok;
    return head.next;
}

static Token *makeNumTok(int64_t v, const Token *like) {
    Token *t = new Token;
    t->kind = Token::PPNUM;
    t->text = std::to_string((long long)v);
    t->file = like->file;
    t->line = like->line;
    t->space = like->space;
    t->bol = like->bol;
    return t;
}
static std::string quoteString(const std::string &s) {
    std::string r = "\"";
    for (char c : s) {
        if (c == '\\' || c == '"') r += '\\';
        r += c;
    }
    return r + "\"";
}
static Token *makeStrTok(const std::string &s, const Token *like) {
    Token *t = new Token;
    t->kind = Token::STR;
    t->text = quoteString(s);
    t->file = like->file;
    t->line = like->line;
    t->space = like->space;
    t->bol = like->bol;
    return t;
}

static std::string joinTokens(Token *tok, Token *end) {
    std::string s;
    for (Token *t = tok; t != end && t->kind != Token::END; t = t->next) {
        if (t != tok && t->space) s += ' ';
        s += t->text;
    }
    return s;
}

void ppDefine(const std::string &name, const std::string &body) {
    Macro *m = new Macro;
    m->name = name;
    m->body = tokenizeString(body);
    macros[name] = m;
}
void ppUndef(const std::string &name) { macros.erase(name); }

static void defineBuiltin(const char *name, int id) {
    Macro *m = new Macro;
    m->name = name;
    m->builtin = id;
    macros[name] = m;
}

void ppInit() {
    macros.clear();
    defineBuiltin("__FILE__", 1);
    defineBuiltin("__LINE__", 2);
    defineBuiltin("__COUNTER__", 3);
    defineBuiltin("__BASE_FILE__", 4);
    time_t now = time(nullptr);
    struct tm *tm = localtime(&now);
    static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    ppDefine("__DATE__", format("\"%s %2d %d\"", mon[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900));
    ppDefine("__TIME__", format("\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec));
    static const char *defs[] = {
        "__P68__=1", "__p68__=1", "__m68k__=1", "__mc68000__=1", "__m68k=1", "__mc68000=1", "mc68000=1",
        "__BIG_ENDIAN__=1", "__ORDER_BIG_ENDIAN__=4321", "__ORDER_LITTLE_ENDIAN__=1234",
        "__BYTE_ORDER__=__ORDER_BIG_ENDIAN__", "__CHAR_BIT__=8", "__SIZEOF_INT__=4", "__SIZEOF_LONG__=4",
        "__SIZEOF_LONG_LONG__=8", "__SIZEOF_SHORT__=2", "__SIZEOF_POINTER__=4", "__SIZEOF_FLOAT__=4",
        "__SIZEOF_DOUBLE__=8", "__SIZEOF_SIZE_T__=4", "__SIZEOF_PTRDIFF_T__=4",
        "__SIZE_TYPE__=unsigned long", "__PTRDIFF_TYPE__=long", "__WCHAR_TYPE__=long", "__INTPTR_TYPE__=long",
        "__UINTPTR_TYPE__=unsigned long", "__INT8_TYPE__=signed char", "__INT16_TYPE__=short",
        "__INT32_TYPE__=long", "__INT64_TYPE__=long long", "__UINT8_TYPE__=unsigned char",
        "__UINT16_TYPE__=unsigned short", "__UINT32_TYPE__=unsigned long", "__UINT64_TYPE__=unsigned long long",
        "__INT_LEAST8_TYPE__=signed char", "__INT_LEAST16_TYPE__=short", "__INT_LEAST32_TYPE__=long", "__INT_LEAST64_TYPE__=long long",
        "__UINT_LEAST8_TYPE__=unsigned char", "__UINT_LEAST16_TYPE__=unsigned short", "__UINT_LEAST32_TYPE__=unsigned long",
        "__UINT_LEAST64_TYPE__=unsigned long long", "__INT_FAST8_TYPE__=signed char", "__INT_FAST16_TYPE__=int", "__INT_FAST32_TYPE__=long",
        "__INT_FAST64_TYPE__=long long", "__UINT_FAST8_TYPE__=unsigned char", "__UINT_FAST16_TYPE__=unsigned int",
        "__UINT_FAST32_TYPE__=unsigned long", "__UINT_FAST64_TYPE__=unsigned long long", "__INTMAX_TYPE__=long long",
        "__UINTMAX_TYPE__=unsigned long long", "__INT8_MAX__=127", "__INT16_MAX__=32767", "__INT32_MAX__=2147483647L",
        "__INT64_MAX__=9223372036854775807LL", "__UINT8_MAX__=255", "__UINT16_MAX__=65535", "__UINT32_MAX__=4294967295UL",
        "__UINT64_MAX__=18446744073709551615ULL", "__INTPTR_MAX__=2147483647L", "__UINTPTR_MAX__=4294967295UL",
        "__SIZE_MAX__=4294967295UL", "__PTRDIFF_MAX__=2147483647L", "__INTMAX_MAX__=9223372036854775807LL",
        "__INT8_C(c)=c", "__INT16_C(c)=c", "__INT32_C(c)=c##L", "__INT64_C(c)=c##LL", "__UINT8_C(c)=c", "__UINT16_C(c)=c",
        "__UINT32_C(c)=c##UL", "__UINT64_C(c)=c##ULL", "__INTMAX_C(c)=c##LL", "__UINTMAX_C(c)=c##ULL",
        "__INT_MAX__=2147483647", "__LONG_MAX__=2147483647L", "__SCHAR_MAX__=127", "__SHRT_MAX__=32767",
        "__LONG_LONG_MAX__=9223372036854775807LL", "__STDC__=1", "__STDC_HOSTED__=0",
        "__GNUC__=4", "__GNUC_MINOR__=2", "__GNUC_PATCHLEVEL__=1", "__VERSION__=\"P68 C compiler 0.1\"",
        "__ELF__=1", "__USER_LABEL_PREFIX__=", "__REGISTER_PREFIX__=%",
        "__alignof__=_Alignof", "__const=const", "__const__=const", "__signed=signed", "__signed__=signed",
        "__volatile__=volatile", "__volatile=volatile", "__inline=inline", "__inline__=inline",
        "__restrict=", "__restrict__=", "__extension__=", "__typeof=__typeof__", "__asm=__asm__",
        "__builtin_expect(x,y)=(x)", "__builtin_offsetof(t,m)=((unsigned long)&((t*)0)->m)",
        "__builtin_va_copy(d,s)=((d)=(s))", "__builtin_va_end(ap)=((void)0)", "__builtin_constant_p(x)=0",
        "__pascal=pascal",
        nullptr};
    for (int i = 0; defs[i]; i++) {
        std::string d = defs[i];
        size_t eq = d.find('=');
        std::string name = d.substr(0, eq), body = d.substr(eq + 1);
        size_t par = name.find('(');
        if (par != std::string::npos) {
            // function-like builtin definition: go through the #define parser
            Token *t = tokenizeString("#define " + name + " " + body);
            preprocess(t);
        } else
            ppDefine(name, body);
    }
    if (opt.cxx) {
        ppDefine("__cplusplus", "201703L");
    } else {
        ppDefine("__STDC_VERSION__", "201112L");
    }
    switch (opt.cpu) {
    case 68020: ppDefine("__mc68020__", "1"); ppDefine("mc68020", "1"); break;
    case 68030: ppDefine("__mc68020__", "1"); ppDefine("__mc68030__", "1"); ppDefine("mc68030", "1"); break;
    case 68040: ppDefine("__mc68020__", "1"); ppDefine("__mc68040__", "1"); ppDefine("mc68040", "1"); break;
    case 68060: ppDefine("__mc68020__", "1"); ppDefine("__mc68060__", "1"); break;
    default: break;
    }
    if (opt.optLevel > 0) ppDefine("__OPTIMIZE__", "1");
    if (opt.optSize) ppDefine("__OPTIMIZE_SIZE__", "1");
    if (opt.asmPreprocess) ppDefine("__ASSEMBLER__", "1");
    for (auto &d : opt.defines) {
        size_t eq = d.find('=');
        std::string name = eq == std::string::npos ? d : d.substr(0, eq);
        std::string body = eq == std::string::npos ? "1" : d.substr(eq + 1);
        if (name.find('(') != std::string::npos) {
            Token *t = tokenizeString("#define " + name + " " + body, "<command-line>");
            preprocess(t);
        } else
            ppDefine(name, body);
    }
    for (auto &u : opt.undefs) ppUndef(u);
}

// ------------------------------------------------------------ #if expressions
struct PPVal {
    int64_t v;
    bool uns;
};
static Token *ppTok;
static PPVal ppCond();
static int64_t parsePPNumber(Token *t, bool *isUnsigned, bool *isFloat, double *fv, int *rank);

static int charConstValue(const Token *t) {
    const char *p = t->text.c_str();
    while (*p != '\'') p++;
    p++;
    int64_t v = 0;
    int n = 0;
    while (*p && *p != '\'') {
        int c;
        if (*p == '\\') { p++; c = readEscape(p); }
        else c = (unsigned char)*p++;
        v = (v << 8) | (c & 0xFF);
        n++;
    }
    if (n == 1) return (int)(signed char)v; // plain char is signed
    return (int)v;
}

static PPVal ppPrimary() {
    Token *t = ppTok;
    if (t->is("(")) {
        ppTok = t->next;
        PPVal v = ppCond();
        if (!ppTok->is(")")) errorAt(ppTok, "expected ')' in #if");
        ppTok = ppTok->next;
        return v;
    }
    if (t->kind == Token::PPNUM) {
        bool u = false, fl = false;
        double fv;
        int rank;
        int64_t v = parsePPNumber(t, &u, &fl, &fv, &rank);
        if (fl) errorAt(t, "floating constant in preprocessor expression");
        ppTok = t->next;
        return {v, u};
    }
    if (t->kind == Token::STR && t->text[0] == '\'') {
        ppTok = t->next;
        return {charConstValue(t), false};
    }
    if (t->kind == Token::IDENT) { // remaining identifiers are 0
        ppTok = t->next;
        if (t->text == "true") return {1, false};
        return {0, false};
    }
    errorAt(t, "bad token in preprocessor expression");
}
static PPVal ppUnary() {
    Token *t = ppTok;
    if (t->is("+")) { ppTok = t->next; return ppUnary(); }
    if (t->is("-")) { ppTok = t->next; PPVal v = ppUnary(); v.v = -v.v; return v; }
    if (t->is("!")) { ppTok = t->next; PPVal v = ppUnary(); return {!v.v, false}; }
    if (t->is("~")) { ppTok = t->next; PPVal v = ppUnary(); v.v = ~v.v; return v; }
    return ppPrimary();
}
static int ppPrec(const Token *t) {
    if (t->kind != Token::PUNCT) return -1;
    const std::string &s = t->text;
    if (s == "*" || s == "/" || s == "%") return 10;
    if (s == "+" || s == "-") return 9;
    if (s == "<<" || s == ">>") return 8;
    if (s == "<" || s == "<=" || s == ">" || s == ">=") return 7;
    if (s == "==" || s == "!=") return 6;
    if (s == "&") return 5;
    if (s == "^") return 4;
    if (s == "|") return 3;
    if (s == "&&") return 2;
    if (s == "||") return 1;
    return -1;
}
static PPVal ppBinary(int minPrec) {
    PPVal l = ppUnary();
    for (;;) {
        int prec = ppPrec(ppTok);
        if (prec < minPrec || prec < 0) return l;
        Token *opTok = ppTok;
        std::string op = ppTok->text;
        ppTok = ppTok->next;
        PPVal r = ppBinary(prec + 1);
        bool u = l.uns || r.uns;
        uint64_t a = (uint64_t)l.v, b = (uint64_t)r.v;
        int64_t sa = l.v, sb = r.v;
        PPVal res{0, u};
        if (op == "*") res.v = (int64_t)(a * b);
        else if (op == "/" || op == "%") {
            if (b == 0) errorAt(opTok, "division by zero in #if");
            if (u) res.v = (int64_t)(op == "/" ? a / b : a % b);
            else res.v = op == "/" ? sa / sb : sa % sb;
        } else if (op == "+") res.v = (int64_t)(a + b);
        else if (op == "-") res.v = (int64_t)(a - b);
        else if (op == "<<") res = {(int64_t)(a << (b & 63)), l.uns};
        else if (op == ">>") res = {l.uns ? (int64_t)(a >> (b & 63)) : (sa >> (b & 63)), l.uns};
        else if (op == "<") res = {u ? a < b : sa < sb, false};
        else if (op == "<=") res = {u ? a <= b : sa <= sb, false};
        else if (op == ">") res = {u ? a > b : sa > sb, false};
        else if (op == ">=") res = {u ? a >= b : sa >= sb, false};
        else if (op == "==") res = {a == b, false};
        else if (op == "!=") res = {a != b, false};
        else if (op == "&") res.v = (int64_t)(a & b);
        else if (op == "^") res.v = (int64_t)(a ^ b);
        else if (op == "|") res.v = (int64_t)(a | b);
        else if (op == "&&") res = {a && b, false};
        else if (op == "||") res = {a || b, false};
        l = res;
    }
}
static PPVal ppCond() {
    PPVal c = ppBinary(1);
    if (!ppTok->is("?")) return c;
    ppTok = ppTok->next;
    PPVal a = ppCond();
    if (!ppTok->is(":")) errorAt(ppTok, "expected ':' in #if");
    ppTok = ppTok->next;
    PPVal b = ppCond();
    return c.v ? a : b;
}

static std::string findInclude(const std::string &name, bool angled, const SrcFile *from, bool next, bool *isSystem);

static bool fileExists(const std::string &p) {
    FILE *f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static int64_t evalIf(Token *tok, Token **rest) {
    Token *start = tok;
    Token *line = copyLine(tok, rest);
    // handle defined(X) / __has_include before macro expansion
    Token head;
    Token *cur = &head;
    for (Token *t = line; t->kind != Token::END;) {
        if (t->kind == Token::IDENT && t->text == "defined") {
            Token *s = t;
            t = t->next;
            bool paren = t->is("(");
            if (paren) t = t->next;
            if (t->kind != Token::IDENT) errorAt(s, "macro name must be an identifier");
            cur = cur->next = makeNumTok(macros.count(t->text) ? 1 : 0, s);
            t = t->next;
            if (paren) {
                if (!t->is(")")) errorAt(t, "expected ')'");
                t = t->next;
            }
            continue;
        }
        if (t->kind == Token::IDENT && (t->text == "__has_include" || t->text == "__has_include_next")) {
            Token *s = t;
            t = t->next;
            if (!t->is("(")) errorAt(t, "expected '('");
            t = t->next;
            std::string name;
            bool angled = false;
            if (t->kind == Token::STR) { name = t->text.substr(1, t->text.size() - 2); t = t->next; }
            else if (t->is("<")) {
                angled = true;
                t = t->next;
                while (!t->is(">") && t->kind != Token::END) { name += t->text; t = t->next; }
                t = t->next;
            }
            if (!t->is(")")) errorAt(t, "expected ')'");
            t = t->next;
            bool sys;
            cur = cur->next = makeNumTok(findInclude(name, angled, s->file, false, &sys).empty() ? 0 : 1, s);
            continue;
        }
        if (t->kind == Token::IDENT && (t->text == "__has_attribute" || t->text == "__has_builtin" ||
                                        t->text == "__has_feature" || t->text == "__has_extension" ||
                                        t->text == "__has_cpp_attribute") && t->next->is("(")) {
            Token *s = t;
            t = t->next->next;
            int depth = 1;
            while (t->kind != Token::END && depth) {
                if (t->is("(")) depth++;
                if (t->is(")")) depth--;
                t = t->next;
            }
            cur = cur->next = makeNumTok(0, s);
            continue;
        }
        cur = cur->next = copyTok(t);
        t = t->next;
    }
    cur->next = newEof(start);
    Token *expanded = preprocess(head.next);
    if (expanded->kind == Token::END) errorAt(start, "#if with no expression");
    ppTok = expanded;
    PPVal v = ppCond();
    if (ppTok->kind != Token::END) errorAt(ppTok, "extra tokens in #if");
    return v.v;
}

// ------------------------------------------------------------ include lookup
static std::string dirOf(const std::string &path) {
    size_t p = path.find_last_of("/\\");
    return p == std::string::npos ? "." : path.substr(0, p);
}
static std::unordered_map<std::string, std::string> includeCache;

static std::string findInclude(const std::string &name, bool angled, const SrcFile *from, bool next, bool *isSystem) {
    *isSystem = false;
    if (name.empty()) return "";
    bool absolute = name[0] == '/' || name[0] == '\\' || (name.size() > 1 && name[1] == ':');
    if (absolute) return fileExists(name) ? name : "";
    std::string fromDir = from ? dirOf(from->name) : ".";
    if (!angled && !next) {
        std::string p = fromDir + "/" + name;
        if (fileExists(p)) { *isSystem = from && from->system; return p; }
    }
    std::vector<std::pair<std::string, bool>> dirs;
    for (auto &d : opt.includeDirs) dirs.push_back({d, false});
    for (auto &d : opt.sysIncludeDirs) dirs.push_back({d, true});
    for (auto &d : opt.afterIncludeDirs) dirs.push_back({d, true});
    bool skipping = next;
    for (auto &d : dirs) {
        if (skipping) { // #include_next: start after the directory of the current file
            std::string a = d.first, b = fromDir;
            std::replace(a.begin(), a.end(), '\\', '/');
            std::replace(b.begin(), b.end(), '\\', '/');
            if (b.compare(0, a.size(), a) == 0) skipping = false;
            continue;
        }
        std::string p = d.first + "/" + name;
        if (fileExists(p)) { *isSystem = d.second; return p; }
    }
    return "";
}

// detect  #ifndef X / #define X ... #endif  include guards so files are read once
static std::string detectGuard(Token *t) {
    if (!(t->is("#") && t->bol)) return "";
    t = t->next;
    if (!(t->kind == Token::IDENT && t->text == "ifndef")) return "";
    t = t->next;
    if (t->kind != Token::IDENT) return "";
    std::string name = t->text;
    t = t->next;
    if (!(t->is("#") && t->bol && t->next->text == "define" && t->next->next->text == name)) return "";
    int depth = 1;
    t = t->next;
    for (; t->kind != Token::END; t = t->next) {
        if (!(t->is("#") && t->bol)) continue;
        const std::string &d = t->next->text;
        if (d == "if" || d == "ifdef" || d == "ifndef") depth++;
        else if (d == "endif") {
            depth--;
            if (depth == 0) {
                Token *e = skipLine(t->next);
                return e->kind == Token::END ? name : "";
            }
        }
    }
    return "";
}

// ------------------------------------------------------------ macro expansion
struct MacroArg {
    Token *tok; // END-terminated
    bool isVa = false;
};

static Token *readDefineBody(Token *tok, Token **rest) { return copyLine(tok, rest); }

static void readDefine(Token *tok, Token **rest) {
    if (tok->kind != Token::IDENT) errorAt(tok, "macro name must be an identifier");
    Macro *m = new Macro;
    m->name = tok->text;
    Token *t = tok->next;
    if (t->is("(") && !t->space && !t->bol) {
        m->funcLike = true;
        t = t->next;
        while (!t->is(")")) {
            if (t->bol || t->kind == Token::END) errorAt(tok, "unterminated macro parameter list");
            if (!m->params.empty() || m->variadic) {
                if (!t->is(",")) errorAt(t, "expected ',' in macro parameter list");
                t = t->next;
            }
            if (t->is("...")) {
                m->variadic = true;
                m->vaName = "__VA_ARGS__";
                t = t->next;
                break;
            }
            if (t->kind != Token::IDENT) errorAt(t, "expected parameter name");
            std::string pname = t->text;
            t = t->next;
            if (t->is("...")) { // GNU named variadic
                m->variadic = true;
                m->vaName = pname;
                t = t->next;
                break;
            }
            m->params.push_back(pname);
        }
        if (!t->is(")")) errorAt(t, "expected ')' in macro parameter list");
        t = t->next;
    }
    m->body = readDefineBody(t, rest);
    macros[m->name] = m;
}

static std::vector<MacroArg> readArgs(Token *tok, Macro *m, Token **rest, Token **rparen) {
    // tok points just after '('
    std::vector<MacroArg> args;
    size_t np = m->params.size();
    Token *start = tok;
    auto readOne = [&](bool va) {
        Token head;
        Token *cur = &head;
        int depth = 0;
        for (;;) {
            if (tok->kind == Token::END) errorAt(start, "unterminated macro invocation of '%s'", m->name.c_str());
            if (depth == 0 && tok->is(")")) break;
            if (depth == 0 && !va && tok->is(",")) break;
            if (tok->is("(")) depth++;
            else if (tok->is(")")) depth--;
            // directives inside macro arguments are not supported; treat tokens literally
            cur = cur->next = copyTok(tok);
            tok = tok->next;
        }
        cur->next = newEof(tok);
        MacroArg a;
        a.tok = head.next;
        a.isVa = va;
        args.push_back(a);
    };
    for (size_t i = 0; i < np; i++) {
        if (i > 0) {
            if (!tok->is(",")) {
                if (tok->is(")") ) { // too few arguments: treat missing ones as empty
                    MacroArg a;
                    a.tok = newEof(tok);
                    args.push_back(a);
                    continue;
                }
                errorAt(tok, "expected ',' in macro arguments");
            }
            tok = tok->next;
        }
        readOne(false);
    }
    if (m->variadic) {
        if (tok->is(")")) {
            MacroArg a;
            a.tok = newEof(tok);
            a.isVa = true;
            args.push_back(a);
        } else {
            if (np > 0) {
                if (!tok->is(",")) errorAt(tok, "expected ','");
                tok = tok->next;
            }
            readOne(true);
        }
    } else if (np == 0 && !tok->is(")")) {
        errorAt(tok, "macro '%s' takes no arguments", m->name.c_str());
    }
    if (!tok->is(")")) errorAt(tok, "too many arguments for macro '%s'", m->name.c_str());
    *rparen = tok;
    *rest = tok->next;
    return args;
}

static int findParam(Macro *m, const Token *t) {
    if (t->kind != Token::IDENT) return -1;
    for (size_t i = 0; i < m->params.size(); i++)
        if (m->params[i] == t->text) return (int)i;
    if (m->variadic && t->text == m->vaName) return (int)m->params.size();
    return -1;
}

static Token *stringize(Token *hash, Token *arg) {
    std::string s = joinTokens(arg, nullptr);
    Token *t = makeStrTok(s, hash);
    // re-quote properly: quoteString escaped \ and " which is what # requires for strings
    return t;
}

static Token *paste(Token *lhs, Token *rhs) {
    std::string s = lhs->text + rhs->text;
    Token *t = tokenizeString(s, lhs->file ? lhs->file->name.c_str() : "<paste>");
    if (t->kind == Token::END) { // both empty
        Token *e = copyTok(lhs);
        return e;
    }
    if (t->next->kind != Token::END && !opt.asmPreprocess)
        errorAt(lhs, "pasting forms '%s', an invalid token", s.c_str());
    Token *r = copyTok(t);
    r->file = lhs->file;
    r->line = lhs->line;
    r->space = lhs->space;
    r->bol = lhs->bol;
    r->hs = nullptr;
    return r;
}

static bool argEmpty(const MacroArg &a) { return a.tok->kind == Token::END; }

static Token *subst(Macro *m, std::vector<MacroArg> &args) {
    Token head;
    Token *cur = &head;
    Token *tok = m->body;
    auto emitCopy = [&](Token *list, const Token *first) {
        bool firstTok = true;
        for (Token *t = list; t->kind != Token::END; t = t->next) {
            cur = cur->next = copyTok(t);
            if (firstTok && first) { cur->space = first->space; cur->bol = false; }
            firstTok = false;
        }
    };
    while (tok->kind != Token::END) {
        // #param
        if (tok->is("#") && m->funcLike) {
            int i = findParam(m, tok->next);
            if (i >= 0) {
                cur = cur->next = stringize(tok, args[i].tok);
                tok = tok->next->next;
                continue;
            }
            if (!opt.asmPreprocess) errorAt(tok->next, "'#' is not followed by a macro parameter");
        }
        // GNU  , ## __VA_ARGS__
        if (tok->is(",") && tok->next->is("##")) {
            int i = findParam(m, tok->next->next);
            if (i >= 0 && args[i].isVa) {
                if (argEmpty(args[i])) {
                    tok = tok->next->next->next;
                } else {
                    cur = cur->next = copyTok(tok);
                    emitCopy(args[i].tok, tok->next->next);
                    tok = tok->next->next->next;
                }
                continue;
            }
        }
        if (tok->is("##")) {
            if (cur == &head) errorAt(tok, "'##' cannot appear at the start of a macro expansion");
            if (tok->next->kind == Token::END) errorAt(tok, "'##' cannot appear at the end of a macro expansion");
            int i = findParam(m, tok->next);
            if (i >= 0) {
                if (!argEmpty(args[i])) {
                    Token *p = paste(cur, args[i].tok);
                    *cur = *p;
                    cur->next = nullptr;
                    for (Token *t = args[i].tok->next; t->kind != Token::END; t = t->next) cur = cur->next = copyTok(t);
                }
                tok = tok->next->next;
                continue;
            }
            Token *p = paste(cur, tok->next);
            *cur = *p;
            cur->next = nullptr;
            tok = tok->next->next;
            continue;
        }
        int i = findParam(m, tok);
        if (i >= 0 && tok->next->is("##")) {
            Token *rhs = tok->next->next;
            if (argEmpty(args[i])) {
                int j = findParam(m, rhs);
                if (j >= 0) emitCopy(args[j].tok, rhs);
                else if (rhs->kind != Token::END) cur = cur->next = copyTok(rhs);
                tok = rhs->kind == Token::END ? rhs : rhs->next;
                continue;
            }
            emitCopy(args[i].tok, tok);
            tok = tok->next; // leave ## for the next iteration
            continue;
        }
        if (i >= 0) {
            Token *ex = preprocess(args[i].tok);
            emitCopy(ex, tok);
            tok = tok->next;
            continue;
        }
        cur = cur->next = copyTok(tok);
        tok = tok->next;
    }
    cur->next = newEof(tok);
    return head.next;
}

// Try to expand the macro at tok. Returns true and sets *rest to the new token list.
static bool expandMacro(Token **rest, Token *tok) {
    if (tok->kind != Token::IDENT) return false;
    auto it = macros.find(tok->text);
    if (it == macros.end()) return false;
    Macro *m = it->second;
    const std::string *iname = intern(m->name);
    if (hsContains(tok->hs, iname)) return false;

    if (m->builtin) {
        Token *t = nullptr;
        switch (m->builtin) {
        case 1: t = makeStrTok(tok->file ? tok->file->name : "", tok); break;
        case 2: t = makeNumTok(tok->line, tok); break;
        case 3: t = makeNumTok(counterValue++, tok); break;
        case 4: t = makeStrTok(baseFile ? baseFile->name : "", tok); break;
        }
        t->next = tok->next;
        *rest = t;
        return true;
    }
    if (!m->funcLike) {
        Hideset *hs = hsUnion(tok->hs, hsNew(iname));
        Token head;
        Token *cur = &head;
        for (Token *t = m->body; t->kind != Token::END; t = t->next) {
            // object-like macros still honour ##
            if (t->is("##") && cur != &head && t->next->kind != Token::END) {
                Token *p = paste(cur, t->next);
                Hideset *keep = cur->hs;
                *cur = *p;
                cur->hs = keep;
                cur->next = nullptr;
                t = t->next;
                continue;
            }
            cur = cur->next = copyTok(t);
            cur->hs = hsUnion(cur->hs, hs);
            cur->file = tok->file;
            cur->line = tok->line;
        }
        if (head.next) { head.next->space = tok->space; head.next->bol = tok->bol; }
        cur->next = tok->next;
        // Preserve line-start flag on the following token when the body is empty
        if (!head.next && tok->bol) tok->next->bol = true;
        *rest = head.next ? head.next : tok->next;
        return true;
    }
    // function-like: needs '('
    if (!tok->next->is("(")) return false;
    Token *rparen = nullptr, *after = nullptr;
    std::vector<MacroArg> args = readArgs(tok->next->next, m, &after, &rparen);
    Hideset *hs = hsIntersect(tok->hs, rparen->hs);
    hs = hsUnion(hs, hsNew(iname));
    Token *body = subst(m, args);
    Token head;
    Token *cur = &head;
    for (Token *t = body; t->kind != Token::END; t = t->next) {
        cur = cur->next = t;
        t->hs = hsUnion(t->hs, hs);
        t->file = tok->file;
        t->line = tok->line;
    }
    if (head.next) { head.next->space = tok->space; head.next->bol = tok->bol; }
    else if (tok->bol) after->bol = true;
    cur->next = after;
    *rest = head.next ? head.next : after;
    return true;
}

// ------------------------------------------------------------ directive processing
struct CondIncl {
    Token *tok;
    bool included;   // some branch already taken
    bool inElse;
};

static Token *skipCondIncl(Token *tok) {
    // skip to matching #elif/#else/#endif at depth 0
    int depth = 0;
    while (tok->kind != Token::END) {
        if (tok->is("#") && tok->bol) {
            const std::string &d = tok->next->text;
            if (d == "if" || d == "ifdef" || d == "ifndef") depth++;
            else if (d == "endif") {
                if (depth == 0) return tok;
                depth--;
            } else if ((d == "elif" || d == "else" || d == "elifdef" || d == "elifndef") && depth == 0)
                return tok;
            tok = skipLine(tok->next);
            continue;
        }
        tok = tok->next;
    }
    return tok;
}

static int includeDepth = 0;

Token *preprocess(Token *tok) {
    Token head;
    Token *cur = &head;
    std::vector<CondIncl> conds;
    while (tok->kind != Token::END) {
        if (expandMacro(&tok, tok)) continue;
        if (!(tok->is("#") && tok->bol)) {
            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }
        Token *start = tok;
        tok = tok->next;
        if (tok->bol || tok->kind == Token::END) continue; // null directive
        const std::string d = tok->text;
        if (d == "include" || d == "include_next" || d == "import") {
            bool next = d == "include_next";
            Token *rest;
            Token *line = copyLine(tok->next, &rest);
            std::string name;
            bool angled = false;
            if (!(line->kind == Token::STR && line->text[0] == '"') && !line->is("<")) line = preprocess(line);
            if (line->kind == Token::STR && line->text[0] == '"') {
                name = line->text.substr(1, line->text.size() - 2);
            } else if (line->is("<")) {
                angled = true;
                Token *t = line->next;
                for (; !t->is(">"); t = t->next) {
                    if (t->kind == Token::END) errorAt(start, "expected '>'");
                    if (t->space && !name.empty()) name += ' ';
                    name += t->text;
                }
            } else
                errorAt(start, "expected \"filename\" or <filename>");
            bool sys = false;
            std::string path = findInclude(name, angled, start->file, next, &sys);
            if (path.empty()) errorAt(start, "%s: file not found", name.c_str());
            tok = rest;
            std::string key = path;
            std::replace(key.begin(), key.end(), '\\', '/');
            if (onceFiles.count(key)) continue;
            auto g = guardOf.find(key);
            if (g != guardOf.end() && macros.count(g->second)) continue;
            SrcFile *f = loadSource(path);
            if (!f) errorAt(start, "%s: cannot read", path.c_str());
            f->system = sys;
            if (++includeDepth > 200) errorAt(start, "#include nested too deeply");
            Token *inc = tokenizeFile(f);
            std::string guard = detectGuard(inc);
            if (!guard.empty()) guardOf[key] = guard;
            // splice: process the included tokens, then continue with the rest
            Token *expanded = preprocess(inc);
            includeDepth--;
            for (Token *t = expanded; t->kind != Token::END;) {
                Token *n = t->next;
                cur = cur->next = t;
                t = n;
            }
            continue;
        }
        if (d == "define") {
            readDefine(tok->next, &tok);
            continue;
        }
        if (d == "undef") {
            tok = tok->next;
            if (tok->kind != Token::IDENT) errorAt(tok, "macro name must be an identifier");
            macros.erase(tok->text);
            tok = skipLine(tok->next);
            continue;
        }
        if (d == "if" || d == "ifdef" || d == "ifndef") {
            bool v;
            if (d == "if") v = evalIf(tok->next, &tok) != 0;
            else {
                Token *n = tok->next;
                if (n->kind != Token::IDENT) errorAt(n, "macro name must be an identifier");
                v = macros.count(n->text) != 0;
                if (d == "ifndef") v = !v;
                tok = skipLine(n->next);
            }
            conds.push_back({start, v, false});
            if (!v) tok = skipCondIncl(tok);
            continue;
        }
        if (d == "elif" || d == "elifdef" || d == "elifndef") {
            if (conds.empty() || conds.back().inElse) errorAt(start, "stray #%s", d.c_str());
            if (conds.back().included) {
                tok = skipCondIncl(skipLine(tok->next));
                continue;
            }
            bool v;
            if (d == "elif") v = evalIf(tok->next, &tok) != 0;
            else {
                Token *n = tok->next;
                v = macros.count(n->text) != 0;
                if (d == "elifndef") v = !v;
                tok = skipLine(n->next);
            }
            if (v) conds.back().included = true;
            else tok = skipCondIncl(tok);
            continue;
        }
        if (d == "else") {
            if (conds.empty() || conds.back().inElse) errorAt(start, "stray #else");
            conds.back().inElse = true;
            tok = skipLine(tok->next);
            if (conds.back().included) tok = skipCondIncl(tok);
            else conds.back().included = true;
            continue;
        }
        if (d == "endif") {
            if (conds.empty()) errorAt(start, "stray #endif");
            conds.pop_back();
            tok = skipLine(tok->next);
            continue;
        }
        if (d == "line") { tok = skipLine(tok->next); continue; }
        if (tok->kind == Token::PPNUM) { tok = skipLine(tok->next); continue; } // # 12 "file"
        if (d == "pragma") {
            Token *rest;
            Token *line = copyLine(tok->next, &rest);
            if (line->text == "once") {
                std::string key = start->file->name;
                std::replace(key.begin(), key.end(), '\\', '/');
                onceFiles.insert(key);
            } else {
                // hand the pragma to the parser as one token
                Token *p = new Token;
                p->kind = Token::OTHER;
                p->text = "#pragma";
                p->sval = joinTokens(line, nullptr);
                p->file = start->file;
                p->line = start->line;
                p->bol = true;
                cur = cur->next = p;
            }
            tok = rest;
            continue;
        }
        if (d == "error") {
            Token *rest;
            Token *line = copyLine(tok->next, &rest);
            errorAt(start, "#error %s", joinTokens(line, nullptr).c_str());
        }
        if (d == "warning") {
            Token *rest;
            Token *line = copyLine(tok->next, &rest);
            warnAt(start, "#warning %s", joinTokens(line, nullptr).c_str());
            tok = rest;
            continue;
        }
        if (d == "ident" || d == "sccs") { tok = skipLine(tok->next); continue; }
        if (opt.asmPreprocess) { // e.g. "# comment" lines in assembly: pass through
            cur = cur->next = start;
            tok = start->next;
            continue;
        }
        errorAt(tok, "invalid preprocessing directive #%s", d.c_str());
    }
    if (!conds.empty()) errorAt(conds.back().tok, "unterminated conditional directive");
    cur->next = tok; // END
    return head.next ? head.next : tok;
}

// ------------------------------------------------------------ -E output
std::string tokensToText(Token *tok) {
    std::string out;
    int line = 1;
    SrcFile *file = nullptr;
    bool first = true;
    for (; tok->kind != Token::END; tok = tok->next) {
        if (tok->kind == Token::OTHER && tok->text == "#pragma") {
            out += "\n#pragma " + tok->sval + "\n";
            first = true;
            continue;
        }
        if (tok->bol || first) {
            if (!first) out += '\n';
            if (opt.asmPreprocess) {
                // keep roughly the same line numbers for assembler diagnostics
                if (tok->file != file) { file = tok->file; line = tok->line; }
                else if (tok->line > line && tok->line - line < 8) {
                    while (line < tok->line) { out += '\n'; line++; }
                } else
                    line = tok->line;
                line++;
            }
            if (tok->space) out += opt.asmPreprocess ? "\t" : " ";
        } else if (tok->space)
            out += ' ';
        first = false;
        out += tok->text;
    }
    out += '\n';
    return out;
}

// ------------------------------------------------------------ number conversion
static int64_t parsePPNumber(Token *t, bool *isUnsigned, bool *isFloat, double *fv, int *rank) {
    const std::string &s = t->text;
    const char *p = s.c_str();
    *isUnsigned = false;
    *isFloat = false;
    *rank = 0; // 0 int, 1 long, 2 long long
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B') && isdigit((unsigned char)p[2])) { base = 2; p += 2; }
    else if (p[0] == '0' && isdigit((unsigned char)p[1])) base = 8;
    // float?
    bool flt = false;
    if (base == 10 || base == 8) {
        for (const char *q = s.c_str(); *q; q++)
            if (*q == '.' || *q == 'e' || *q == 'E') flt = true;
    } else if (base == 16) {
        for (const char *q = p; *q; q++)
            if (*q == '.' || *q == 'p' || *q == 'P') flt = true;
    }
    if (flt) {
        char *end;
        double v = strtod(s.c_str(), &end);
        *isFloat = true;
        *fv = v;
        *rank = 1; // double
        if (*end == 'f' || *end == 'F') { *rank = 0; end++; }
        else if (*end == 'l' || *end == 'L') end++;
        if (*end) errorAt(t, "invalid floating constant");
        return 0;
    }
    uint64_t v = 0;
    const char *q = p;
    for (;; q++) {
        if (*q == '\'' ) continue;
        int dgt;
        if (isdigit((unsigned char)*q)) dgt = *q - '0';
        else if (isalpha((unsigned char)*q)) dgt = tolower(*q) - 'a' + 10;
        else break;
        if (dgt >= base) break;
        v = v * base + dgt;
    }
    bool u = false;
    int l = 0;
    for (; *q; q++) {
        if (*q == 'u' || *q == 'U') u = true;
        else if (*q == 'l' || *q == 'L') l++;
        else errorAt(t, "invalid integer constant");
    }
    *isUnsigned = u;
    *rank = l >= 2 ? 2 : l;
    // pick the type per C rules (int and long are both 32 bits)
    if (*rank < 2) {
        if (base == 10) {
            if (u) { if (v > 0xFFFFFFFFull) *rank = 2; }
            else if (v > 0x7FFFFFFFull) *rank = 2;
        } else {
            if (v > 0xFFFFFFFFull) *rank = 2;
            else if (v > 0x7FFFFFFFull) *isUnsigned = true;
        }
    }
    if (*rank == 2 && !u && v > 0x7FFFFFFFFFFFFFFFull) *isUnsigned = true;
    return (int64_t)v;
}

// Convert preprocessing tokens into parser tokens: numbers, chars, strings.
void convertTokens(Token *tok) {
    for (Token *t = tok; t->kind != Token::END; t = t->next) {
        if (t->kind == Token::PPNUM) {
            bool u, fl;
            double fv = 0;
            int rank;
            int64_t v = parsePPNumber(t, &u, &fl, &fv, &rank);
            t->kind = Token::NUM;
            if (fl) {
                t->fval = fv;
                t->ty = rank == 0 ? tyFloat : tyDouble;
            } else {
                t->ival = v;
                t->ty = rank == 2 ? (u ? tyULLong : tyLLong) : rank == 1 ? (u ? tyULong : tyLong) : (u ? tyUInt : tyInt);
                if (rank < 2) t->ival = u ? (int64_t)(uint32_t)v : (int64_t)(int32_t)v;
            }
        } else if (t->kind == Token::STR) {
            size_t q = t->text.find_first_of("\"'");
            if (t->text[q] == '\'') {
                t->kind = Token::NUM;
                t->ival = charConstValue(t);
                t->ty = tyInt;
                if (q > 0) t->ival &= 0xFFFFFFFF; // wide char: plain value
            } else {
                const char *p = t->text.c_str() + q + 1;
                std::string s;
                while (*p && *p != '"') {
                    if (*p == '\\') { p++; s += (char)readEscape(p); }
                    else s += *p++;
                }
                t->sval = s;
            }
        }
    }
}

} // namespace p68
