// P68 assembler - driver: source reader, statements, directives, macros, conditionals,
// expressions, symbol table, layout/relaxation and the ELF32 relocatable writer.
#include "as_internal.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace p68 { namespace asmblr {

std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    return r;
}
static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) b--;
    return s.substr(a, b - a);
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

Asm::Asm(const AsOptions& o, std::string& d) : opt(o), diag(d) {
    mot = o.motorolaSyntax;
    regOpt = o.motorolaSyntax || o.regPrefixOptional;
    incDirs = o.includeDirs;
}

void Asm::errorAt(const Loc& l, const std::string& msg) {
    failed = true;
    if (quiet) return;
    errors++;
    diag += (l.file < (int)files.size() ? files[l.file] : std::string("?")) + ":" + std::to_string(l.line) + ": error: " + msg + "\n";
}
void Asm::error(const std::string& msg) { errorAt(cur, msg); }
void Asm::warn(const std::string&) {}

// ------------------------------------------------------------------ symbols
std::string Asm::localName(const std::string& name) {
    if (mot && name.size() > 1 && name[0] == '.' && !scope.empty()) return scope + name;
    return name;
}
Symbol* Asm::findSym(const std::string& n0) {
    auto it = symTab.find(localName(n0));
    return it == symTab.end() ? nullptr : it->second;
}
Symbol* Asm::sym(const std::string& n0) {
    std::string name = localName(n0);
    auto it = symTab.find(name);
    if (it != symTab.end()) return it->second;
    symPool.emplace_back();
    Symbol* s = &symPool.back();
    s->name = name;
    symTab[name] = s;
    symOrder.push_back(s);
    return s;
}
Symbol* Asm::dotSym() {
    symPool.emplace_back();
    Symbol* s = &symPool.back();
    s->temp = true;
    s->sec = curSec;
    s->frag = (int)sec().frags.size() - 1;
    s->value = (int64_t)frag().data.size();
    return s;
}
void Asm::defineLabel(const std::string& name) {
    Symbol* s;
    bool numeric = !name.empty() && isDigit(name[0]);
    if (numeric) {
        for (char c : name) if (!isDigit(c)) { error("bad label '" + name + "'"); return; }
        int n = atoi(name.c_str());
        int k = ++localCount[n];
        s = sym(std::string("\x01") + std::to_string(n) + "\x02" + std::to_string(k));
        s->temp = true;
    } else {
        if (mot && name[0] != '.') scope = name;
        s = sym(name);
    }
    if (s->sec != SEC_UNDEF && !(s->sec == SEC_ABS && s->isSet)) { error("symbol '" + name + "' is already defined"); return; }
    s->sec = curSec;
    s->frag = (int)sec().frags.size() - 1;
    s->value = (int64_t)frag().data.size();
    s->isSet = false;
}
void Asm::assign(const std::string& name, const char* p, bool isSet, bool equiv) {
    const Expr* e = parseExpr(p);
    if (!e) return;
    skipWs(p);
    if (*p) { error("junk after expression: '" + std::string(p) + "'"); return; }
    if (name == ".") { // ". = expr" behaves like .org
        int64_t v;
        if (!evalNow(e, v)) { error("non-constant .org"); return; }
        frag().vtype = V_ORG; frag().org = v; frag().loc = cur; newFrag();
        return;
    }
    Symbol* s = sym(name);
    if (s->sec >= 0 || s->sec == SEC_COMMON || (equiv && s->sec != SEC_UNDEF)) { error("symbol '" + name + "' is already defined"); return; }
    (void)isSet;
    int64_t v;
    if (evalNow(e, v)) { s->sec = SEC_ABS; s->value = v; s->expr = nullptr; }
    else {
        // freeze "." and keep the expression for later
        s->sec = SEC_EXPR; s->expr = e;
    }
    s->isSet = true;
}

// ------------------------------------------------------------------ expressions
const Expr* Asm::num(int64_t v) {
    pool.emplace_back();
    pool.back().k = Expr::NUM; pool.back().num = v;
    return &pool.back();
}
static const Expr* mk(std::deque<Expr>& pool, Expr::Kind k, int op, const Expr* a, const Expr* b) {
    pool.emplace_back();
    Expr& e = pool.back();
    e.k = k; e.op = op; e.a = a; e.b = b;
    return &e;
}
static int escChar(const char*& p) { // after backslash
    char c = *p++;
    switch (c) {
    case 'n': return '\n'; case 't': return '\t'; case 'r': return '\r'; case 'b': return '\b';
    case 'f': return '\f'; case 'a': return 7; case 'e': return 27; case 'v': return 11;
    case 'x': case 'X': { int v = 0, h; while ((h = hexVal(*p)) >= 0) { v = v * 16 + h; p++; } return v & 0xFF; }
    default:
        if (c >= '0' && c <= '7') {
            int v = c - '0';
            for (int i = 0; i < 2 && *p >= '0' && *p <= '7'; i++) v = v * 8 + (*p++ - '0');
            return v & 0xFF;
        }
        return (unsigned char)c;
    }
}

const Expr* Asm::primary(const char*& p) {
    skipWs(p);
    char c = *p;
    if (c == '(') {
        p++;
        const Expr* e = binary(p, 0);
        if (!e) return nullptr;
        skipWs(p);
        if (*p != ')') { error("missing ')' in expression"); return nullptr; }
        p++;
        return e;
    }
    if (c == '-' || c == '~' || c == '!' || c == '+') {
        p++;
        const Expr* e = primary(p);
        if (!e) return nullptr;
        if (c == '+') return e;
        if (e->k == Expr::NUM) return num(c == '-' ? -e->num : c == '~' ? ~e->num : !e->num);
        return mk(pool, Expr::UN, c, e, nullptr);
    }
    if (isDigit(c)) {
        int64_t v = 0;
        const char* s = p;
        if (c == '0' && (p[1] == 'x' || p[1] == 'X') && hexVal(p[2]) >= 0) {
            p += 2;
            while (hexVal(*p) >= 0) v = (int64_t)((uint64_t)v * 16 + (unsigned)hexVal(*p++));
        } else if (c == '0' && (p[1] == 'b' || p[1] == 'B') && (p[2] == '0' || p[2] == '1')) {
            p += 2;
            while (*p == '0' || *p == '1') v = v * 2 + (*p++ - '0');
        } else {
            while (isDigit(*p)) p++;
            if ((*p == 'b' || *p == 'f') && !isSymChar(p[1])) { // numeric local label reference
                int n = atoi(std::string(s, p).c_str());
                int k = localCount[n] + (*p == 'f' ? 1 : 0);
                if (k == 0) { error("backward reference to undefined local label"); return nullptr; }
                p++;
                Symbol* ls = sym(std::string("\x01") + std::to_string(n) + "\x02" + std::to_string(k));
                ls->temp = true; ls->used = true;
                const Expr* e = mk(pool, Expr::SYM, 0, nullptr, nullptr);
                const_cast<Expr*>(e)->sym = ls;
                return e;
            }
            if (c == '0' && !mot) { for (const char* q = s; q < p; q++) v = v * 8 + (*q - '0'); }
            else for (const char* q = s; q < p; q++) v = (int64_t)((uint64_t)v * 10 + (unsigned)(*q - '0'));
        }
        return num(v);
    }
    if (mot && c == '$' && hexVal(p[1]) >= 0) {
        p++;
        int64_t v = 0;
        while (hexVal(*p) >= 0) v = (int64_t)((uint64_t)v * 16 + (unsigned)hexVal(*p++));
        return num(v);
    }
    if (mot && c == '%' && (p[1] == '0' || p[1] == '1')) {
        p++;
        int64_t v = 0;
        while (*p == '0' || *p == '1') v = v * 2 + (*p++ - '0');
        return num(v);
    }
    if (mot && c == '@' && p[1] >= '0' && p[1] <= '7') {
        p++;
        int64_t v = 0;
        while (*p >= '0' && *p <= '7') v = v * 8 + (*p++ - '0');
        return num(v);
    }
    if (c == '\'' || (mot && c == '"')) {
        char q = c;
        p++;
        int64_t v = 0;
        if (mot) {
            while (*p && *p != q) v = (v << 8) | (unsigned char)*p++;
            if (*p == q) p++;
        } else {
            if (*p == '\\') { p++; v = escChar(p); }
            else if (*p) v = (unsigned char)*p++;
            if (*p == '\'') p++;
        }
        return num(v);
    }
    if (mot && c == '*') {
        p++;
        const Expr* e = mk(pool, Expr::SYM, 0, nullptr, nullptr);
        const_cast<Expr*>(e)->sym = dotSym();
        return e;
    }
    if (isSymStart(c)) {
        const char* s = p;
        while (isSymChar(*p)) p++;
        std::string name(s, p);
        Symbol* sy;
        if (name == ".") sy = dotSym();
        else {
            if (*p == '@') { // name@PLT and friends: ignore the suffix
                const char* q = p + 1;
                while (isSymChar(*q)) q++;
                std::string suf = lower(std::string(p + 1, q));
                if (suf == "plt" || suf == "plt_pc" || suf == "pltpc") p = q;
            }
            sy = sym(name);
            sy->used = true;
        }
        const Expr* e = mk(pool, Expr::SYM, 0, nullptr, nullptr);
        const_cast<Expr*>(e)->sym = sy;
        return e;
    }
    error(std::string("bad expression near '") + p + "'");
    return nullptr;
}

static int binOp(const char* p, int& len, bool mot) {
    len = 2;
    if (p[0] == '<' && p[1] == '<') return 'L';
    if (p[0] == '>' && p[1] == '>') return 'R';
    if (p[0] == '=' && p[1] == '=') return 'e';
    if (p[0] == '!' && p[1] == '=') return 'n';
    if (p[0] == '<' && p[1] == '>') return 'n';
    if (p[0] == '<' && p[1] == '=') return 'l';
    if (p[0] == '>' && p[1] == '=') return 'g';
    if (p[0] == '&' && p[1] == '&') return 'A';
    if (p[0] == '|' && p[1] == '|') return 'O';
    len = 1;
    switch (p[0]) {
    case '*': case '/': case '%': case '+': case '-': case '&': case '|': case '^': case '<': case '>': return p[0];
    case '!': return mot ? '|' : '!';
    case '=': return mot ? 'e' : 0;
    }
    return 0;
}
static int precOf(int op) {
    switch (op) {
    case '*': case '/': case '%': case 'L': case 'R': return 7;
    case '|': case '&': case '^': case '!': return 6;
    case '+': case '-': return 5;
    case 'e': case 'n': case '<': case '>': case 'l': case 'g': return 4;
    case 'A': return 3;
    default: return 2;
    }
}
static bool applyBin(int op, int64_t a, int64_t b, int64_t& r) {
    switch (op) {
    case '*': r = (int64_t)((uint64_t)a * (uint64_t)b); break;
    case '/': if (!b) return false; r = a / b; break;
    case '%': if (!b) return false; r = a % b; break;
    case 'L': r = (b < 0 || b > 63) ? 0 : (int64_t)((uint64_t)a << b); break;
    case 'R': r = (b < 0 || b > 63) ? 0 : (int64_t)((uint64_t)a >> b); break;
    case '|': r = a | b; break;
    case '&': r = a & b; break;
    case '^': r = a ^ b; break;
    case '!': r = a | ~b; break;
    case '+': r = a + b; break;
    case '-': r = a - b; break;
    case 'e': r = a == b ? -1 : 0; break;
    case 'n': r = a != b ? -1 : 0; break;
    case '<': r = a < b ? -1 : 0; break;
    case '>': r = a > b ? -1 : 0; break;
    case 'l': r = a <= b ? -1 : 0; break;
    case 'g': r = a >= b ? -1 : 0; break;
    case 'A': r = (a && b) ? 1 : 0; break;
    case 'O': r = (a || b) ? 1 : 0; break;
    default: return false;
    }
    return true;
}
const Expr* Asm::binary(const char*& p, int minPrec) {
    const Expr* l = primary(p);
    if (!l) return nullptr;
    for (;;) {
        const char* q = p;
        skipWs(q);
        int len;
        int op = binOp(q, len, mot);
        if (!op) break;
        int pr = precOf(op);
        if (pr < minPrec || pr == 0) break;
        if (op == '%' && !mot && isSymStart(q[1]) && regOpt == false) {
            // "%name" directly after an expression is a register, not modulo
            break;
        }
        q += len;
        p = q;
        const Expr* r = binary(p, pr + 1);
        if (!r) return nullptr;
        int64_t v;
        if (l->k == Expr::NUM && r->k == Expr::NUM && applyBin(op, l->num, r->num, v)) l = num(v);
        else l = mk(pool, Expr::BIN, op, l, r);
    }
    return l;
}
const Expr* Asm::parseExpr(const char*& p) { return binary(p, 1); }
const Expr* Asm::parseExprStr(const std::string& s) {
    const char* p = s.c_str();
    const Expr* e = parseExpr(p);
    if (!e) return nullptr;
    skipWs(p);
    if (*p) { error("junk in expression: '" + s + "'"); return nullptr; }
    return e;
}
bool Asm::constExpr(const char*& p, int64_t& v) {
    const Expr* e = parseExpr(p);
    if (!e) return false;
    if (!evalNow(e, v)) { error("expression is not constant"); return false; }
    return true;
}

void Asm::reduce(Val& v, bool fin) {
    if (v.t.empty()) return;
    std::vector<Term> t;
    for (const Term& x : v.t) {
        bool found = false;
        for (Term& y : t) if (y.s == x.s) { y.coef += x.coef; found = true; break; }
        if (!found) t.push_back(x);
    }
    t.erase(std::remove_if(t.begin(), t.end(), [](const Term& x) { return x.coef == 0; }), t.end());
    // cancel label differences inside one section
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i].coef <= 0 || t[i].s->sec < 0) continue;
        for (size_t j = 0; j < t.size() && t[i].coef > 0; j++) {
            if (t[j].coef >= 0 || t[j].s->sec != t[i].s->sec) continue;
            Symbol* a = t[i].s; Symbol* b = t[j].s;
            if (fin) v.c += (int64_t)symAddr(a) - (int64_t)symAddr(b);
            else if (a->frag == b->frag) v.c += a->value - b->value;
            else continue;
            t[i].coef--; t[j].coef++;
            j--;
        }
    }
    t.erase(std::remove_if(t.begin(), t.end(), [](const Term& x) { return x.coef == 0; }), t.end());
    v.t.swap(t);
}
Val Asm::eval(const Expr* e, bool fin) {
    Val v;
    switch (e->k) {
    case Expr::NUM: v.c = e->num; break;
    case Expr::SYM: {
        Symbol* s = e->sym;
        if (s->sec == SEC_ABS) v.c = s->value;
        else if (s->sec == SEC_EXPR) {
            if (s->evalBusy > 8) v.bad = true;
            else { s->evalBusy++; v = eval(s->expr, fin); s->evalBusy--; }
        } else v.t.push_back({s, 1});
        break;
    }
    case Expr::UN:
        v = eval(e->a, fin);
        if (e->op == '-') { v.c = -v.c; for (Term& x : v.t) x.coef = -x.coef; }
        else {
            reduce(v, fin);
            if (!v.t.empty()) v.bad = true;
            else v.c = e->op == '~' ? ~v.c : !v.c;
        }
        break;
    case Expr::BIN: {
        v = eval(e->a, fin);
        Val r = eval(e->b, fin);
        if (r.bad) v.bad = true;
        if (e->op == '+' || e->op == '-') {
            int sg = e->op == '+' ? 1 : -1;
            v.c += sg * r.c;
            for (Term& x : r.t) v.t.push_back({x.s, x.coef * sg});
        } else {
            reduce(v, fin); reduce(r, fin);
            if (e->op == '*' && r.t.empty() && !v.t.empty()) { v.c *= r.c; for (Term& x : v.t) x.coef *= r.c; }
            else if (e->op == '*' && v.t.empty() && !r.t.empty()) { int64_t k = v.c; v = r; v.c *= k; for (Term& x : v.t) x.coef *= k; }
            else if (!v.t.empty() || !r.t.empty()) v.bad = true;
            else if (!applyBin(e->op, v.c, r.c, v.c)) v.bad = true;
        }
        break;
    }
    }
    return v;
}
bool Asm::evalNow(const Expr* e, int64_t& out) {
    if (e->k == Expr::NUM) { out = e->num; return true; }
    Val v = eval(e, false);
    if (v.bad) return false;
    reduce(v, false);
    if (!v.t.empty()) return false;
    out = v.c;
    return true;
}

// ------------------------------------------------------------------ sections / emission
static void defaultSecAttrs(const std::string& n, uint32_t& type, uint32_t& flags) {
    auto starts = [&](const char* p) { size_t l = strlen(p); return n.compare(0, l, p) == 0 && (n.size() == l || n[l] == '.' || p[l - 1] == '.'); };
    type = SHT_PROGBITS; flags = 0;
    if (starts(".text") || starts(".init") || starts(".fini") || starts(".gnu.linkonce.t.")) flags = SHF_ALLOC | SHF_EXECINSTR;
    else if (starts(".bss") || starts(".sbss") || starts(".gnu.linkonce.b.")) { flags = SHF_ALLOC | SHF_WRITE; type = SHT_NOBITS; }
    else if (starts(".rodata") || starts(".gnu.linkonce.r.") || starts(".eh_frame") || starts(".gcc_except_table")) flags = SHF_ALLOC;
    else if (starts(".init_array")) { flags = SHF_ALLOC | SHF_WRITE; type = SHT_INIT_ARRAY; }
    else if (starts(".fini_array")) { flags = SHF_ALLOC | SHF_WRITE; type = SHT_FINI_ARRAY; }
    else if (starts(".preinit_array")) { flags = SHF_ALLOC | SHF_WRITE; type = SHT_PREINIT_ARRAY; }
    else if (starts(".data") || starts(".sdata") || starts(".ctors") || starts(".dtors") || starts(".gnu.linkonce.d.") || starts(".tdata")) flags = SHF_ALLOC | SHF_WRITE;
    else if (starts(".comment")) flags = SHF_MERGE | SHF_STRINGS;
    else if (starts(".note")) type = SHT_NOTE;
}
int Asm::findSection(const std::string& name, bool create, uint32_t type, uint32_t flags, bool explicitFlags) {
    for (size_t i = 0; i < secs.size(); i++) if (secs[i].name == name) return (int)i;
    if (!create) return -1;
    Section s;
    s.name = name;
    if (explicitFlags) { s.type = type; s.flags = flags; }
    else defaultSecAttrs(name, s.type, s.flags);
    if (s.flags & SHF_MERGE) s.entsize = 1;
    s.align = (name == ".text" || name == ".data" || name == ".bss") ? 4 : 1;
    s.frags.emplace_back();
    secs.push_back(s);
    return (int)secs.size() - 1;
}
void Asm::switchSection(int idx) {
    if (idx == curSec) return;
    prevSec = curSec;
    curSec = idx;
}
void Asm::newFrag() { secs[curSec].frags.emplace_back(); }
void Asm::emitExpr(const Expr* e, int size) {
    int64_t v;
    Bytes& d = frag().data;
    if (evalNow(e, v)) {
        for (int i = size - 1; i >= 0; i--) d.push_back((unsigned char)(i >= 8 ? 0 : ((uint64_t)v >> (8 * i))));
        if (v && sec().type == SHT_NOBITS) sec().nonZero = true;
        return;
    }
    if (size > 4) { error("64-bit value must be constant"); return; }
    Fixup f;
    f.off = (uint32_t)d.size(); f.size = size; f.e = e; f.loc = cur;
    frag().fix.push_back(f);
    d.insert(d.end(), (size_t)size, 0);
    if (sec().type == SHT_NOBITS) sec().nonZero = true;
}
void Asm::emitBuild(Build& b) {
    if (sec().type == SHT_NOBITS) { error("instruction in a no-bits section"); return; }
    Frag* f = &frag();
    size_t base = f->data.size();
    size_t cut = b.rtype ? b.relaxAt : b.b.size();
    f->data.insert(f->data.end(), b.b.begin(), b.b.begin() + (long)cut);
    for (Fixup fx : b.fx) if (fx.off < cut) { fx.off += (uint32_t)base; fx.loc = cur; f->fix.push_back(fx); }
    if (!b.rtype) return;
    f->vtype = V_RELAX; f->rtype = b.rtype; f->state = b.rstate; f->opOff = (int)(base + b.relaxOp);
    f->target = b.rexpr; f->loc = cur;
    newFrag();
    f = &frag();
    f->data.insert(f->data.end(), b.b.begin() + (long)cut, b.b.end());
    for (Fixup fx : b.fx) if (fx.off >= cut) { fx.off -= (uint32_t)cut; fx.loc = cur; f->fix.push_back(fx); }
}
void Asm::doAlign(int64_t align, int fill, int maxSkip) {
    if (align <= 1) return;
    if (align & (align - 1)) { error("alignment is not a power of 2"); return; }
    Frag& f = frag();
    f.vtype = V_ALIGN; f.align = (int)align; f.fill = fill; f.maxSkip = maxSkip; f.loc = cur;
    if (sec().align < (uint32_t)align && !maxSkip) sec().align = (uint32_t)align;
    newFrag();
}

bool Asm::parseString(const char*& p, std::string& out) {
    skipWs(p);
    char q = *p;
    if (q != '"' && !(mot && q == '\'')) { error("string expected"); return false; }
    p++;
    out.clear();
    while (*p && *p != q) {
        if (*p == '\\' && q == '"') { p++; if (!*p) break; out += (char)escChar(p); }
        else out += *p++;
    }
    if (*p != q) { error("unterminated string"); return false; }
    p++;
    return true;
}
void Asm::stringDir(const char* p, bool zero) {
    for (;;) {
        std::string s;
        if (!parseString(p, s)) return;
        emitBytes((const unsigned char*)s.data(), s.size());
        if (zero) emit8(0);
        if (!s.empty() && sec().type == SHT_NOBITS) sec().nonZero = true;
        skipWs(p);
        if (*p != ',') break;
        p++;
    }
    if (*p) error("junk after string");
}
void Asm::dataDir(int size, const char* p, bool motStrings) {
    skipWs(p);
    if (!*p) { if (!mot) emitExpr(num(0), size); return; }
    for (;;) {
        skipWs(p);
        if ((*p == '"' || (mot && *p == '\'')) && (motStrings || *p == '"')) {
            // a quoted string as a whole item -> characters
            const char* q = p;
            std::string s;
            if (parseString(q, s)) {
                const char* r = q;
                skipWs(r);
                if (*r == ',' || !*r) {
                    for (unsigned char ch : s) { for (int i = 1; i < size; i++) emit8(0); emit8(ch); }
                    if (!s.empty() && sec().type == SHT_NOBITS) sec().nonZero = true;
                    p = r;
                    if (*p != ',') break;
                    p++;
                    continue;
                }
            } else return;
        }
        const Expr* e = parseExpr(p);
        if (!e) return;
        emitExpr(e, size);
        skipWs(p);
        if (*p != ',') break;
        p++;
    }
    if (*p) error(std::string("junk at end of line: '") + p + "'");
}
void Asm::floatDir(int fmt, const char* p) {
    for (;;) {
        skipWs(p);
        const char* s = p;
        while (*p && *p != ',') p++;
        Bytes b;
        if (!parseFloatImm(trim(std::string(s, p)), fmt, b)) { error("bad floating point constant"); return; }
        emitBytes(b.data(), b.size());
        if (sec().type == SHT_NOBITS) sec().nonZero = true;
        if (*p != ',') break;
        p++;
    }
}
void Asm::spaceDir(int64_t count, int size, int64_t fillv) {
    if (count < 0) { error("negative repeat count"); return; }
    if (count * size > (int64_t)256 * 1024 * 1024) { error("space too large"); return; }
    Bytes& d = frag().data;
    if (fillv == 0 || size == 1) d.insert(d.end(), (size_t)(count * size), (unsigned char)fillv);
    else for (int64_t i = 0; i < count; i++) for (int k = size - 1; k >= 0; k--) d.push_back((unsigned char)(k >= 8 ? 0 : ((uint64_t)fillv >> (8 * k))));
    if (fillv && sec().type == SHT_NOBITS) sec().nonZero = true;
}

// ------------------------------------------------------------------ source reader
void Asm::loadSource(const std::string& src, int fileIdx, std::vector<SrcLine>& out) {
    int file = fileIdx, line = 1;
    bool inBlock = false;
    std::string cur_;
    int startLine = 1;
    size_t n = src.size();
    auto flush = [&]() {
        // line markers / line comments
        size_t i = 0;
        while (i < cur_.size() && (cur_[i] == ' ' || cur_[i] == '\t')) i++;
        if (i < cur_.size() && cur_[i] == '#') {
            size_t j = i + 1;
            while (j < cur_.size() && cur_[j] == ' ') j++;
            if (cur_.compare(j, 4, "line") == 0) { j += 4; while (j < cur_.size() && cur_[j] == ' ') j++; }
            if (j < cur_.size() && isDigit(cur_[j])) {
                int ln = atoi(cur_.c_str() + j);
                while (j < cur_.size() && isDigit(cur_[j])) j++;
                while (j < cur_.size() && cur_[j] == ' ') j++;
                if (j < cur_.size() && cur_[j] == '"') {
                    size_t k = cur_.find('"', j + 1);
                    if (k != std::string::npos) {
                        std::string fn = cur_.substr(j + 1, k - j - 1);
                        size_t fi = 0;
                        for (; fi < files.size(); fi++) if (files[fi] == fn) break;
                        if (fi == files.size()) files.push_back(fn);
                        file = (int)fi;
                    }
                }
                line = ln - 1; // the following line gets number ln
            }
            cur_.clear();
            return;
        }
        if (i < cur_.size() && cur_[i] == '*' && (mot || true)) { cur_.clear(); return; }
        while (!cur_.empty() && (cur_.back() == '\r' || cur_.back() == ' ' || cur_.back() == '\t')) cur_.pop_back();
        if (i < cur_.size()) out.push_back({cur_, file, startLine});
        cur_.clear();
    };
    char cmt = mot ? ';' : '|';
    for (size_t i = 0; i < n; i++) {
        char c = src[i];
        if (inBlock) {
            if (c == '*' && i + 1 < n && src[i + 1] == '/') { inBlock = false; i++; cur_ += ' '; }
            else if (c == '\n') { line++; }
            continue;
        }
        if (c == '\n') { flush(); line++; startLine = line; continue; }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') { inBlock = true; i++; continue; }
        if (c == '"' || (c == '\'' && mot)) {
            char q = c;
            cur_ += c;
            for (i++; i < n && src[i] != '\n'; i++) {
                cur_ += src[i];
                if (src[i] == '\\' && q == '"' && i + 1 < n && src[i + 1] != '\n') cur_ += src[++i];
                else if (src[i] == q) break;
            }
            if (i < n && src[i] == '\n') i--;
            continue;
        }
        if (c == '\'' && !mot) {
            cur_ += c;
            if (i + 1 < n && src[i + 1] != '\n') {
                cur_ += src[++i];
                if (src[i] == '\\' && i + 1 < n && src[i + 1] != '\n') cur_ += src[++i];
            }
            continue;
        }
        if (c == cmt) {
            while (i < n && src[i] != '\n') i++;
            i--;
            continue;
        }
        cur_ += c;
    }
    flush();
}

static std::string firstTokens(const std::string& line, std::string* second) {
    // lower-cased opcode token of a line, skipping "label:" prefixes; optionally the token after it
    const char* p = line.c_str();
    std::string tok;
    for (;;) {
        skipWs(p);
        const char* q = p;
        while (isSymChar(*q)) q++;
        tok = lower(std::string(p, q));
        const char* r = q;
        skipWs(r);
        if (*r == ':' && q != p) { p = r + 1; continue; }
        if (second) {
            const char* t = r;
            while (isSymChar(*t)) t++;
            *second = lower(std::string(r, t));
        }
        break;
    }
    return tok;
}
static std::string undot(const std::string& s) { return (!s.empty() && s[0] == '.') ? s.substr(1) : s; }

bool Asm::collectBody(std::vector<SrcLine>& body, const char* open1, const char* open2, const char* close1, const char* close2) {
    Frame& f = frames.back();
    int depth = 1;
    Loc start = cur;
    while (f.pos < f.lines.size()) {
        const SrcLine& l = f.lines[f.pos++];
        std::string second;
        std::string t = firstTokens(l.text, &second);
        std::string u = undot(t), u2 = undot(second);
        bool dotted = !t.empty() && t[0] == '.';
        bool ok1 = dotted || mot;
        auto is = [&](const std::string& x, const char* a, const char* b) { return (a && x == a) || (b && x == b); };
        if (ok1 && is(u, open1, open2)) depth++;
        else if (mot && is(u2, open1, open2) && !is(u, close1, close2)) depth++;
        else if (ok1 && is(u, close1, close2)) { if (--depth == 0) return true; }
        body.push_back(l);
    }
    errorAt(start, "unterminated block (missing end directive)");
    return false;
}

static void splitArgs(const std::string& s, std::vector<std::string>& out, bool wsSplit) {
    std::string curArg;
    int depth = 0;
    bool any = false;
    static const char opsChars[] = "+-*/&|^<>=!~,(";
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"' || c == '\'') {
            char q = c;
            curArg += c;
            for (i++; i < s.size(); i++) {
                curArg += s[i];
                if (s[i] == '\\' && i + 1 < s.size()) curArg += s[++i];
                else if (s[i] == q) break;
                if (q == '\'' && !any) {}
            }
            any = true;
            continue;
        }
        if (c == '(' || c == '[' || c == '{') depth++;
        if (c == ')' || c == ']' || c == '}') depth--;
        if (c == ',' && depth <= 0) { out.push_back(trim(curArg)); curArg.clear(); any = true; continue; }
        if ((c == ' ' || c == '\t') && depth <= 0 && wsSplit) {
            size_t j = i;
            while (j < s.size() && (s[j] == ' ' || s[j] == '\t')) j++;
            std::string t = trim(curArg);
            char prev = t.empty() ? ',' : t.back();
            char next = j < s.size() ? s[j] : ',';
            if (!strchr(opsChars, prev) && !strchr("+-*/&|^<>=!~,)", next) && !t.empty()) {
                out.push_back(t); curArg.clear(); any = true;
            } else curArg += ' ';
            i = j - 1;
            continue;
        }
        curArg += c;
    }
    std::string t = trim(curArg);
    if (!t.empty() || any) out.push_back(t);
    if (!out.empty() && out.back().empty() && !any) out.pop_back();
}
static std::string unquote(const std::string& s) {
    if (s.size() >= 2 && s[0] == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
}

static std::string substitute(const std::string& line, const std::vector<std::string>& names, const std::vector<std::string>& vals,
                              int counter, bool motStyle, const std::string& size0) {
    std::string out;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c != '\\' || i + 1 >= line.size()) { out += c; continue; }
        char d = line[i + 1];
        if (d == '@') { out += motStyle ? "_" + std::to_string(counter) : std::to_string(counter); i++; continue; }
        if (d == '(' && i + 2 < line.size() && line[i + 2] == ')') { i += 2; continue; }
        if (motStyle && d >= '0' && d <= '9') {
            if (d == '0') out += size0;
            else if ((size_t)(d - '1') < vals.size()) out += vals[(size_t)(d - '1')];
            i++;
            continue;
        }
        if (isSymStart(d) && d != '.') {
            size_t j = i + 1;
            while (j < line.size() && isSymChar(line[j]) && line[j] != '.') j++;
            // longest formal name that is a prefix
            size_t best = names.size(), bestLen = 0;
            for (size_t k = 0; k < names.size(); k++) {
                size_t l = names[k].size();
                if (l > bestLen && l <= j - (i + 1) && line.compare(i + 1, l, names[k]) == 0 && l == j - (i + 1)) { best = k; bestLen = l; }
            }
            if (best == names.size()) {
                for (size_t k = 0; k < names.size(); k++) {
                    size_t l = names[k].size();
                    if (l > bestLen && line.compare(i + 1, l, names[k]) == 0) { best = k; bestLen = l; }
                }
            }
            if (best != names.size()) { out += vals[best]; i += bestLen; continue; }
        }
        out += c;
    }
    return out;
}

void Asm::expandMacro(const Macro& m, const char* args, const std::string& sizeSuffix) {
    std::vector<std::string> given;
    splitArgs(args, given, !mot);
    std::vector<std::string> names, vals;
    std::vector<bool> set;
    for (const Macro::Arg& a : m.args) { names.push_back(a.name); vals.push_back(a.def); set.push_back(false); }
    size_t pos = 0;
    for (size_t gi = 0; gi < given.size(); gi++) {
        const std::string& g = given[gi];
        size_t eq = g.find('=');
        bool kw = false;
        if (eq != std::string::npos && eq > 0) {
            std::string k = trim(g.substr(0, eq));
            for (size_t i = 0; i < names.size(); i++) if (names[i] == k) { vals[i] = unquote(trim(g.substr(eq + 1))); set[i] = true; kw = true; break; }
        }
        if (kw) continue;
        if (mot && m.args.empty()) { vals.push_back(g); names.push_back(std::string()); continue; }
        if (pos < m.args.size()) {
            if (m.args[pos].vararg) {
                std::string rest = g;
                for (size_t j = gi + 1; j < given.size(); j++) rest += "," + given[j];
                vals[pos] = rest; set[pos] = true; pos++;
                break;
            }
            vals[pos] = unquote(g); set[pos] = true; pos++;
        } else { error("too many arguments for macro '" + m.name + "'"); return; }
    }
    for (size_t i = 0; i < m.args.size(); i++) if (m.args[i].req && !set[i]) { error("missing required argument '" + m.args[i].name + "' of macro '" + m.name + "'"); return; }
    if (frames.size() > 200) { error("macro nesting too deep"); return; }
    Frame f;
    f.isMacro = true;
    f.condDepth = conds.size();
    int counter = macroCounter++;
    bool motStyle = mot && m.args.empty();
    for (const SrcLine& l : m.body) f.lines.push_back({substitute(l.text, names, vals, counter, motStyle, sizeSuffix), cur.file, cur.line});
    frames.push_back(f);
}

// ------------------------------------------------------------------ conditionals
bool Asm::conditional(const std::string& name, const char* args) {
    std::string d = undot(name);
    if (name[0] != '.' && !mot) return false;
    int kind = 0; // 1 if-expr-nonzero, 2 zero, 3 def, 4 ndef, 5 ifc, 6 ifnc, 7 ifb, 8 ifnb, 9 >0, 10 >=0, 11 <0, 12 <=0
    if (d == "if" || d == "ifne") kind = 1;
    else if (d == "ifeq") kind = 2;
    else if (d == "ifdef" || d == "ifd") kind = 3;
    else if (d == "ifndef" || d == "ifnotdef" || d == "ifnd") kind = 4;
    else if (d == "ifc" || d == "ifeqs") kind = 5;
    else if (d == "ifnc" || d == "ifnes") kind = 6;
    else if (d == "ifb") kind = 7;
    else if (d == "ifnb") kind = 8;
    else if (d == "ifgt") kind = 9;
    else if (d == "ifge") kind = 10;
    else if (d == "iflt") kind = 11;
    else if (d == "ifle") kind = 12;
    auto evalCond = [&](int k) -> bool {
        const char* p = args;
        skipWs(p);
        if (k == 3 || k == 4) {
            const char* q = p;
            while (isSymChar(*q)) q++;
            std::string n(p, q);
            Symbol* s = findSym(n);
            bool def = (s && s->sec != SEC_UNDEF) || macros.count(n);
            return (k == 3) == def;
        }
        if (k == 5 || k == 6) {
            std::vector<std::string> a;
            splitArgs(p, a, false);
            while (a.size() < 2) a.push_back("");
            auto strip = [](std::string s) { if (s.size() >= 2 && (s[0] == '"' || s[0] == '\'') && s.back() == s[0]) return s.substr(1, s.size() - 2); return s; };
            return (k == 5) == (strip(a[0]) == strip(a[1]));
        }
        if (k == 7 || k == 8) return (k == 7) == (*p == 0);
        int64_t v;
        if (!constExpr(p, v)) return false;
        switch (k) { case 1: return v != 0; case 2: return v == 0; case 9: return v > 0; case 10: return v >= 0; case 11: return v < 0; default: return v <= 0; }
    };
    if (kind) {
        Cond c;
        c.parentActive = !skipping();
        c.seenElse = false;
        c.taken = c.parentActive ? evalCond(kind) : false;
        c.active = c.parentActive && c.taken;
        conds.push_back(c);
        return true;
    }
    if (d == "else" || d == "elsec") {
        if (conds.empty()) { error(".else without .if"); return true; }
        Cond& c = conds.back();
        if (c.seenElse) error("duplicate .else");
        c.seenElse = true;
        c.active = c.parentActive && !c.taken;
        c.taken = true;
        return true;
    }
    if (d == "elseif" || d == "elif") {
        if (conds.empty()) { error(".elseif without .if"); return true; }
        Cond& c = conds.back();
        if (c.taken || !c.parentActive) c.active = false;
        else { c.taken = evalCond(1); c.active = c.taken; }
        return true;
    }
    if (d == "endif" || d == "endc") {
        if (conds.empty()) { error(".endif without .if"); return true; }
        conds.pop_back();
        return true;
    }
    return false;
}

// ------------------------------------------------------------------ directives
static const char* const motDirs[] = {
    "dc.b", "dc.w", "dc.l", "dc.s", "dc.d", "dc.x", "dc", "ds.b", "ds.w", "ds.l", "ds", "dcb.b", "dcb.w", "dcb.l", "dcb", "equ", "set", "org",
    "even", "odd", "cnop", "xdef", "xref", "public", "extern", "global", "globl", "section", "rsreset", "rsset", "rs.b", "rs.w", "rs.l", "rs",
    "incbin", "include", "incdir", "macro", "endm", "rept", "endr", "end", "text", "data", "bss", "code", "align", "opt", "fail", "equr",
    "nolist", "list", "ttl", "page", "idnt", "output", "mexit", "exitm", "irp", "irpc", "weak", "comm", "lcomm", "ascii", "asciz", "string",
    "byte", "word", "long", "short", "balign", "p2align", "space", "skip", "zero", "fill", "blk.b", "blk.w", "blk.l", "cpu", "mc68000", "mc68010",
    "mc68020", "mc68030", "mc68040", "mc68060", "mc68881", "machine", "fpu", "dx.b", "dx.w", "dx.l", "type", "size", "quad", "int", nullptr };
static bool isMotDirective(const std::string& n) {
    for (int i = 0; motDirs[i]; i++) if (n == motDirs[i]) return true;
    return false;
}

bool Asm::directive(const std::string& name, const char* p, const std::string& label) {
    bool dotted = name[0] == '.';
    std::string d = dotted ? name.substr(1) : name;
    if (!dotted && !(mot && isMotDirective(d))) return false;
    skipWs(p);
    auto symList = [&](int what) {
        for (;;) {
            skipWs(p);
            const char* q = p;
            while (isSymChar(*q)) q++;
            if (q == p) { error("symbol name expected"); return; }
            Symbol* s = sym(std::string(p, q));
            switch (what) {
            case 0: s->global = true; break;
            case 1: s->weak = true; break;
            case 2: s->declLocal = true; break;
            case 3: s->vis = 2; break;
            case 4: s->vis = 1; break;
            case 5: s->vis = 3; break;
            default: break;
            }
            p = q;
            skipWs(p);
            if (*p != ',') break;
            p++;
        }
    };
    auto sizeOf = [&](const std::string& n, int def) -> int {
        size_t dot = n.find('.');
        if (dot == std::string::npos) return def;
        char c = n[dot + 1];
        return c == 'b' ? 1 : c == 'w' ? 2 : c == 'l' ? 4 : c == 's' ? -4 : c == 'd' ? -8 : c == 'x' ? -12 : c == 'p' ? -13 : def;
    };
    auto endCheck = [&]() { skipWs(p); if (*p) error(std::string("junk at end of line: '") + p + "'"); };

    // ---- data
    if (d == "byte") { dataDir(1, p, mot); return true; }
    if (d == "word" || d == "short" || d == "hword" || d == "2byte") { dataDir(2, p, false); return true; }
    if (d == "long" || d == "int" || d == "4byte") { dataDir(4, p, false); return true; }
    if (d == "quad" || d == "8byte") { dataDir(8, p, false); return true; }
    if (d == "dc" || d.compare(0, 3, "dc.") == 0) {
        int sz = sizeOf(d, 2);
        if (sz < 0) floatDir(sz == -4 ? 'f' : sz == -8 ? 'F' : sz == -12 ? 'x' : 'p', p);
        else dataDir(sz, p, true);
        return true;
    }
    if (d == "ascii") { stringDir(p, false); return true; }
    if (d == "asciz" || d == "string") { stringDir(p, true); return true; }
    if (d == "float" || d == "single") { floatDir('f', p); return true; }
    if (d == "double") { floatDir('F', p); return true; }
    if (d == "extend" || d == "ldouble") { floatDir('x', p); return true; }
    if (d == "space" || d == "skip" || d == "zero" || d == "ds" || d.compare(0, 3, "ds.") == 0 || d.compare(0, 4, "blk.") == 0 ||
        d.compare(0, 3, "dx.") == 0 || d == "dcb" || d.compare(0, 4, "dcb.") == 0) {
        int sz = (d == "space" || d == "skip" || d == "zero") ? 1 : sizeOf(d, 2);
        if (sz < 0) sz = -sz == 13 ? 12 : -sz;
        int64_t n, fv = 0;
        if (!constExpr(p, n)) return true;
        skipWs(p);
        if (*p == ',') { p++; if (!constExpr(p, fv)) return true; }
        endCheck();
        spaceDir(n, sz, fv);
        return true;
    }
    if (d == "fill") {
        int64_t rep, sz = 1, fv = 0;
        if (!constExpr(p, rep)) return true;
        skipWs(p);
        if (*p == ',') { p++; if (!constExpr(p, sz)) return true; skipWs(p); if (*p == ',') { p++; if (!constExpr(p, fv)) return true; } }
        if (sz > 8) sz = 8;
        if (sz < 0) { error("bad .fill size"); return true; }
        if (sz) spaceDir(rep, (int)sz, fv);
        return true;
    }
    // ---- alignment
    if (d == "align" || d == "balign" || d == "p2align" || d == "balignw" || d == "balignl" || d == "p2alignw" || d == "p2alignl") {
        int64_t a = 0, fv = 0, mx = 0;
        if (*p && !constExpr(p, a)) return true;
        skipWs(p);
        if (*p == ',') { p++; skipWs(p); if (*p != ',' && !constExpr(p, fv)) return true; skipWs(p); if (*p == ',') { p++; if (!constExpr(p, mx)) return true; } }
        if (d.compare(0, 7, "p2align") == 0) { if (a < 0 || a > 30) { error("bad alignment"); return true; } a = (int64_t)1 << a; }
        doAlign(a, (int)fv, (int)mx);
        return true;
    }
    if (d == "even") { doAlign(2, 0, 0); return true; }
    if (d == "odd") { if (!(frag().data.size() & 1)) {} error(".odd is not supported"); return true; }
    if (d == "cnop") {
        int64_t off, al;
        if (!constExpr(p, off)) return true;
        skipWs(p);
        if (*p != ',') { error("cnop needs offset,alignment"); return true; }
        p++;
        if (!constExpr(p, al)) return true;
        doAlign(al, 0, 0);
        spaceDir(off, 1, 0);
        return true;
    }
    if (d == "org") {
        int64_t v;
        if (!constExpr(p, v)) return true;
        if (mot && sec().frags.size() == 1 && frag().data.empty()) return true; // origin of an empty section: linker decides
        frag().vtype = V_ORG; frag().org = v; frag().loc = cur; newFrag();
        return true;
    }
    // ---- sections
    if (d == "text" || d == "code") { switchSection(0); return true; }
    if (d == "data") { switchSection(1); return true; }
    if (d == "bss") { switchSection(2); return true; }
    if (d == "section" || d == "pushsection" || d == "sect") {
        std::string nm;
        if (*p == '"') { if (!parseString(p, nm)) return true; }
        else { const char* q = p; while (*q && *q != ',' && *q != ' ' && *q != '\t') q++; nm.assign(p, q); p = q; }
        if (nm.empty()) { error("section name expected"); return true; }
        uint32_t type = SHT_PROGBITS, flags = 0;
        bool expl = false;
        std::string group;
        skipWs(p);
        if (*p == ',') {
            p++; skipWs(p);
            if (*p == '"') {
                std::string fl;
                if (!parseString(p, fl)) return true;
                expl = true;
                for (char c : fl) switch (c) {
                    case 'a': flags |= SHF_ALLOC; break; case 'w': flags |= SHF_WRITE; break; case 'x': flags |= SHF_EXECINSTR; break;
                    case 'M': flags |= SHF_MERGE; break; case 'S': flags |= SHF_STRINGS; break; case 'G': flags |= SHF_GROUP; break;
                    case 'T': flags |= SHF_TLS; break; case 'e': flags |= SHF_EXCLUDE; break; default: break;
                }
                uint32_t dt, df;
                defaultSecAttrs(nm, dt, df);
                type = dt;
                if (type == SHT_NOBITS && !(nm.compare(0, 4, ".bss") == 0)) type = SHT_PROGBITS;
                std::vector<std::string> rest;
                splitArgs(p, rest, false);
                size_t ri = 0;
                if (ri < rest.size() && rest[ri].empty()) ri++;
                if (ri < rest.size() && !rest[ri].empty() && (rest[ri][0] == '@' || rest[ri][0] == '%' || rest[ri][0] == '#')) {
                    std::string t = lower(rest[ri].substr(1));
                    if (t == "nobits") type = SHT_NOBITS; else if (t == "progbits") type = SHT_PROGBITS; else if (t == "note") type = SHT_NOTE;
                    else if (t == "init_array") type = SHT_INIT_ARRAY; else if (t == "fini_array") type = SHT_FINI_ARRAY;
                    else if (t == "preinit_array") type = SHT_PREINIT_ARRAY;
                    ri++;
                }
                uint32_t ent = 0;
                if ((flags & SHF_MERGE) && ri < rest.size()) ent = (uint32_t)atoi(rest[ri++].c_str());
                if ((flags & SHF_GROUP) && ri < rest.size()) group = rest[ri++];
                int idx = -1;
                for (size_t i = 0; i < secs.size(); i++) if (secs[i].name == nm && secs[i].group == group) idx = (int)i;
                if (idx < 0) {
                    Section s;
                    s.name = nm; s.type = type; s.flags = flags; s.group = group; s.entsize = ent;
                    s.frags.emplace_back();
                    secs.push_back(s);
                    idx = (int)secs.size() - 1;
                }
                if (d == "pushsection") secStack.push_back({curSec, prevSec});
                switchSection(idx);
                return true;
            } else if (mot || isSymStart(*p)) {
                // Motorola: section name,code|data|bss
                const char* q = p;
                while (isSymChar(*q)) q++;
                std::string t = lower(std::string(p, q));
                expl = true;
                if (t == "code" || t == "text" || t == "code_c" || t == "code_f") flags = SHF_ALLOC | SHF_EXECINSTR;
                else if (t == "data" || t == "data_c" || t == "data_f") flags = SHF_ALLOC | SHF_WRITE;
                else if (t == "bss" || t == "bss_c" || t == "bss_f") { flags = SHF_ALLOC | SHF_WRITE; type = SHT_NOBITS; }
                else if (t == "rodata") flags = SHF_ALLOC;
                else expl = false;
            }
        }
        std::string ln = lower(nm);
        if (mot && !expl && nm[0] != '.') {
            if (ln == "text" || ln == "code") nm = ".text"; else if (ln == "data") nm = ".data"; else if (ln == "bss") nm = ".bss";
        }
        int idx = findSection(nm, true, type, flags, expl);
        if (d == "pushsection") secStack.push_back({curSec, prevSec});
        switchSection(idx);
        return true;
    }
    if (d == "popsection") {
        if (secStack.empty()) { error(".popsection without .pushsection"); return true; }
        curSec = secStack.back().first; prevSec = secStack.back().second; secStack.pop_back();
        return true;
    }
    if (d == "previous") { std::swap(curSec, prevSec); return true; }
    if (d == "subsection") return true;
    // ---- symbols
    if (d == "globl" || d == "global" || d == "xdef" || d == "public") { symList(0); return true; }
    if (d == "xref" || d == "extern") { if (mot) symList(0); return true; }
    if (d == "weak") { symList(1); return true; }
    if (d == "local") { symList(2); return true; }
    if (d == "hidden") { symList(3); return true; }
    if (d == "internal") { symList(4); return true; }
    if (d == "protected") { symList(5); return true; }
    if (d == "equ" || d == "set" || d == "equiv") {
        std::string n = label;
        if (n.empty()) {
            const char* q = p;
            while (isSymChar(*q)) q++;
            n.assign(p, q);
            p = q; skipWs(p);
            if (*p != ',') { error("expected comma after symbol name"); return true; }
            p++;
        }
        assign(n, p, d == "set", d == "equiv");
        return true;
    }
    if (d == "comm" || d == "lcomm" || d == "common") {
        const char* q = p;
        while (isSymChar(*q)) q++;
        std::string n(p, q);
        p = q; skipWs(p);
        if (n.empty() || *p != ',') { error("expected symbol,size"); return true; }
        p++;
        int64_t size, al = 0;
        if (!constExpr(p, size)) return true;
        skipWs(p);
        if (*p == ',') { p++; if (!constExpr(p, al)) return true; }
        Symbol* s = sym(n);
        if (d == "lcomm") {
            if (s->sec != SEC_UNDEF) { error("symbol '" + n + "' is already defined"); return true; }
            int save = curSec, savePrev = prevSec;
            curSec = 2;
            if (al > 1) doAlign(al, 0, 0);
            s->sec = 2; s->frag = (int)sec().frags.size() - 1; s->value = (int64_t)frag().data.size();
            s->type = STT_OBJECT; s->size = size;
            spaceDir(size, 1, 0);
            curSec = save; prevSec = savePrev;
        } else {
            if (s->sec != SEC_UNDEF && s->sec != SEC_COMMON) { error("symbol '" + n + "' is already defined"); return true; }
            s->sec = SEC_COMMON; s->global = true; s->type = STT_OBJECT;
            if (size > s->size) s->size = size;
            if (al > s->value) s->value = al;
        }
        return true;
    }
    if (d == "type") {
        const char* q = p;
        while (isSymChar(*q)) q++;
        Symbol* s = sym(std::string(p, q));
        std::string t = lower(q);
        if (t.find("function") != std::string::npos || t.find("stt_func") != std::string::npos) s->type = STT_FUNC;
        else if (t.find("object") != std::string::npos) s->type = STT_OBJECT;
        else if (t.find("notype") != std::string::npos) s->type = STT_NOTYPE;
        return true;
    }
    if (d == "size") {
        const char* q = p;
        while (isSymChar(*q)) q++;
        Symbol* s = sym(std::string(p, q));
        p = q; skipWs(p);
        if (*p != ',') { error("expected comma"); return true; }
        p++;
        s->sizeExpr = parseExpr(p);
        return true;
    }
    // ---- Motorola structure offsets
    if (d == "rsreset") { rsCounter = 0; return true; }
    if (d == "rsset") { constExpr(p, rsCounter); return true; }
    if (d == "rs" || d.compare(0, 3, "rs.") == 0) {
        int sz = sizeOf(d, 2);
        int64_t n = 0;
        if (*p && !constExpr(p, n)) return true;
        if (sz > 1 && (rsCounter & 1)) rsCounter++;
        if (!label.empty()) { Symbol* s = sym(label); s->sec = SEC_ABS; s->value = rsCounter; s->isSet = true; }
        rsCounter += n * sz;
        return true;
    }
    if (d == "equr") { if (!label.empty()) regAlias[lower(label)] = lower(trim(p)); return true; }
    // ---- files
    if (d == "include" || d == "incbin") {
        std::string fn;
        if (*p == '"' || *p == '\'') { char q = *p++; const char* e = strchr(p, q); if (!e) { error("unterminated file name"); return true; } fn.assign(p, e); p = e + 1; }
        else { const char* q = p; while (*q && *q != ',' && *q != ' ') q++; fn.assign(p, q); p = q; }
        std::vector<std::string> cand;
        cand.push_back(dirName(files[cur.file]) + fn);
        for (const std::string& dir : incDirs) cand.push_back(dir + ((dir.empty() || dir.back() == '/' || dir.back() == '\\') ? "" : "/") + fn);
        cand.push_back(fn);
        Bytes data;
        std::string found;
        for (const std::string& c : cand) if (readFile(c, data)) { found = c; break; }
        if (found.empty()) { error("cannot open '" + fn + "'"); return true; }
        if (d == "incbin") {
            int64_t skip = 0, count = -1;
            skipWs(p);
            if (*p == ',') { p++; if (!constExpr(p, skip)) return true; skipWs(p); if (*p == ',') { p++; if (!constExpr(p, count)) return true; } }
            if (skip < 0 || skip > (int64_t)data.size()) skip = (int64_t)data.size();
            if (count < 0 || skip + count > (int64_t)data.size()) count = (int64_t)data.size() - skip;
            emitBytes(data.data() + skip, (size_t)count);
            if (sec().type == SHT_NOBITS) sec().nonZero = true;
        } else {
            if (++includeDepth > 100) { error("include nesting too deep"); return true; }
            Frame f;
            f.condDepth = conds.size();
            files.push_back(found);
            loadSource(std::string(data.begin(), data.end()), (int)files.size() - 1, f.lines);
            frames.push_back(f);
        }
        return true;
    }
    if (d == "incdir") { std::string s = trim(p); if (s.size() >= 2 && (s[0] == '"' || s[0] == '\'')) s = s.substr(1, s.size() - 2); incDirs.push_back(s); return true; }
    // ---- macros and repetition
    if (d == "macro") {
        Macro m;
        if (!label.empty()) m.name = label;
        else {
            const char* q = p;
            while (isSymChar(*q)) q++;
            m.name.assign(p, q);
            p = q;
            skipWs(p);
            if (*p == ',') p++;
            std::vector<std::string> a;
            splitArgs(p, a, true);
            for (std::string s : a) {
                if (s.empty()) continue;
                Macro::Arg arg;
                size_t eq = s.find('='), col = s.find(':');
                if (eq != std::string::npos) { arg.def = unquote(trim(s.substr(eq + 1))); s = trim(s.substr(0, eq)); }
                else if (col != std::string::npos) {
                    std::string q2 = lower(trim(s.substr(col + 1)));
                    if (q2 == "req") arg.req = true; else if (q2 == "vararg") arg.vararg = true;
                    s = trim(s.substr(0, col));
                }
                arg.name = s;
                m.args.push_back(arg);
            }
        }
        if (m.name.empty()) { error("macro name expected"); return true; }
        if (!collectBody(m.body, "macro", nullptr, "endm", nullptr)) return true;
        macros[mot ? lower(m.name) : m.name] = m;
        return true;
    }
    if (d == "endm" || d == "endr") { error("." + d + " without matching start"); return true; }
    if (d == "exitm" || d == "mexit") {
        while (!frames.empty() && !frames.back().isMacro && frames.size() > 1) { while (conds.size() > frames.back().condDepth) conds.pop_back(); frames.pop_back(); }
        if (frames.size() > 1 && frames.back().isMacro) { while (conds.size() > frames.back().condDepth) conds.pop_back(); frames.pop_back(); }
        return true;
    }
    if (d == "purgem") { macros.erase(trim(p)); return true; }
    if (d == "rept" || d == "irp" || d == "irpc") {
        std::string argLine = p;
        std::vector<SrcLine> body;
        if (!collectBody(body, "rept", d == "rept" ? "irp" : "rept", "endr", nullptr)) return true;
        Frame f;
        f.condDepth = conds.size();
        if (d == "rept") {
            const char* q = argLine.c_str();
            int64_t n;
            if (!constExpr(q, n)) return true;
            if (n * (int64_t)body.size() > 4000000) { error(".rept expansion too large"); return true; }
            for (int64_t i = 0; i < n; i++) f.lines.insert(f.lines.end(), body.begin(), body.end());
        } else {
            std::vector<std::string> a;
            const char* q = argLine.c_str();
            const char* e = q;
            while (isSymChar(*e)) e++;
            std::vector<std::string> names(1, std::string(q, e));
            skipWs(e);
            if (*e == ',') e++;
            if (d == "irp") splitArgs(e, a, true);
            else { std::string s = trim(e); for (char c : s) a.push_back(std::string(1, c)); }
            if (d == "irp" && a.empty()) a.push_back("");
            for (const std::string& v : a) {
                std::vector<std::string> vals(1, unquote(v));
                for (const SrcLine& l : body) f.lines.push_back({substitute(l.text, names, vals, 0, false, ""), l.file, l.line});
            }
        }
        frames.push_back(f);
        return true;
    }
    // ---- misc
    if (d == "end") { ended = true; return true; }
    if (d == "err") { error(".err encountered"); return true; }
    if (d == "error" || d == "fail") { error(std::string(".error: ") + p); return true; }
    if (d == "abort") { error(".abort"); ended = true; return true; }
    if (d == "arch" || d == "cpu" || d == "machine") { if (!setCpuByName(lower(trim(p)))) error("unknown cpu '" + trim(p) + "'"); return true; }
    if (d.compare(0, 2, "mc") == 0 && d.size() > 4) { if (setCpuByName(d.substr(2))) return true; }
    static const char* const ignored[] = {
        "file", "loc", "line", "ident", "title", "sbttl", "list", "nolist", "eject", "psize", "ttl", "page", "idnt", "output", "opt", "version",
        "stabs", "stabn", "stabd", "altmacro", "noaltmacro", "warning", "print", "swbeg", "loc_mark_labels", "addrsig", "addrsig_sym", "fpu",
        "symver", "vtable_inherit", "vtable_entry", "ln", "desc", "attribute", "gnu_attribute", "linkonce", nullptr };
    for (int i = 0; ignored[i]; i++) if (d == ignored[i]) return true;
    if (d.compare(0, 4, "cfi_") == 0) return true;
    if (!dotted) return false;
    error("unknown directive '" + name + "'");
    return true;
}

// ------------------------------------------------------------------ statements
void Asm::statement(const std::string& s, bool col0) {
    const char* p = s.c_str();
    std::string label;
    for (;;) {
        skipWs(p);
        if (!*p) return;
        const char* q = p;
        while (isSymChar(*q)) q++;
        if (q == p) {
            if (!skipping()) error(std::string("syntax error near '") + p + "'");
            return;
        }
        std::string tok(p, q);
        const char* r = q;
        skipWs(r);
        std::string ltok = lower(tok);
        if (*r == ':' ) {
            if (!skipping()) { defineLabel(tok); label = tok; }
            p = r + 1; col0 = false;
            continue;
        }
        if (skipping()) {
            if (mot && col0 && !(ltok[0] == '.' ? isMotDirective(ltok.substr(1)) : isMotDirective(ltok)) &&
                ltok.compare(0, 2, "if") != 0 && ltok.compare(0, 3, "els") != 0 && ltok.compare(0, 3, "end") != 0) { p = q; col0 = false; continue; }
            conditional(ltok, r);
            return;
        }
        if (*r == '=' && !(mot && r[1] == '=')) {
            assign(tok, r + (r[1] == '=' ? 2 : 1), true, false);
            return;
        }
        if (mot && col0) {
            // "label opcode ..." without a colon
            const char* t = r;
            while (isSymChar(*t)) t++;
            std::string next = lower(std::string(r, t));
            bool isOp = (ltok[0] == '.' ? isMotDirective(ltok.substr(1)) : isMotDirective(ltok)) || macros.count(ltok) || isMnemonic(ltok) ||
                        ltok.compare(0, 2, "if") == 0 || ltok == "else" || ltok == "endc" || ltok == "endif" || ltok == "elseif";
            if (!isOp || next == "equ" || next == "set" || next == "macro" || next == "equr" || next == "=" ) {
                std::string un = undot(next);
                if (un == "equ" || un == "set" || un == "macro" || un == "equr" || un == "rs" || un.compare(0, 3, "rs.") == 0) {
                    const char* a = t;
                    directive(next, a, tok);
                    return;
                }
                defineLabel(tok);
                label = tok;
                p = q; col0 = false;
                continue;
            }
        }
        if (mot && !label.empty()) {
            std::string un = undot(ltok);
            if (un == "equ" || un == "set" || un == "macro" || un == "equr" || un == "rs" || un.compare(0, 3, "rs.") == 0) {
                // the label was defined as an address just before: undo and treat as the directive's name
                Symbol* ls = findSym(label);
                if (ls) { ls->sec = SEC_UNDEF; ls->value = 0; }
                directive(ltok, r, label);
                return;
            }
        }
        if (conditional(ltok, r)) return;
        // macro call?
        {
            std::string mname = mot ? ltok : tok;
            std::string size0;
            auto it = macros.find(mname);
            if (it == macros.end() && mot) {
                size_t dot = mname.rfind('.');
                if (dot != std::string::npos && dot > 0) { it = macros.find(mname.substr(0, dot)); size0 = mname.substr(dot + 1); }
            }
            if (it != macros.end()) { Macro m = it->second; expandMacro(m, r, size0); return; }
        }
        if (directive(ltok, r, std::string())) return;
        if (ltok[0] == '.' && !isMnemonic(ltok)) { error("unknown directive '" + tok + "'"); return; }
        // instruction
        assembleInsn(ltok, r);
        return;
    }
}

void Asm::processLine(const SrcLine& l) {
    cur.file = l.file; cur.line = l.line;
    const std::string& t = l.text;
    bool col0 = !t.empty() && t[0] != ' ' && t[0] != '\t';
    if (mot || t.find(';') == std::string::npos) { statement(t, col0); return; }
    size_t start = 0;
    size_t depthFrames = frames.size();
    for (size_t i = 0; i <= t.size(); i++) {
        char c = i < t.size() ? t[i] : ';';
        if (c == '"') { for (i++; i < t.size() && t[i] != '"'; i++) if (t[i] == '\\') i++; continue; }
        if (c == '\'') { if (i + 1 < t.size()) { i++; if (t[i] == '\\') i++; } continue; }
        if (c == ';') {
            statement(t.substr(start, i - start), col0 && start == 0);
            start = i + 1;
            if (ended || frames.size() != depthFrames) {
                // a macro/include started in the middle of the line: run the rest afterwards is not supported
                if (start < t.size() && !trim(t.substr(start)).empty() && frames.size() > depthFrames) {
                    Frame& fr = frames[depthFrames - 1];
                    fr.lines.insert(fr.lines.begin() + (long)fr.pos, SrcLine{t.substr(start), l.file, l.line});
                }
                return;
            }
        }
    }
}

// ------------------------------------------------------------------ layout and relaxation
static int relaxSize(int rtype, int state) {
    switch (rtype) {
    case RX_BRANCHBW: return state == 0 ? 0 : 2;
    case RX_BRANCHBWL: case RX_BRABSJUNC: return state == 0 ? 0 : state == 1 ? 2 : 4;
    case RX_BRABSJCOND: return state == 0 ? 0 : state == 1 ? 2 : 6;
    case RX_FBRANCH: case RX_ABSTOPCREL: return state <= 1 ? 2 : 4;
    case RX_PCREL1632: return state <= 1 ? 2 : 6;
    case RX_PCINDEX: return state == 0 ? 0 : 4;
    case RX_DBCCLBR: case RX_DBCCABSJ: return state <= 1 ? 2 : 10;
    }
    return 0;
}
static int relaxMaxState(int rtype) { return rtype == RX_BRANCHBW ? 1 : 2; }
static int relaxUndefState(int rtype) { return (rtype == RX_BRANCHBW || rtype == RX_FBRANCH) ? 1 : 2; }
static int relaxMinState(int rtype) {
    switch (rtype) {
    case RX_FBRANCH: case RX_ABSTOPCREL: case RX_PCREL1632: case RX_DBCCLBR: case RX_DBCCABSJ: return 1;
    }
    return 0;
}

// Address of a relaxable target: a non-global label of this section (+ constant).
bool Asm::targetAddr(const Expr* e, int secIdx, int64_t& addr) {
    Val v = eval(e, false);
    if (v.bad) return false;
    // reduce with final addresses (all labels of a section are comparable during layout)
    reduce(v, true);
    if (v.t.size() != 1 || v.t[0].coef != 1) return false;
    Symbol* s = v.t[0].s;
    if (s->sec != secIdx || s->global || s->weak) return false;
    addr = (int64_t)symAddr(s) + v.c;
    return true;
}

void Asm::layout() {
    for (size_t si = 0; si < secs.size(); si++) {
        Section& s = secs[si];
        for (Frag& f : s.frags) if (f.vtype == V_RELAX) { if (f.state < relaxMinState(f.rtype)) f.state = relaxMinState(f.rtype); }
        bool changed = true;
        int iter = 0;
        while (changed && iter++ < 1000) {
            changed = false;
            uint32_t addr = 0;
            for (Frag& f : s.frags) {
                f.addr = addr;
                addr += (uint32_t)f.data.size();
                uint32_t vs = 0;
                if (f.vtype == V_ALIGN) {
                    vs = (uint32_t)((f.align - (addr % (uint32_t)f.align)) % (uint32_t)f.align);
                    if (f.maxSkip && vs > (uint32_t)f.maxSkip) vs = 0;
                } else if (f.vtype == V_ORG) {
                    if (f.org < (int64_t)addr) { if (iter > 1) {} vs = 0; }
                    else vs = (uint32_t)(f.org - addr);
                } else if (f.vtype == V_RELAX) vs = (uint32_t)relaxSize(f.rtype, f.state);
                f.vsize = vs;
                addr += vs;
            }
            s.size = addr;
            for (Frag& f : s.frags) {
                if (f.vtype != V_RELAX) continue;
                int64_t t;
                int st = f.state;
                if (!targetAddr(f.target, (int)si, t)) st = std::max(st, relaxUndefState(f.rtype));
                else {
                    int64_t dist = t - ((int64_t)f.addr + (int64_t)f.data.size());
                    int mx = relaxMaxState(f.rtype);
                    if (st == 0) {
                        bool ok = f.rtype == RX_PCINDEX ? (dist >= -130 && dist <= 125) : (dist >= -128 && dist <= 127 && dist != 0);
                        if (!ok) st = f.rtype == RX_PCINDEX ? 2 : 1;
                    }
                    if (st == 1 && mx > 1 && (dist < -32768 || dist > 32767)) st = 2;
                }
                if (st != f.state) { f.state = st; changed = true; }
            }
        }
        for (Frag& f : s.frags) if (f.vtype == V_ORG && f.org < (int64_t)(f.addr + f.data.size())) errorAt(f.loc, "attempt to move .org backwards");
    }
}

void Asm::convert() {
    for (Section& s : secs) {
        for (Frag& f : s.frags) {
            if (f.vtype == V_ALIGN || f.vtype == V_ORG) {
                f.data.insert(f.data.end(), f.vsize, (unsigned char)(f.vtype == V_ALIGN ? f.fill : 0));
                if (f.vtype == V_ALIGN && f.fill && s.type == SHT_NOBITS) s.nonZero = true;
                continue;
            }
            if (f.vtype != V_RELAX) continue;
            Bytes& d = f.data;
            unsigned char* op = &d[(size_t)f.opOff];
            auto fix = [&](int size, bool pcrel, int pcadj, uint32_t off) {
                Fixup x;
                x.off = off; x.size = size; x.pcrel = pcrel; x.pcadj = pcadj; x.e = f.target; x.loc = f.loc; x.sgn = pcrel;
                f.fix.push_back(x);
            };
            uint32_t at = (uint32_t)d.size();
            int st = f.state;
            switch (f.rtype) {
            case RX_BRANCHBW: case RX_BRANCHBWL: case RX_BRABSJUNC: case RX_BRABSJCOND:
                if (st == 0) fix(1, true, -1, (uint32_t)f.opOff + 1);
                else if (st == 1) { op[1] = 0; put16(d, 0); fix(2, true, 0, at); }
                else if (f.rtype == RX_BRANCHBWL) { op[1] = 0xFF; put32(d, 0); fix(4, true, 0, at); }
                else if (f.rtype == RX_BRABSJUNC) {
                    bool bsr = op[0] == 0x61;
                    op[0] = 0x4E; op[1] = bsr ? 0xB9 : 0xF9;
                    put32(d, 0); fix(4, false, 0, at);
                } else {
                    op[0] ^= 0x01; op[1] = 0x06;
                    put16(d, 0x4EF9); put32(d, 0); fix(4, false, 0, at + 2);
                }
                break;
            case RX_FBRANCH:
                if (st <= 1) { put16(d, 0); fix(2, true, 0, at); }
                else { op[1] |= 0x40; put32(d, 0); fix(4, true, 0, at); }
                break;
            case RX_PCREL1632:
                if (st <= 1) { op[1] = (unsigned char)((op[1] & ~0x3F) | 0x3A); put16(d, 0); fix(2, true, 0, at); }
                else { op[1] = (unsigned char)((op[1] & ~0x3F) | 0x3B); put16(d, 0x0170); put32(d, 0); fix(4, true, 2, at + 2); }
                break;
            case RX_PCINDEX:
                if (st == 0) fix(1, true, 1, at - 1);
                else { d[at - 2] |= 0x01; d[at - 1] = 0x30; put32(d, 0); fix(4, true, 2, at); }
                break;
            case RX_ABSTOPCREL:
                if (st <= 1) { op[1] = (unsigned char)((op[1] & ~0x3F) | 0x3A); put16(d, 0); fix(2, true, 0, at); }
                else { op[1] = (unsigned char)((op[1] & ~0x3F) | 0x39); put32(d, 0); f.fix.push_back(Fixup()); Fixup& x = f.fix.back(); x.off = at; x.size = 4; x.e = f.target; x.loc = f.loc; }
                break;
            case RX_DBCCLBR: case RX_DBCCABSJ:
                if (st <= 1) { put16(d, 0); fix(2, true, 0, at); }
                else {
                    put16(d, 0x0004); put16(d, 0x6006);
                    if (f.rtype == RX_DBCCLBR) { put16(d, 0x60FF); put32(d, 0); fix(4, true, 0, at + 6); }
                    else { put16(d, 0x4EF9); put32(d, 0); f.fix.push_back(Fixup()); Fixup& x = f.fix.back(); x.off = at + 6; x.size = 4; x.e = f.target; x.loc = f.loc; }
                }
                break;
            }
        }
    }
}

void Asm::applyFixups() {
    for (size_t si = 0; si < secs.size(); si++) {
        Section& s = secs[si];
        s.out.clear();
        s.out.reserve(s.size);
        for (Frag& f : s.frags) {
            uint32_t base = (uint32_t)s.out.size();
            s.out.insert(s.out.end(), f.data.begin(), f.data.end());
            for (const Fixup& x : f.fix) {
                Val v = eval(x.e, true);
                reduce(v, true);
                uint32_t fa = base + x.off;
                bool pcrel = x.pcrel;
                int64_t addend = v.c;
                Symbol* target = nullptr;
                if (v.bad) { errorAt(x.loc, "invalid expression"); continue; }
                if (v.t.size() == 2 && !pcrel) {
                    // sym - label_in_this_section  ->  pc-relative
                    int pi = v.t[0].coef == 1 ? 0 : 1, ni = 1 - pi;
                    if (v.t[pi].coef == 1 && v.t[ni].coef == -1 && v.t[ni].s->sec == (int)si) {
                        addend += (int64_t)fa - (int64_t)symAddr(v.t[ni].s);
                        target = v.t[pi].s;
                        pcrel = true;
                    } else { errorAt(x.loc, "expression cannot be relocated (difference of symbols in different sections)"); continue; }
                } else if (v.t.size() == 1 && v.t[0].coef == 1) target = v.t[0].s;
                else if (!v.t.empty()) { errorAt(x.loc, "expression cannot be relocated"); continue; }
                else if (pcrel) {
                    // pc-relative reference to an absolute address: only GNU-compatible forms
                    if (x.size == 1 || (x.size == 2 && (addend < -32768 || addend > 65535))) { errorAt(x.loc, "pc-relative reference to an absolute address is out of range"); continue; }
                    addend += x.pcadj;
                }
                if (target && x.pcrel) addend += x.pcadj;

                bool resolved = false;
                int64_t value = 0;
                if (!target && !pcrel) { resolved = true; value = addend; }
                else if (target && pcrel && target->sec == (int)si && !target->global && !target->weak) {
                    resolved = true;
                    value = (int64_t)symAddr(target) + addend - (int64_t)fa;
                }
                if (resolved) {
                    bool ok = true;
                    if (pcrel && x.size == 1 && (value == 0 || value == -1)) { errorAt(x.loc, "invalid byte branch offset"); continue; }
                    if (x.size == 1) ok = (pcrel || x.sgn) ? (value >= -128 && value <= 127) : (value >= -128 && value <= 255);
                    else if (x.size == 2) ok = (pcrel || x.sgn) ? (value >= -32768 && value <= 32767) : (value >= -32768 && value <= 65535);
                    if (!ok) { errorAt(x.loc, "value " + std::to_string(value) + " out of range for a " + std::to_string(x.size * 8) + "-bit field"); continue; }
                    for (int i = 0; i < x.size; i++) s.out[fa + (uint32_t)i] = (unsigned char)((uint64_t)value >> (8 * (x.size - 1 - i)));
                    if (value && s.type == SHT_NOBITS) s.nonZero = true;
                    continue;
                }
                Rela r;
                r.off = fa;
                r.type = pcrel ? (x.size == 4 ? R_68K_PC32 : x.size == 2 ? R_68K_PC16 : R_68K_PC8) : (x.size == 4 ? R_68K_32 : x.size == 2 ? R_68K_16 : R_68K_8);
                r.sym = nullptr; r.secSym = -1;
                if (!target) { r.addend = addend; } // against *ABS*
                else if (target->sec >= 0 && !target->global && !target->weak) { r.secSym = target->sec; r.addend = addend + (int64_t)symAddr(target); }
                else { r.sym = target; r.addend = addend; }
                s.rel.push_back(r);
            }
        }
        if (s.type == SHT_NOBITS && (s.nonZero || !s.rel.empty())) {
            bool nz = !s.rel.empty();
            for (unsigned char c : s.out) if (c) { nz = true; break; }
            if (nz) errorAt(cur, "non-zero data in no-bits section '" + s.name + "'");
        }
    }
}

// ------------------------------------------------------------------ ELF writer
void Asm::writeElf(Bytes& elf) {
    struct Sh { std::string name; uint32_t type, flags, link, info, align, entsize; Bytes data; uint32_t size; };
    std::vector<Sh> sh(1);
    sh[0] = Sh{"", 0, 0, 0, 0, 0, 0, Bytes(), 0};
    // groups first
    std::vector<std::string> groups;
    for (Section& s : secs) if (!s.group.empty() && std::find(groups.begin(), groups.end(), s.group) == groups.end()) groups.push_back(s.group);
    size_t firstContent = 1 + groups.size();
    for (size_t i = 0; i < secs.size(); i++) { secs[i].elfIndex = (int)(firstContent + i); secs[i].symIndex = (int)(1 + i); }
    // final symbol values for .set symbols bound to labels
    struct ESym { std::string name; uint32_t value, size; int info, other; uint32_t shndx; };
    std::vector<ESym> es(1);
    es[0] = ESym{"", 0, 0, 0, 0, 0};
    for (Section& s : secs) es.push_back(ESym{"", 0, 0, STT_SECTION, 0, (uint32_t)s.elfIndex});
    auto resolve = [&](Symbol* s, ESym& e) -> bool {
        e.name = s->name; e.other = s->vis; e.size = (uint32_t)s->size; e.value = 0; e.shndx = SHN_UNDEF;
        int bind = s->weak ? STB_WEAK : (s->global || s->sec == SEC_UNDEF || s->sec == SEC_COMMON) ? STB_GLOBAL : STB_LOCAL;
        if (s->sizeExpr) { int64_t v; Val vv = eval(s->sizeExpr, true); reduce(vv, true); v = vv.c; if (vv.t.empty() && !vv.bad) e.size = (uint32_t)v; }
        if (s->sec >= 0) { e.shndx = (uint32_t)secs[s->sec].elfIndex; e.value = symAddr(s); }
        else if (s->sec == SEC_ABS) { e.shndx = SHN_ABS; e.value = (uint32_t)s->value; }
        else if (s->sec == SEC_COMMON) { e.shndx = SHN_COMMON; e.value = (uint32_t)(s->value ? s->value : (s->size >= 4 ? 4 : s->size >= 2 ? 2 : 1)); }
        else if (s->sec == SEC_EXPR) {
            Val v = eval(s->expr, true);
            reduce(v, true);
            if (v.bad || v.t.size() > 1 || (v.t.size() == 1 && (v.t[0].coef != 1 || v.t[0].s->sec < 0))) return false;
            if (v.t.empty()) { e.shndx = SHN_ABS; e.value = (uint32_t)v.c; }
            else { e.shndx = (uint32_t)secs[v.t[0].s->sec].elfIndex; e.value = (uint32_t)(symAddr(v.t[0].s) + v.c); }
        }
        e.info = (bind << 4) | (s->type & 15);
        return true;
    };
    // locals
    for (Symbol* s : symOrder) {
        if (s->temp || s->global || s->weak || s->sec == SEC_UNDEF || s->sec == SEC_COMMON) continue;
        if (s->sec == SEC_ABS || s->sec == SEC_EXPR) continue;
        if (s->name.compare(0, 2, ".L") == 0 || s->name.empty()) continue;
        ESym e;
        if (!resolve(s, e)) continue;
        s->elfIndex = (int)es.size();
        es.push_back(e);
    }
    // group signature symbols that are not otherwise defined
    std::vector<int> groupSym(groups.size(), 0);
    for (size_t g = 0; g < groups.size(); g++) {
        Symbol* s = findSym(groups[g]);
        if (s && (s->global || s->weak || s->elfIndex >= 0)) continue;
        int member = 0;
        for (Section& x : secs) if (x.group == groups[g]) { member = x.elfIndex; break; }
        groupSym[g] = (int)es.size();
        es.push_back(ESym{groups[g], 0, 0, STT_NOTYPE, 0, (uint32_t)member});
    }
    uint32_t firstGlobal = (uint32_t)es.size();
    for (Symbol* s : symOrder) {
        if (s->temp || s->elfIndex >= 0) continue;
        bool want = s->global || s->weak || s->sec == SEC_COMMON || (s->sec == SEC_UNDEF && s->used);
        if (!want) continue;
        if (s->sec == SEC_UNDEF && !s->used && !s->global && !s->weak) continue;
        ESym e;
        if (!resolve(s, e)) { errorAt(cur, "cannot resolve value of symbol '" + s->name + "'"); continue; }
        s->elfIndex = (int)es.size();
        es.push_back(e);
    }
    for (size_t g = 0; g < groups.size(); g++) if (!groupSym[g]) { Symbol* s = findSym(groups[g]); groupSym[g] = s ? s->elfIndex : 0; }

    // section headers: groups, content, rela, symtab, strtab, shstrtab
    size_t nContent = secs.size();
    std::vector<int> relaIndex(nContent, 0);
    size_t idx = firstContent + nContent;
    for (size_t i = 0; i < nContent; i++) if (!secs[i].rel.empty()) relaIndex[i] = (int)idx++;
    uint32_t symtabIdx = (uint32_t)idx, strtabIdx = (uint32_t)idx + 1, shstrIdx = (uint32_t)idx + 2;
    for (size_t g = 0; g < groups.size(); g++) {
        Sh h{".group", SHT_GROUP, 0, symtabIdx, (uint32_t)groupSym[g], 4, 4, Bytes(), 0};
        put32(h.data, 1); // GRP_COMDAT
        for (Section& x : secs) if (x.group == groups[g]) put32(h.data, (uint32_t)x.elfIndex);
        for (size_t i = 0; i < nContent; i++) if (secs[i].group == groups[g] && relaIndex[i]) put32(h.data, (uint32_t)relaIndex[i]);
        h.size = (uint32_t)h.data.size();
        sh.push_back(h);
    }
    for (Section& s : secs) {
        Sh h{s.name, s.type, s.flags, 0, 0, s.align, s.entsize, Bytes(), s.size};
        if (s.type != SHT_NOBITS) h.data = s.out;
        sh.push_back(h);
    }
    for (size_t i = 0; i < nContent; i++) {
        if (!relaIndex[i]) continue;
        Sh h{".rela" + secs[i].name, SHT_RELA, SHF_INFO_LINK | (secs[i].group.empty() ? 0u : (uint32_t)SHF_GROUP), symtabIdx, (uint32_t)secs[i].elfIndex, 4, 12, Bytes(), 0};
        for (const Rela& r : secs[i].rel) {
            uint32_t si = r.sym ? (uint32_t)r.sym->elfIndex : r.secSym >= 0 ? (uint32_t)secs[r.secSym].symIndex : 0;
            put32(h.data, r.off);
            put32(h.data, (si << 8) | (uint32_t)r.type);
            put32(h.data, (uint32_t)r.addend);
        }
        h.size = (uint32_t)h.data.size();
        sh.push_back(h);
    }
    Bytes strtab(1, 0);
    {
        Sh h{".symtab", SHT_SYMTAB, 0, strtabIdx, firstGlobal, 4, 16, Bytes(), 0};
        for (const ESym& e : es) {
            uint32_t no = 0;
            if (!e.name.empty()) { no = (uint32_t)strtab.size(); strtab.insert(strtab.end(), e.name.begin(), e.name.end()); strtab.push_back(0); }
            put32(h.data, no); put32(h.data, e.value); put32(h.data, e.size);
            put8(h.data, (uint32_t)e.info); put8(h.data, (uint32_t)e.other); put16(h.data, e.shndx);
        }
        h.size = (uint32_t)h.data.size();
        sh.push_back(h);
    }
    sh.push_back(Sh{".strtab", SHT_STRTAB, 0, 0, 0, 1, 0, strtab, (uint32_t)strtab.size()});
    Bytes shstr(1, 0);
    std::vector<uint32_t> nameOff(sh.size() + 1, 0);
    sh.push_back(Sh{".shstrtab", SHT_STRTAB, 0, 0, 0, 1, 0, Bytes(), 0});
    for (size_t i = 1; i < sh.size(); i++) { nameOff[i] = (uint32_t)shstr.size(); shstr.insert(shstr.end(), sh[i].name.begin(), sh[i].name.end()); shstr.push_back(0); }
    sh.back().data = shstr; sh.back().size = (uint32_t)shstr.size();

    elf.assign(52, 0);
    static const unsigned char ident[8] = {0x7F, 'E', 'L', 'F', 1, 2, 1, 0};
    memcpy(elf.data(), ident, 8);
    std::vector<uint32_t> offs(sh.size(), 0);
    for (size_t i = 1; i < sh.size(); i++) {
        uint32_t al = sh[i].align ? sh[i].align : 1;
        if (al > 16) al = 16;
        while (elf.size() % al) elf.push_back(0);
        offs[i] = (uint32_t)elf.size();
        elf.insert(elf.end(), sh[i].data.begin(), sh[i].data.end());
    }
    while (elf.size() % 4) elf.push_back(0);
    uint32_t shoff = (uint32_t)elf.size();
    for (size_t i = 0; i < sh.size(); i++) {
        put32(elf, nameOff[i]); put32(elf, sh[i].type); put32(elf, sh[i].flags); put32(elf, 0);
        put32(elf, offs[i]); put32(elf, sh[i].size); put32(elf, sh[i].link); put32(elf, sh[i].info);
        put32(elf, sh[i].align); put32(elf, sh[i].entsize);
    }
    wr16(&elf[16], ET_REL); wr16(&elf[18], EM_68K); wr32(&elf[20], 1);
    wr32(&elf[32], shoff);
    wr32(&elf[36], (cpu & 1) ? 0x01000000u : 0); // EF_M68K_M68000 like GNU as
    wr16(&elf[40], 52); wr16(&elf[46], 40); wr16(&elf[48], (uint32_t)sh.size()); wr16(&elf[50], shstrIdx);
}

// ------------------------------------------------------------------ driver
bool Asm::run(const std::string& src, const std::string& fileName, Bytes& elf) {
    files.push_back(fileName);
    setCpu(opt.cpu, opt.fpu, opt.mmu);
    findSection(".text", true, 0, 0, false);
    findSection(".data", true, 0, 0, false);
    findSection(".bss", true, 0, 0, false);
    curSec = prevSec = 0;
    for (const auto& d : opt.defsyms) {
        const Expr* e = parseExprStr(d.second);
        int64_t v = 0;
        if (!e || !evalNow(e, v)) { error("bad --defsym value for " + d.first); continue; }
        Symbol* s = sym(d.first);
        s->sec = SEC_ABS; s->value = v; s->isSet = true;
    }
    Frame f;
    loadSource(src, 0, f.lines);
    frames.push_back(f);
    while (!frames.empty() && !ended) {
        Frame& fr = frames.back();
        if (fr.pos >= fr.lines.size()) {
            if (conds.size() > fr.condDepth && frames.size() > 1) {
                // conditionals opened inside a macro/include must close there
                while (conds.size() > fr.condDepth) conds.pop_back();
            }
            frames.pop_back();
            continue;
        }
        SrcLine l = fr.lines[fr.pos++];
        processLine(l);
    }
    if (!conds.empty() && !ended) error("unterminated conditional (.if without .endif)");
    for (Symbol* s : symOrder)
        if (s->temp && s->sec == SEC_UNDEF && !s->name.empty() && s->name[0] == '')
            error("local label '" + s->name.substr(1, s->name.find('') - 1) + "' (forward reference) is not defined");
    layout();
    convert();
    applyFixups();
    if (errors) return false;
    writeElf(elf);
    return errors == 0;
}

bool assemble(const std::string& source, const std::string& fileName, const AsOptions& opts,
              std::vector<unsigned char>& elf, std::string& diag) {
    Asm a(opts, diag);
    return a.run(source, fileName, elf);
}

} // namespace asmblr

// public entry point in namespace p68, forwarding into the assembler implementation
bool assemble(const std::string& source, const std::string& fileName, const AsOptions& opts,
              std::vector<unsigned char>& elf, std::string& diag) {
    return asmblr::assemble(source, fileName, opts, elf, diag);
}
} // namespace p68
