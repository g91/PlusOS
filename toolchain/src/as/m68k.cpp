// P68 assembler - Motorola 680x0 / CPU32 / 6888x back end: operand parser, opcode
// templates and instruction encoder. Encoding choices (shortest forms, relaxation
// classes, default sizes) deliberately mirror GNU as so objects are byte-compatible.
#include "as_internal.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace p68 { namespace asmblr {

// ---------------------------------------------------------------- CPU features
enum : unsigned {
    C000 = 1, C010 = 2, C020 = 4, C030 = 8, C040 = 0x10, C060 = 0x20, C32 = 0x40,
    FPU = 0x80, MMU = 0x100, F040 = 0x200,
    ALL = 0x7F, U10 = 0x7E, U20 = 0x3C, U20C = 0x7C, U40 = 0x30
};

void Asm::setCpu(int n, bool fpu, bool mmu) {
    unsigned keep = cpu & (FPU | MMU);
    switch (n) {
    case 68000: case 68008: case 68302: case 68306: case 68307: case 68322: case 68356: cpu = C000; break;
    case 68010: cpu = C010; break;
    case 68020: cpu = C020; break;
    case 68030: cpu = C030; break;
    case 68040: cpu = C040 | FPU | F040; break;
    case 68060: cpu = C060 | FPU | F040; break;
    case 68330: case 68331: case 68332: case 68333: case 68334: case 68336: case 68340: case 68341: case 68349: case 68360: case 32: cpu = C32; break;
    default: cpu = C000; break;
    }
    cpu |= keep;
    if (fpu) cpu |= FPU;
    if (mmu) cpu |= MMU;
    if (opt.noFpu) cpu &= ~(unsigned)(FPU | F040);
}
bool Asm::setCpuByName(const std::string& n0) {
    std::string n = n0;
    if (!n.empty() && n[0] == 'm') n = n.substr(1);
    if (n.compare(0, 2, "c6") == 0) n = n.substr(1);
    if (n == "cpu32") { setCpu(68332, false, false); return true; }
    if (n == "68881" || n == "68882") { cpu |= FPU; return true; }
    if (n == "68851") { cpu |= MMU; return true; }
    int v = atoi(n.c_str());
    if (v == 68000 || v == 68008 || v == 68010 || v == 68020 || v == 68030 || v == 68040 || v == 68060 || (v >= 68302 && v <= 68360)) {
        setCpu(v, false, false);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- registers
enum Reg {
    D0 = 0, A0 = 8, PC = 16, ZPC = 17, ZD0 = 18, ZA0 = 26, FP0 = 34,
    CCR = 42, SR, USP, FPCR, FPSR, FPIAR, SFC, DFC, CACR, VBR, CAAR, MSP, ISP, TC, ITT0, ITT1, DTT0, DTT1,
    MMUSR, URP, SRP, CRP, BUSCR, PCR, TT0, TT1, NC, DC, IC, BC, PSR
};
static const struct { const char* n; int r; } regNames[] = {
    {"d0", 0}, {"d1", 1}, {"d2", 2}, {"d3", 3}, {"d4", 4}, {"d5", 5}, {"d6", 6}, {"d7", 7},
    {"a0", 8}, {"a1", 9}, {"a2", 10}, {"a3", 11}, {"a4", 12}, {"a5", 13}, {"a6", 14}, {"a7", 15}, {"sp", 15}, {"fp", 14},
    {"pc", PC}, {"zpc", ZPC},
    {"zd0", ZD0}, {"zd1", ZD0 + 1}, {"zd2", ZD0 + 2}, {"zd3", ZD0 + 3}, {"zd4", ZD0 + 4}, {"zd5", ZD0 + 5}, {"zd6", ZD0 + 6}, {"zd7", ZD0 + 7},
    {"za0", ZA0}, {"za1", ZA0 + 1}, {"za2", ZA0 + 2}, {"za3", ZA0 + 3}, {"za4", ZA0 + 4}, {"za5", ZA0 + 5}, {"za6", ZA0 + 6}, {"za7", ZA0 + 7},
    {"zsp", ZA0 + 7}, {"zfp", ZA0 + 6},
    {"fp0", FP0}, {"fp1", FP0 + 1}, {"fp2", FP0 + 2}, {"fp3", FP0 + 3}, {"fp4", FP0 + 4}, {"fp5", FP0 + 5}, {"fp6", FP0 + 6}, {"fp7", FP0 + 7},
    {"ccr", CCR}, {"cc", CCR}, {"sr", SR}, {"usp", USP}, {"fpcr", FPCR}, {"fpc", FPCR}, {"fpsr", FPSR}, {"fps", FPSR},
    {"fpiar", FPIAR}, {"fpi", FPIAR}, {"sfc", SFC}, {"sfcr", SFC}, {"dfc", DFC}, {"dfcr", DFC}, {"cacr", CACR}, {"vbr", VBR},
    {"caar", CAAR}, {"msp", MSP}, {"isp", ISP}, {"tc", TC}, {"itt0", ITT0}, {"itt1", ITT1}, {"dtt0", DTT0}, {"dtt1", DTT1},
    {"iacr0", ITT0}, {"iacr1", ITT1}, {"dacr0", DTT0}, {"dacr1", DTT1}, {"mmusr", MMUSR}, {"psr", PSR}, {"urp", URP},
    {"srp", SRP}, {"crp", CRP}, {"buscr", BUSCR}, {"pcr", PCR}, {"tt0", TT0}, {"tt1", TT1}, {"nc", NC}, {"dc", DC}, {"ic", IC}, {"bc", BC},
    {nullptr, 0}};
static int lookupReg(const std::string& n) {
    for (int i = 0; regNames[i].n; i++) if (n == regNames[i].n) return regNames[i].r;
    return -1;
}
bool Asm::isRegName(const std::string& s) { return lookupReg(lower(s)) >= 0 || regAlias.count(lower(s)); }

// ---------------------------------------------------------------- operands
enum Mode { M_NONE, DREG, AREG, AINDR, AINC, ADEC, DISP, BASE, POST, PRE, ABSL, IMMED, FPREG, CONTROL, REGLST };

struct Operand {
    int mode = M_NONE;
    int reg = -1;
    const Expr* disp = nullptr; int dsize = 0; bool dconst = true; int64_t dval = 0;
    int ireg = -1, isize = 0, iscale = 1;
    const Expr* odisp = nullptr; int osize = 0; bool oconst = true; int64_t oval = 0;
    uint32_t mask = 0;
    std::string text;
};

namespace {

struct Parser {
    Asm& as;
    explicit Parser(Asm& a) : as(a) {}

    // Register at p; advances p. Returns -1 when there is none.
    int reg(const char*& p) {
        const char* q = p;
        bool pct = *q == '%';
        if (pct) q++;
        else if (!as.regOpt) return -1;
        const char* e = q;
        while ((*e >= 'a' && *e <= 'z') || (*e >= 'A' && *e <= 'Z') || (*e >= '0' && *e <= '9')) e++;
        std::string n = lower(std::string(q, e));
        int r = lookupReg(n);
        if (r < 0) {
            auto it = as.regAlias.find(n);
            if (it != as.regAlias.end()) r = lookupReg(it->second);
        }
        if (r < 0) return -1;
        if (!pct && isSymChar(*e)) {
            // "d0.w" is fine, "d0foo" / "a0.field" are symbols
            if (!(*e == '.' && strchr("bwlBWL", e[1]) && e[1] && !isSymChar(e[2]))) return -1;
        }
        p = e;
        return r;
    }
    static int sizeChar(char c) {
        switch (c) { case 'b': case 'B': case 's': case 'S': return 1; case 'w': case 'W': return 2; case 'l': case 'L': return 3; }
        return 0;
    }
    // trailing ".w" / ":l" on an expression string
    static int stripSize(std::string& t) {
        size_t n = t.size();
        if (n > 2 && (t[n - 2] == '.' || t[n - 2] == ':')) {
            int s = sizeChar(t[n - 1]);
            if (s) { t.erase(n - 2); return s; }
        }
        return 0;
    }
    static std::string trim(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && (s[a] == ' ' || s[a] == '\t')) a++;
        while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
        return s.substr(a, b - a);
    }
    static void splitTop(const std::string& s, char sep, std::vector<std::string>& out) {
        int depth = 0;
        std::string cur;
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];
            if (c == '"' ) { cur += c; for (i++; i < s.size(); i++) { cur += s[i]; if (s[i] == '\\' && i + 1 < s.size()) cur += s[++i]; else if (s[i] == '"') break; } continue; }
            if (c == '\'') { cur += c; if (i + 1 < s.size()) { cur += s[++i]; if (s[i] == '\\' && i + 1 < s.size()) cur += s[++i]; if (i + 1 < s.size() && s[i + 1] == '\'') cur += s[++i]; } continue; }
            if (c == '(' || c == '[' || c == '{') depth++;
            else if (c == ')' || c == ']' || c == '}') depth--;
            if (c == sep && depth == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
            cur += c;
        }
        out.push_back(trim(cur));
    }
    bool isRegOnly(const std::string& s) {
        const char* p = s.c_str();
        if (reg(p) < 0) return false;
        skipWs(p);
        return *p == 0;
    }
    bool isRegOrInd(const std::string& s0) {
        std::string s = trim(s0);
        if (isRegOnly(s)) return true;
        if (s.size() > 2 && s[0] == '(' && s.back() == ')') return isRegOnly(trim(s.substr(1, s.size() - 2)));
        return false;
    }

    struct Item { int kind = 0; int r = -1, size = 0, scale = 1; bool sized = false; const Expr* e = nullptr; int esize = 0; std::string br; };
    // kind: 1 register, 2 expression, 3 bracket group
    bool item(const std::string& t0, Item& it) {
        std::string t = trim(t0);
        if (t.empty()) { as.error("empty operand component"); return false; }
        if (t[0] == '[' && t.back() == ']') { it.kind = 3; it.br = t.substr(1, t.size() - 2); return true; }
        const char* p = t.c_str();
        int r = reg(p);
        if (r >= 0) {
            it.kind = 1; it.r = r;
            if ((*p == '.' || *p == ':') && sizeChar(p[1]) && !isSymChar(p[2])) { it.size = sizeChar(p[1]); it.sized = true; p += 2; }
            skipWs(p);
            if (*p == '*' || *p == ':') {
                p++; skipWs(p);
                int sc = *p - '0';
                if (sc != 1 && sc != 2 && sc != 4 && sc != 8) { as.error("bad index scale"); return false; }
                it.scale = sc; it.sized = true; p++;
            }
            skipWs(p);
            if (*p) { as.error("junk after register in '" + t + "'"); return false; }
            return true;
        }
        it.kind = 2;
        it.esize = stripSize(t);
        it.e = as.parseExprStr(t);
        return it.e != nullptr;
    }
    static bool baseCapable(int r) { return (r >= A0 && r < A0 + 8) || r == PC || r == ZPC || (r >= ZA0 && r < ZA0 + 8); }

    bool indexed(const std::string& prefix, const std::string& inner, Operand& o) {
        std::vector<std::string> parts;
        splitTop(inner, ',', parts);
        std::vector<Item> items(parts.size());
        for (size_t i = 0; i < parts.size(); i++) if (!item(parts[i], items[i])) return false;
        const Expr* outer = nullptr;
        int outerSize = 0;
        if (!prefix.empty()) {
            std::string pf = prefix;
            outerSize = stripSize(pf);
            outer = as.parseExprStr(pf);
            if (!outer) return false;
        }
        int base = -1;
        bool haveIndex = false, bracket = false, idxInside = false;
        auto addReg = [&](const Item& it, bool inside) -> bool {
            bool plainD = it.r < 8 || (it.r >= ZD0 && it.r < ZD0 + 8);
            if (base < 0 && baseCapable(it.r) && !it.sized) { base = it.r; return true; }
            if (!haveIndex && (plainD || (it.r >= A0 && it.r < A0 + 8) || (it.r >= ZA0 && it.r < ZA0 + 8))) {
                haveIndex = true; o.ireg = it.r; o.isize = it.size; o.iscale = it.scale; idxInside = inside;
                return true;
            }
            as.error("bad register combination in operand");
            return false;
        };
        auto setDisp = [&](const Expr*& slot, int& ssize, const Item& it) -> bool {
            if (slot) { as.error("too many displacements in operand"); return false; }
            slot = it.e; ssize = it.esize;
            return true;
        };
        for (const Item& it : items) {
            if (it.kind == 3) {
                if (bracket) { as.error("nested memory indirection"); return false; }
                bracket = true;
                std::vector<std::string> ps;
                splitTop(it.br, ',', ps);
                for (const std::string& s : ps) {
                    Item x;
                    if (!item(s, x)) return false;
                    if (x.kind == 1) { if (!addReg(x, true)) return false; }
                    else if (x.kind == 2) { if (!setDisp(o.disp, o.dsize, x)) return false; }
                    else { as.error("nested memory indirection"); return false; }
                }
            } else if (it.kind == 1) {
                if (bracket) {
                    // after the bracket only an index is possible
                    if (haveIndex) { as.error("two index registers"); return false; }
                    haveIndex = true; o.ireg = it.r; o.isize = it.size; o.iscale = it.scale; idxInside = false;
                } else if (!addReg(it, false)) return false;
            } else {
                if (bracket) { if (!setDisp(o.odisp, o.osize, it)) return false; }
                else if (!setDisp(o.disp, o.dsize, it)) return false;
            }
        }
        if (outer) {
            if (bracket) { as.error("displacement outside of memory-indirect operand"); return false; }
            if (o.disp) { as.error("too many displacements in operand"); return false; }
            o.disp = outer; o.dsize = outerSize;
        }
        // a lone data register that became "index" while an address register follows: (d0,a0)
        o.reg = base;
        if (bracket) o.mode = (haveIndex && idxInside) ? PRE : POST;
        else if (!haveIndex) {
            if (base >= A0 && base < A0 + 8) o.mode = o.disp ? DISP : AINDR;
            else if (base == PC) o.mode = DISP;
            else if (base >= 0) o.mode = BASE;
            else { as.error("bad operand"); return false; }
        } else o.mode = BASE;
        return true;
    }

    bool regList(const char* p, Operand& o) {
        uint32_t mask = 0;
        auto bit = [](int r) -> int {
            if (r < 16) return r;
            if (r >= FP0 && r < FP0 + 8) return 16 + (r - FP0);
            if (r == FPIAR) return 24;
            if (r == FPSR) return 25;
            if (r == FPCR) return 26;
            return -1;
        };
        for (;;) {
            skipWs(p);
            int r1 = reg(p);
            int b1 = r1 < 0 ? -1 : bit(r1);
            if (b1 < 0) { as.error("bad register list"); return false; }
            skipWs(p);
            if (*p == '-') {
                p++; skipWs(p);
                int r2 = reg(p);
                int b2 = r2 < 0 ? -1 : bit(r2);
                if (b2 < 0 || b2 >= 24 || b1 >= 24 || (b1 < 16) != (b2 < 16)) { as.error("bad register range"); return false; }
                if (b2 < b1) std::swap(b1, b2);
                for (int i = b1; i <= b2; i++) mask |= 1u << i;
            } else mask |= 1u << b1;
            skipWs(p);
            if (*p == '/') { p++; continue; }
            break;
        }
        if (*p) { as.error("junk after register list"); return false; }
        o.mode = REGLST; o.mask = mask;
        return true;
    }

    bool operand(const std::string& s0, Operand& o) {
        std::string s = trim(s0);
        if (s.empty()) { as.error("missing operand"); return false; }
        if (s[0] == '#' || (s[0] == '&' && !as.mot)) {
            o.mode = IMMED;
            o.text = trim(s.substr(1));
            as.quiet++;
            bool savedFailed = as.failed;
            o.disp = as.parseExprStr(o.text);
            as.quiet--;
            as.failed = savedFailed;
            return true;
        }
        const char* p = s.c_str();
        int r = reg(p);
        if (r >= 0) {
            skipWs(p);
            if (!*p) {
                o.reg = r;
                o.mode = r < 8 ? DREG : r < 16 ? AREG : (r >= FP0 && r < FP0 + 8) ? FPREG : CONTROL;
                return true;
            }
            if (*p == '-' || *p == '/') return regList(s.c_str(), o);
            if (*p == '@') {
                // MIT syntax: reg@, reg@+, reg@-, reg@(d), reg@(d,idx:size:scale)
                if (!((r >= A0 && r < A0 + 8) || r == PC)) { as.error("bad base register"); return false; }
                p++;
                if (!*p) { o.mode = r == PC ? DISP : AINDR; o.reg = r; return r != PC; }
                if (*p == '+' && !p[1]) { o.mode = AINC; o.reg = r; return r != PC; }
                if (*p == '-' && !p[1]) { o.mode = ADEC; o.reg = r; return r != PC; }
                if (*p == '(' && s.back() == ')') {
                    std::string inner(p + 1, s.c_str() + s.size() - 1);
                    if (inner.find(")@(") != std::string::npos) { as.error("MIT memory-indirect syntax is not supported"); return false; }
                    std::string regtxt = s.substr(0, (size_t)(p - 1 - s.c_str()));
                    return indexed("", inner + "," + regtxt, o);
                }
                as.error("bad operand '" + s + "'");
                return false;
            }
            if (*p != '(' ) { as.error("junk after register in '" + s + "'"); return false; }
        }
        // -(An)
        if (s[0] == '-' && s.size() > 3 && s.back() == ')') {
            const char* q = s.c_str() + 1;
            skipWs(q);
            if (*q == '(') {
                q++;
                skipWs(q);
                int ar = reg(q);
                skipWs(q);
                if (ar >= A0 && ar < A0 + 8 && *q == ')' && !q[1]) { o.mode = ADEC; o.reg = ar; return true; }
            }
        }
        // (An)+
        if (s[0] == '(' && s.size() > 3 && s.back() == '+') {
            const char* q = s.c_str() + 1;
            skipWs(q);
            int ar = reg(q);
            skipWs(q);
            if (ar >= A0 && ar < A0 + 8 && *q == ')') {
                q++; skipWs(q);
                if (*q == '+' && !q[1]) { o.mode = AINC; o.reg = ar; return true; }
            }
        }
        // something(...) with registers inside?
        std::string body = s;
        int absSize = 0;
        if (body.size() > 3 && body[body.size() - 3] == ')') absSize = stripSize(body);
        if (body.back() == ')') {
            int depth = 0;
            size_t open = std::string::npos;
            for (size_t i = body.size(); i-- > 0;) {
                if (body[i] == ')') depth++;
                else if (body[i] == '(') { if (--depth == 0) { open = i; break; } }
            }
            if (open != std::string::npos) {
                std::string inner = body.substr(open + 1, body.size() - open - 2);
                // does the group contain a register or a bracket at top level?
                std::vector<std::string> parts;
                splitTop(inner, ',', parts);
                bool special = false;
                for (const std::string& part : parts) {
                    if (!part.empty() && part[0] == '[') { special = true; break; }
                    const char* q = part.c_str();
                    if (reg(q) >= 0) { special = true; break; }
                }
                std::string prefix = trim(body.substr(0, open));
                // "sym(%pc)" style needs an operator-free join: prefix must not end with an operator
                if (special && (prefix.empty() || !strchr("+-*/&|^<>~!(,", prefix.back()))) {
                    if (absSize) { as.error("size suffix not allowed here"); return false; }
                    return indexed(prefix, inner, o);
                }
            }
        }
        // absolute address / plain expression
        std::string t = s;
        o.dsize = stripSize(t);
        if (t.size() > 2 && t[0] == '(' && t.back() == ')') {
            // (expr).w keeps working; nothing special to do
        }
        o.disp = as.parseExprStr(t);
        if (!o.disp) return false;
        o.mode = ABSL;
        return true;
    }
};

// ---------------------------------------------------------------- templates
struct Tpl {
    unsigned short nw, w0, w1;
    const char* args;
    unsigned cpu;
};
struct Table {
    std::vector<Tpl> tpls;
    std::unordered_map<std::string, std::vector<int>> byName;
    std::deque<std::string> strs;
    void T(const std::string& name, int nw, unsigned w0, unsigned w1, const std::string& args, unsigned cpu) {
        strs.push_back(args);
        Tpl t;
        t.nw = (unsigned short)nw; t.w0 = (unsigned short)w0; t.w1 = (unsigned short)w1; t.args = strs.back().c_str(); t.cpu = cpu;
        byName[name].push_back((int)tpls.size());
        tpls.push_back(t);
    }
    Table();
};

static const char* const ccNames[16] = {"t", "f", "hi", "ls", "cc", "cs", "ne", "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"};
static const char* const fccNames[32] = {"f", "eq", "ogt", "oge", "olt", "ole", "ogl", "or", "un", "ueq", "ugt", "uge", "ult", "ule", "ne", "t",
                                         "sf", "seq", "gt", "ge", "lt", "le", "gl", "gle", "ngle", "ngl", "nle", "nlt", "nge", "ngt", "sne", "st"};

Table::Table() {
    typedef std::string S;
    const char sz[] = "bwl";
    auto ea = [](char cls, int z) { S r; r += cls; r += "bwl"[z]; return r; };

    T("abcd", 1, 0xC100, 0, "DsDd", ALL); T("abcd", 1, 0xC108, 0, "-s-d", ALL);
    T("sbcd", 1, 0x8100, 0, "DsDd", ALL); T("sbcd", 1, 0x8108, 0, "-s-d", ALL);
    T("sbcdb", 1, 0x8100, 0, "DsDd", ALL); T("sbcdb", 1, 0x8108, 0, "-s-d", ALL);

    // add / sub
    for (int k = 0; k < 2; k++) {
        S n = k ? "sub" : "add";
        unsigned reg = k ? 0x9000 : 0xD000, imm = k ? 0x0400 : 0x0600, quick = k ? 0x5100 : 0x5000;
        for (int z = 0; z < 3; z++) {
            unsigned zs = (unsigned)z << 6;
            std::vector<S> names(1, n + sz[z]);
            if (z == 1) names.push_back(n);
            for (const S& x : names) {
                T(x, 1, quick | zs, 0, S("Qd") + (z == 0 ? (k ? "%s" : "$b") : z == 1 ? "%w" : "%l"), ALL);
                T(x, 1, imm | zs, 0, S("#") + sz[z] + "$s", ALL);
                if (z) T(x, 1, reg | (z == 1 ? 0x00C0 : 0x01C0), 0, ea('*', z) + "Ad", ALL);
                T(x, 1, reg | zs, 0, ea(z ? '*' : ';', z) + "Dd", ALL);
                T(x, 1, reg | 0x100 | zs, 0, "Dd~s", ALL);
            }
            std::vector<S> in(1, n + "i" + sz[z]), qn(1, n + "q" + sz[z]), xn(1, n + "x" + sz[z]);
            if (z == 1) { in.push_back(n + "i"); qn.push_back(n + "q"); xn.push_back(n + "x"); }
            for (const S& x : in) T(x, 1, imm | zs, 0, S("#") + sz[z] + "$s", ALL);
            for (const S& x : qn) T(x, 1, quick | zs, 0, S("Qd") + (z == 0 ? (k ? "%s" : "$b") : z == 1 ? "%w" : "%l"), ALL);
            for (const S& x : xn) { T(x, 1, reg | 0x100 | zs, 0, "DsDd", ALL); T(x, 1, reg | 0x108 | zs, 0, "-s-d", ALL); }
            if (z) {
                std::vector<S> an(1, n + "a" + sz[z]);
                if (z == 1) an.push_back(n + "a");
                for (const S& x : an) T(x, 1, reg | (z == 1 ? 0x00C0 : 0x01C0), 0, ea('*', z) + "Ad", ALL);
            }
        }
    }
    // and / or / eor
    for (int k = 0; k < 3; k++) {
        S n = k == 0 ? "and" : k == 1 ? "or" : "eor";
        unsigned reg = k == 0 ? 0xC000 : k == 1 ? 0x8000 : 0xB000, imm = k == 0 ? 0x0200 : k == 1 ? 0x0000 : 0x0A00;
        for (int z = 0; z < 3; z++) {
            unsigned zs = (unsigned)z << 6;
            std::vector<S> names(1, n + sz[z]), in(1, n + "i" + sz[z]);
            if (z == 1) { names.push_back(n); in.push_back(n + "i"); }
            for (int pass = 0; pass < 2; pass++) {
                for (const S& x : (pass ? in : names)) {
                    T(x, 1, imm | zs, 0, S("#") + sz[z] + "$s", ALL);
                    if (z == 0 || x == n || x == n + "i") T(x, 1, imm | 0x3C, 0, "#bCs", ALL);
                    if (z == 1) T(x, 1, imm | 0x7C, 0, "#wSs", ALL);
                    if (pass) continue;
                    if (k != 2) T(x, 1, reg | zs, 0, ea(';', z) + "Dd", ALL);
                    T(x, 1, reg | 0x100 | zs, 0, k == 2 ? "Dd$s" : "Dd~s", ALL);
                }
            }
        }
    }
    // cmp
    for (int z = 0; z < 3; z++) {
        unsigned zs = (unsigned)z << 6;
        std::vector<S> names(1, S("cmp") + sz[z]), in(1, S("cmpi") + sz[z]), mn(1, S("cmpm") + sz[z]);
        if (z == 1) { names.push_back("cmp"); in.push_back("cmpi"); mn.push_back("cmpm"); }
        for (int pass = 0; pass < 2; pass++)
            for (const S& x : (pass ? in : names)) {
                T(x, 1, 0x0C00 | zs, 0, S("#") + sz[z] + "$s", C000 | C010);
                T(x, 1, 0x0C00 | zs, 0, S("#") + sz[z] + "@s", U20C);
                if (pass) continue;
                if (z) T(x, 1, 0xB000 | (z == 1 ? 0x00C0 : 0x01C0), 0, ea('*', z) + "Ad", ALL);
                T(x, 1, 0xB108 | zs, 0, "+s+d", ALL);
                T(x, 1, 0xB000 | zs, 0, ea(z ? '*' : ';', z) + "Dd", ALL);
            }
        for (const S& x : mn) T(x, 1, 0xB108 | zs, 0, "+s+d", ALL);
        if (z) {
            std::vector<S> an(1, S("cmpa") + sz[z]);
            if (z == 1) an.push_back("cmpa");
            for (const S& x : an) T(x, 1, 0xB000 | (z == 1 ? 0x00C0 : 0x01C0), 0, ea('*', z) + "Ad", ALL);
        }
        // chk2 / cmp2
        T(S("chk2") + sz[z], 2, 0x00C0 | ((unsigned)z << 9), 0x0800, "!sR1", U20C);
        T(S("cmp2") + sz[z], 2, 0x00C0 | ((unsigned)z << 9), 0x0000, "!sR1", U20C);
        if (z == 1) { T("chk2", 2, 0x02C0, 0x0800, "!sR1", U20C); T("cmp2", 2, 0x02C0, 0x0000, "!sR1", U20C); }
        // cas / moves
        T(S("cas") + sz[z], 2, 0x08C0 | ((unsigned)(z + 1) << 9), 0, "D3D2~s", U20);
        T(S("moves") + sz[z], 2, 0x0E00 | zs, 0x0000, "~sR1", U10);
        T(S("moves") + sz[z], 2, 0x0E00 | zs, 0x0800, "R1~s", U10);
        if (z == 1) { T("moves", 2, 0x0E00 | zs, 0x0000, "~sR1", U10); T("moves", 2, 0x0E00 | zs, 0x0800, "R1~s", U10); }
        // clr neg negx not tst
        static const struct { const char* n; unsigned op; } un[] = {{"clr", 0x4200}, {"neg", 0x4400}, {"negx", 0x4000}, {"not", 0x4600}};
        for (int i = 0; i < 4; i++) {
            T(S(un[i].n) + sz[z], 1, un[i].op | zs, 0, "$s", ALL);
            if (z == 1) T(un[i].n, 1, un[i].op | zs, 0, "$s", ALL);
        }
        std::vector<S> tn(1, S("tst") + sz[z]);
        if (z == 1) tn.push_back("tst");
        for (const S& x : tn) {
            T(x, 1, 0x4A00 | zs, 0, ea(z ? '*' : ';', z), U20C);
            T(x, 1, 0x4A00 | zs, 0, ea('$', z), C000 | C010);
        }
    }
    T("cas", 2, 0x0CC0, 0, "D3D2~s", U20);
    T("cas2w", 3, 0x0CFC, 0, "D3D6D2D5r1r4", U20); T("cas2l", 3, 0x0EFC, 0, "D3D6D2D5r1r4", U20);
    T("cas2", 3, 0x0CFC, 0, "D3D6D2D5r1r4", U20);

    // shifts and rotates
    {
        static const struct { const char* n; unsigned t, dir; } sh[] = {{"asl", 0, 1}, {"asr", 0, 0}, {"lsl", 1, 1}, {"lsr", 1, 0},
                                                                         {"roxl", 2, 1}, {"roxr", 2, 0}, {"rol", 3, 1}, {"ror", 3, 0}};
        for (int i = 0; i < 8; i++) {
            for (int z = 0; z < 3; z++) {
                unsigned op = 0xE000 | (sh[i].dir << 8) | ((unsigned)z << 6) | (sh[i].t << 3);
                std::vector<S> names(1, S(sh[i].n) + sz[z]);
                if (z == 1) names.push_back(sh[i].n);
                for (const S& x : names) {
                    T(x, 1, op, 0, "QdDs", ALL);
                    T(x, 1, op | 0x20, 0, "DdDs", ALL);
                    if (z == 1) T(x, 1, 0xE0C0 | (sh[i].t << 9) | (sh[i].dir << 8), 0, "~s", ALL);
                }
            }
        }
    }
    // bit operations
    for (int v = 0; v < 3; v++) {
        static const char* const suf[3] = {"", "b", "l"};
        T(S("btst") + suf[v], 1, 0x0100, 0, "Dd;b", ALL); T(S("btst") + suf[v], 1, 0x0800, 0, "#b@s", ALL);
        static const struct { const char* n; unsigned op; } b[] = {{"bchg", 0x40}, {"bclr", 0x80}, {"bset", 0xC0}};
        for (int i = 0; i < 3; i++) { T(S(b[i].n) + suf[v], 1, 0x0100 | b[i].op, 0, "Dd$s", ALL); T(S(b[i].n) + suf[v], 1, 0x0800 | b[i].op, 0, "#b$s", ALL); }
    }
    // bit fields
    T("bftst", 2, 0xE8C0, 0, "/sO2O3", U20); T("bfextu", 2, 0xE9C0, 0, "/sO2O3D1", U20); T("bfchg", 2, 0xEAC0, 0, "?sO2O3", U20);
    T("bfexts", 2, 0xEBC0, 0, "/sO2O3D1", U20); T("bfclr", 2, 0xECC0, 0, "?sO2O3", U20); T("bfffo", 2, 0xEDC0, 0, "/sO2O3D1", U20);
    T("bfset", 2, 0xEEC0, 0, "?sO2O3", U20); T("bfins", 2, 0xEFC0, 0, "D1?sO2O3", U20);

    // branches
    for (int i = 0; i < 20; i++) {
        S cc; unsigned c;
        if (i < 16) { cc = i == 0 ? "ra" : i == 1 ? "sr" : ccNames[i]; c = (unsigned)i; }
        else if (i == 16) { cc = "hs"; c = 4; } else if (i == 17) { cc = "lo"; c = 5; }
        else continue;
        unsigned op = 0x6000 | (c << 8);
        S n = "b" + cc;
        T(n + "s", 1, op, 0, "BB", ALL); T(n + "b", 1, op, 0, "BB", ALL); T(n + "w", 1, op, 0, "BW", ALL);
        T(n + "l", 1, op, 0, "BL", U20C); T(n, 1, op, 0, "BW", ALL);
        S j = i == 1 ? "jbsr" : "j" + cc;
        T(j, 1, op, 0, "Bj", ALL);
        if (i == 0) { T("jbra", 1, op, 0, "Bj", ALL); T("jra", 1, 0x4EC0, 0, "!s", ALL); T("jbra", 1, 0x4EC0, 0, "!s", ALL); }
        if (i == 1) T("jbsr", 1, 0x4E80, 0, "!s", ALL);
    }
    for (int i = 0; i < 19; i++) {
        S cc; unsigned c;
        if (i < 16) { cc = ccNames[i]; c = (unsigned)i; }
        else if (i == 16) { cc = "hs"; c = 4; } else if (i == 17) continue; else { cc = "ra"; c = 1; }
        T("db" + cc, 1, 0x50C8 | (c << 8), 0, "DsBv", ALL); T("db" + cc + "w", 1, 0x50C8 | (c << 8), 0, "DsBv", ALL);
        if (i >= 16) continue;
        T("s" + cc, 1, 0x50C0 | (c << 8), 0, "$s", ALL); T("s" + cc + "b", 1, 0x50C0 | (c << 8), 0, "$s", ALL);
        T("trap" + cc, 1, 0x50FC | (c << 8), 0, "", U20C);
        T("trap" + cc + "w", 1, 0x50FA | (c << 8), 0, "#w", U20C);
        T("trap" + cc + "l", 1, 0x50FB | (c << 8), 0, "#l", U20C);
    }
    T("bkpt", 1, 0x4848, 0, "qs", U10);
    T("bgnd", 1, 0x4AFA, 0, "", C32);
    T("callm", 1, 0x06C0, 0, "#b!s", C020); T("rtm", 1, 0x06C0, 0, "Rs", C020);
    T("chkw", 1, 0x4180, 0, ";wDd", ALL); T("chk", 1, 0x4180, 0, ";wDd", ALL); T("chkl", 1, 0x4100, 0, ";lDd", U20C);
    T("divsw", 1, 0x81C0, 0, ";wDd", ALL); T("divs", 1, 0x81C0, 0, ";wDd", ALL);
    T("divuw", 1, 0x80C0, 0, ";wDd", ALL); T("divu", 1, 0x80C0, 0, ";wDd", ALL);
    T("mulsw", 1, 0xC1C0, 0, ";wDd", ALL); T("muls", 1, 0xC1C0, 0, ";wDd", ALL);
    T("muluw", 1, 0xC0C0, 0, ";wDd", ALL); T("mulu", 1, 0xC0C0, 0, ";wDd", ALL);
    T("mulsl", 2, 0x4C00, 0x0800, ";lD1", U20C); T("mulsl", 2, 0x4C00, 0x0C00, ";lD3D1", U20C);
    T("mulul", 2, 0x4C00, 0x0000, ";lD1", U20C); T("mulul", 2, 0x4C00, 0x0400, ";lD3D1", U20C);
    T("divsl", 2, 0x4C40, 0x0C00, ";lD3D1", U20C); T("divsl", 2, 0x4C40, 0x0800, ";lDD", U20C);
    T("divul", 2, 0x4C40, 0x0400, ";lD3D1", U20C); T("divul", 2, 0x4C40, 0x0000, ";lDD", U20C);
    T("divsll", 2, 0x4C40, 0x0800, ";lD3D1", U20C); T("divsll", 2, 0x4C40, 0x0800, ";lDD", U20C);
    T("divull", 2, 0x4C40, 0x0000, ";lD3D1", U20C); T("divull", 2, 0x4C40, 0x0000, ";lDD", U20C);
    T("exg", 1, 0xC140, 0, "DdDs", ALL); T("exg", 1, 0xC148, 0, "AdAs", ALL); T("exg", 1, 0xC188, 0, "DdAs", ALL); T("exg", 1, 0xC188, 0, "AsDd", ALL);
    T("extw", 1, 0x4880, 0, "Ds", ALL); T("ext", 1, 0x4880, 0, "Ds", ALL); T("extl", 1, 0x48C0, 0, "Ds", ALL);
    T("extbw", 1, 0x4880, 0, "Ds", ALL); T("extbl", 1, 0x49C0, 0, "Ds", U20C);
    T("illegal", 1, 0x4AFC, 0, "", ALL);
    for (int v = 0; v < 3; v++) {
        static const char* const suf[3] = {"", "s", "l"};
        T(S("jmp") + suf[v], 1, 0x4EC0, 0, "!s", ALL); T(S("jsr") + suf[v], 1, 0x4E80, 0, "!s", ALL);
    }
    T("lea", 1, 0x41C0, 0, "!sAd", ALL); T("leal", 1, 0x41C0, 0, "!sAd", ALL);
    T("linkw", 1, 0x4E50, 0, "As#w", ALL); T("linkl", 1, 0x4808, 0, "As#l", U20C);
    T("link", 1, 0x4E50, 0, "As#z", ALL); T("link", 1, 0x4808, 0, "As#l", U20C);
    T("lpstop", 2, 0xF800, 0x01C0, "#w", C32 | C060);
    // move
    T("moveal", 1, 0x2040, 0, "*lAd", ALL); T("moveaw", 1, 0x3040, 0, "*wAd", ALL); T("movea", 1, 0x3040, 0, "*wAd", ALL);
    T("movec", 2, 0x4E7A, 0, "JsR1", U10); T("movec", 2, 0x4E7B, 0, "R1Js", U10);
    T("movecl", 2, 0x4E7A, 0, "JsR1", U10); T("movecl", 2, 0x4E7B, 0, "R1Js", U10);
    for (int z = 1; z < 3; z++) {
        unsigned zs = z == 1 ? 0 : 0x40;
        std::vector<S> names(1, S("movem") + sz[z]);
        if (z == 1) names.push_back("movem");
        for (const S& x : names) {
            T(x, 2, 0x4880 | zs, 0, "Lw&s", ALL); T(x, 2, 0x48A0 | zs, 0, "lw-s", ALL);
            T(x, 2, 0x4C80 | zs, 0, "<sLw", ALL);
            T(x, 2, 0x4880 | zs, 0, "#W>s", ALL); T(x, 2, 0x4C80 | zs, 0, "<s#W", ALL);
        }
        std::vector<S> pn(1, S("movep") + sz[z]);
        if (z == 1) pn.push_back("movep");
        for (const S& x : pn) { T(x, 1, 0x0108 | zs, 0, "dsDd", ALL); T(x, 1, 0x0188 | zs, 0, "Ddds", ALL); }
    }
    T("moveq", 1, 0x7000, 0, "MsDd", ALL); T("moveql", 1, 0x7000, 0, "MsDd", ALL);
    T("moveb", 1, 0x1000, 0, ";b$d", ALL);
    for (int k = 0; k < 2; k++) {
        S x = k ? "move" : "movew";
        T(x, 1, 0x3000, 0, "*w%d", ALL);
        T(x, 1, 0x40C0, 0, "Ss$s", ALL); T(x, 1, 0x42C0, 0, "Cs$s", U10);
        T(x, 1, 0x44C0, 0, ";wCd", ALL); T(x, 1, 0x46C0, 0, ";wSd", ALL);
        if (k) { T(x, 1, 0x4E60, 0, "AsUd", ALL); T(x, 1, 0x4E68, 0, "UdAs", ALL); }
    }
    T("movel", 1, 0x7000, 0, "NsDd", ALL); T("movel", 1, 0x2000, 0, "*l%d", ALL);
    T("movel", 1, 0x4E60, 0, "AsUd", ALL); T("movel", 1, 0x4E68, 0, "UdAs", ALL);
    T("move16", 2, 0xF620, 0x8000, "+s+1", U40); T("move16", 1, 0xF600, 0, "+s_L", U40); T("move16", 1, 0xF608, 0, "_L+s", U40);
    T("move16", 1, 0xF610, 0, "as_L", U40); T("move16", 1, 0xF618, 0, "_Las", U40);
    T("nbcd", 1, 0x4800, 0, "$s", ALL); T("nbcdb", 1, 0x4800, 0, "$s", ALL);
    T("nop", 1, 0x4E71, 0, "", ALL);
    T("pack", 1, 0x8140, 0, "DsDd#w", U20); T("pack", 1, 0x8148, 0, "-s-d#w", U20);
    T("unpk", 1, 0x8180, 0, "DsDd#w", U20); T("unpk", 1, 0x8188, 0, "-s-d#w", U20);
    T("pea", 1, 0x4840, 0, "!s", ALL); T("peal", 1, 0x4840, 0, "!s", ALL);
    T("reset", 1, 0x4E70, 0, "", ALL); T("rtd", 1, 0x4E74, 0, "#w", U10); T("rte", 1, 0x4E73, 0, "", ALL);
    T("rtr", 1, 0x4E77, 0, "", ALL); T("rts", 1, 0x4E75, 0, "", ALL); T("stop", 1, 0x4E72, 0, "#w", ALL);
    T("swap", 1, 0x4840, 0, "Ds", ALL); T("swapw", 1, 0x4840, 0, "Ds", ALL);
    T("tas", 1, 0x4AC0, 0, "$s", ALL); T("tasb", 1, 0x4AC0, 0, "$s", ALL); T("trap", 1, 0x4E40, 0, "Ts", ALL); T("trapv", 1, 0x4E76, 0, "", ALL);
    T("unlk", 1, 0x4E58, 0, "As", ALL);
    // CPU32 table lookup
    for (int k = 0; k < 4; k++) {
        static const char* const tn[4] = {"tblu", "tblun", "tbls", "tblsn"};
        unsigned fl = (k >= 2 ? 0x0800u : 0u) | ((k & 1) ? 0x0400u : 0u);
        for (int z = 0; z < 3; z++) {
            T(S(tn[k]) + sz[z], 2, 0xF800, fl | 0x0100 | ((unsigned)z << 6), "!sD1", C32);
            T(S(tn[k]) + sz[z], 2, 0xF800, fl | ((unsigned)z << 6), "DsD3D1", C32);
        }
    }
    // 68040 / 68060 cache and MMU
    T("cinvl", 1, 0xF408, 0, "ceas", U40); T("cinvp", 1, 0xF410, 0, "ceas", U40); T("cinva", 1, 0xF418, 0, "ce", U40);
    T("cpushl", 1, 0xF428, 0, "ceas", U40); T("cpushp", 1, 0xF430, 0, "ceas", U40); T("cpusha", 1, 0xF438, 0, "ce", U40);
    T("pflush", 1, 0xF508, 0, "as", U40); T("pflushn", 1, 0xF500, 0, "as", U40);
    T("pflusha", 1, 0xF518, 0, "", U40); T("pflushan", 1, 0xF510, 0, "", U40);
    T("ptestr", 1, 0xF568, 0, "as", C040); T("ptestw", 1, 0xF548, 0, "as", C040);
    T("plpar", 1, 0xF5C8, 0, "as", C060); T("plpaw", 1, 0xF588, 0, "as", C060);
    // 68030 / 68851 MMU
    {
        const unsigned M3 = C030 | MMU;
        T("pmove", 2, 0xF000, 0x4000, "&sP8", M3); T("pmove", 2, 0xF000, 0x4200, "P8&s", M3); T("pmovefd", 2, 0xF000, 0x4100, "&sP8", C030);
        T("pmove", 2, 0xF000, 0x0000, "&sW8", C030); T("pmove", 2, 0xF000, 0x0200, "W8&s", C030); T("pmovefd", 2, 0xF000, 0x0100, "&sW8", C030);
        T("pmove", 2, 0xF000, 0x6000, "&sY8", M3); T("pmove", 2, 0xF000, 0x6200, "Y8&s", M3);
        T("pflusha", 2, 0xF000, 0x2400, "", M3);
        T("pflush", 2, 0xF000, 0x3000, "x3m9", M3); T("pflush", 2, 0xF000, 0x3800, "x3m9&s", M3);
        T("ploadr", 2, 0xF000, 0x2200, "x3&s", M3); T("ploadw", 2, 0xF000, 0x2000, "x3&s", M3);
        T("ptestr", 2, 0xF000, 0x8200, "x3&sv8", M3); T("ptestr", 2, 0xF000, 0x8300, "x3&sv8A9", M3);
        T("ptestw", 2, 0xF000, 0x8000, "x3&sv8", M3); T("ptestw", 2, 0xF000, 0x8100, "x3&sv8A9", M3);
    }
    // ---------------- FPU
    {
        static const struct { const char* n; unsigned op; int kind; unsigned cpu; } fop[] = { // kind 0 monadic, 1 dyadic
            {"fmove", 0x00, 2, FPU}, {"fint", 0x01, 0, FPU}, {"fsinh", 0x02, 0, FPU}, {"fintrz", 0x03, 0, FPU}, {"fsqrt", 0x04, 0, FPU},
            {"flognp1", 0x06, 0, FPU}, {"fetoxm1", 0x08, 0, FPU}, {"ftanh", 0x09, 0, FPU}, {"fatan", 0x0A, 0, FPU}, {"fasin", 0x0C, 0, FPU},
            {"fatanh", 0x0D, 0, FPU}, {"fsin", 0x0E, 0, FPU}, {"ftan", 0x0F, 0, FPU}, {"fetox", 0x10, 0, FPU}, {"ftwotox", 0x11, 0, FPU},
            {"ftentox", 0x12, 0, FPU}, {"flogn", 0x14, 0, FPU}, {"flog10", 0x15, 0, FPU}, {"flog2", 0x16, 0, FPU}, {"fabs", 0x18, 0, FPU},
            {"fcosh", 0x19, 0, FPU}, {"fneg", 0x1A, 0, FPU}, {"facos", 0x1C, 0, FPU}, {"fcos", 0x1D, 0, FPU}, {"fgetexp", 0x1E, 0, FPU},
            {"fgetman", 0x1F, 0, FPU}, {"fdiv", 0x20, 1, FPU}, {"fmod", 0x21, 1, FPU}, {"fadd", 0x22, 1, FPU}, {"fmul", 0x23, 1, FPU},
            {"fsgldiv", 0x24, 1, FPU}, {"frem", 0x25, 1, FPU}, {"fscale", 0x26, 1, FPU}, {"fsglmul", 0x27, 1, FPU}, {"fsub", 0x28, 1, FPU},
            {"fcmp", 0x38, 1, FPU},
            {"fsmove", 0x40, 2, F040}, {"fdmove", 0x44, 2, F040}, {"fssqrt", 0x41, 0, F040}, {"fdsqrt", 0x45, 0, F040},
            {"fsabs", 0x58, 0, F040}, {"fdabs", 0x5C, 0, F040}, {"fsneg", 0x5A, 0, F040}, {"fdneg", 0x5E, 0, F040},
            {"fsadd", 0x62, 1, F040}, {"fdadd", 0x66, 1, F040}, {"fssub", 0x68, 1, F040}, {"fdsub", 0x6C, 1, F040},
            {"fsmul", 0x63, 1, F040}, {"fdmul", 0x67, 1, F040}, {"fsdiv", 0x60, 1, F040}, {"fddiv", 0x64, 1, F040}};
        static const struct { char c; unsigned code; const char* spec; } ff[] = {
            {'b', 6, ";b"}, {'w', 4, ";w"}, {'l', 0, ";l"}, {'s', 1, ";f"}, {'d', 5, ";F"}, {'x', 2, ";x"}, {'p', 3, ";p"}};
        for (size_t i = 0; i < sizeof fop / sizeof fop[0]; i++) {
            S n = fop[i].n;
            unsigned c = fop[i].cpu;
            for (int f = 0; f < 7; f++) T(n + ff[f].c, 2, 0xF200, 0x4000 | (ff[f].code << 10) | fop[i].op, S(ff[f].spec) + "F7", c);
            T(n + "x", 2, 0xF200, fop[i].op, "F8F7", c);
            if (fop[i].kind == 0 || fop[i].op == 0x24 || fop[i].op == 0x27 || fop[i].op == 0x28) T(n + "x", 2, 0xF200, fop[i].op, "Ft", c);
        }
        // fmove to memory, control registers
        T("fmoveb", 2, 0xF200, 0x7800, "F7$s", FPU); T("fmovew", 2, 0xF200, 0x7000, "F7$s", FPU); T("fmovel", 2, 0xF200, 0x6000, "F7$s", FPU);
        T("fmoves", 2, 0xF200, 0x6400, "F7$s", FPU); T("fmoved", 2, 0xF200, 0x7400, "F7~s", FPU); T("fmovex", 2, 0xF200, 0x6800, "F7~s", FPU);
        T("fmovep", 2, 0xF200, 0x6C00, "F7~s#C", FPU); T("fmovep", 2, 0xF200, 0x7C00, "F7~sDk", FPU);
        T("fmovel", 2, 0xF200, 0xA000, "i8%s", FPU); T("fmovel", 2, 0xF200, 0x8000, "*li8", FPU);
        for (int k = 0; k < 2; k++) {
            S x = k ? "fmovem" : "fmoveml";
            T(x, 2, 0xF200, 0xA000, "s8%s", FPU); T(x, 2, 0xF200, 0x8000, "*ls8", FPU);
        }
        for (int k = 0; k < 2; k++) {
            S x = k ? "fmovem" : "fmovemx";
            T(x, 2, 0xF200, 0xF000, "E3&s", FPU); T(x, 2, 0xF220, 0xE000, "Er-s", FPU); T(x, 2, 0xF200, 0xD000, "]sE3", FPU);
            if (!k) { T(x, 2, 0xF200, 0xF800, "Dk&s", FPU); T(x, 2, 0xF220, 0xE800, "Dk-s", FPU); T(x, 2, 0xF200, 0xD800, "]sDk", FPU); }
            T(x, 2, 0xF200, 0xF000, "#3&s", FPU); T(x, 2, 0xF220, 0xE000, "#3-s", FPU); T(x, 2, 0xF200, 0xD000, "]s#3", FPU);
        }
        T("fmovecr", 2, 0xF200, 0x5C00, "#CF7", FPU); T("fmovecrx", 2, 0xF200, 0x5C00, "#CF7", FPU);
        for (int f = 0; f < 7; f++) {
            T(S("ftst") + ff[f].c, 2, 0xF200, 0x4000 | (ff[f].code << 10) | 0x3A, ff[f].spec, FPU);
            T(S("fsincos") + ff[f].c, 2, 0xF200, 0x4000 | (ff[f].code << 10) | 0x30, S(ff[f].spec) + "F3F7", FPU);
        }
        T("ftstx", 2, 0xF200, 0x003A, "F8", FPU);
        T("fsincosx", 2, 0xF200, 0x0030, "F8F3F7", FPU);
        for (unsigned i = 0; i < 32; i++) {
            S cc = fccNames[i];
            T("fb" + cc, 1, 0xF280 | i, 0, "Bc", FPU); T("fb" + cc + "l", 1, 0xF2C0 | i, 0, "BM", FPU);
            T("fj" + cc, 1, 0xF280 | i, 0, "Bc", FPU);
            T("fdb" + cc, 2, 0xF248, i, "DsBw", FPU);
            T("fs" + cc, 2, 0xF240, i, "$s", FPU);
            T("ftrap" + cc, 2, 0xF27C, i, "", FPU); T("ftrap" + cc + "w", 2, 0xF27A, i, "#w", FPU); T("ftrap" + cc + "l", 2, 0xF27B, i, "#l", FPU);
        }
        T("fnop", 2, 0xF280, 0, "", FPU);
        T("fsave", 1, 0xF300, 0, ">s", FPU); T("frestore", 1, 0xF340, 0, "<s", FPU);
    }
}

static const Table& table() {
    static const Table t;
    return t;
}

// ---------------------------------------------------------------- matching
static int eaKind(const Operand& o) {
    switch (o.mode) {
    case DREG: return 0; case AREG: return 1; case AINDR: return 2; case AINC: return 3; case ADEC: return 4;
    case DISP: return o.reg == PC ? 8 : 5;
    case BASE: case POST: case PRE: return (o.reg == PC || o.reg == ZPC) ? 9 : 6;
    case ABSL: return 7;
    case IMMED: return 10;
    }
    return -1;
}
static unsigned eaClassMask(char c) {
    switch (c) {
    case '*': return 0x7FF;
    case '~': return 0x0FC;
    case '%': return 0x0FF;
    case ';': return 0x7FD;
    case '@': return 0x3FD;
    case '!': return 0x3E4;
    case '&': return 0x0E4;
    case '$': return 0x0FD;
    case '?': return 0x0E5;
    case '/': return 0x3E5;
    case '>': return 0x0F4;
    case '<': return 0x3EC;
    case ']': return 0x0EC; // alterable control or postincrement (fmovem to registers)
    }
    return 0;
}
static bool isbyte(int64_t v) { return v >= -255 && v <= 255; }
static bool issbyte(int64_t v) { return v >= -128 && v <= 127; }
static bool isword(int64_t v) { return v >= -65535 && v <= 65535; }
static bool issword(int64_t v) { return v >= -32768 && v <= 32767; }

struct MovecReg { int r; unsigned code, cpu; };
static const MovecReg movecRegs[] = {
    {SFC, 0x000, U10}, {DFC, 0x001, U10}, {USP, 0x800, U10}, {VBR, 0x801, U10},
    {CACR, 0x002, U20}, {CAAR, 0x802, C020 | C030}, {MSP, 0x803, C020 | C030 | C040}, {ISP, 0x804, C020 | C030 | C040},
    {TC, 0x003, U40}, {ITT0, 0x004, U40}, {ITT1, 0x005, U40}, {DTT0, 0x006, U40}, {DTT1, 0x007, U40},
    {MMUSR, 0x805, C040}, {PSR, 0x805, C040}, {URP, 0x806, U40}, {SRP, 0x807, U40}, {BUSCR, 0x008, C060}, {PCR, 0x808, C060}, {-1, 0, 0}};

static bool matchOp(const Operand& o, char t, char pl, unsigned cpu) {
    unsigned cm = eaClassMask(t);
    if (cm) {
        int k = eaKind(o);
        if (k < 0 || !((cm >> k) & 1)) return false;
        if (k == 1 && pl == 'b') return false;
        if (k == 10) {
            if (pl == 'f' || pl == 'F' || pl == 'x' || pl == 'p') return true;
            if (!o.disp) return false;
            if (o.dconst && pl == 'b' && !isbyte(o.dval)) return false;
            if (o.dconst && pl == 'w' && !isword(o.dval)) return false;
        }
        return true;
    }
    bool imm = o.mode == IMMED && o.disp;
    bool cimm = imm && o.dconst;
    switch (t) {
    case 'D': return o.mode == DREG;
    case 'A': return o.mode == AREG;
    case 'R': return o.mode == DREG || o.mode == AREG;
    case 'F': return o.mode == FPREG;
    case 'a': return o.mode == AINDR;
    case '+': return o.mode == AINC;
    case '-': return o.mode == ADEC;
    case 'r': return o.mode == AINDR || (o.mode == BASE && o.reg < 0 && o.ireg >= 0 && o.ireg < 8 && !o.disp && o.iscale == 1);
    case 'Q': return cimm && o.dval >= 1 && o.dval <= 8;
    case 'M': return imm && (!o.dconst || issbyte(o.dval));
    case 'N': return cimm && issbyte(o.dval);
    case 'T': return cimm && o.dval >= 0 && o.dval <= 15;
    case 'q': case 'm': case 'v': return cimm && o.dval >= 0 && o.dval <= 7;
    case '#':
        if (!imm) return false;
        if (pl == 'b') return !o.dconst || isbyte(o.dval);
        if (pl == 'w') return !o.dconst || isword(o.dval);
        if (pl == 'z') return !o.dconst || issword(o.dval);
        if (pl == 'C') return o.dconst && o.dval >= 0 && o.dval <= 127;
        if (pl == '3') return o.dconst;
        if (pl == 'W') return !o.dconst || isword(o.dval);
        return true;
    case 'B': case '_': return o.mode == ABSL;
    case 'd': return o.mode == DISP && o.reg != PC;
    case 'C': return o.mode == CONTROL && o.reg == CCR;
    case 'S': return o.mode == CONTROL && o.reg == SR;
    case 'U': return o.mode == CONTROL && o.reg == USP;
    case 'J':
        if (o.mode != CONTROL) return false;
        for (int i = 0; movecRegs[i].r >= 0; i++) if (movecRegs[i].r == o.reg) return true;
        return false;
    case 'L': case 'l': return o.mode == DREG || o.mode == AREG || (o.mode == REGLST && !(o.mask & ~0xFFFFu));
    case 'E': return o.mode == FPREG || (o.mode == REGLST && !(o.mask & ~0xFF0000u));
    case 's': return (o.mode == CONTROL && (o.reg == FPCR || o.reg == FPSR || o.reg == FPIAR)) || (o.mode == REGLST && !(o.mask & ~0x7000000u));
    case 'i': return o.mode == CONTROL && (o.reg == FPCR || o.reg == FPSR || o.reg == FPIAR);
    case 'O': return o.mode == DREG || cimm;
    case 'k': return o.mode == DREG;
    case 'c': return o.mode == CONTROL && o.reg >= NC && o.reg <= BC;
    case 'x': return o.mode == DREG || (o.mode == CONTROL && (o.reg == SFC || o.reg == DFC)) || (cimm && o.dval >= 0 && o.dval <= ((cpu & MMU) ? 15 : 7));
    case 'P': return o.mode == CONTROL && (o.reg == TC || o.reg == SRP || o.reg == CRP);
    case 'W': return o.mode == CONTROL && (o.reg == TT0 || o.reg == TT1);
    case 'Y': return o.mode == CONTROL && (o.reg == MMUSR || o.reg == PSR);
    }
    return false;
}

// ---------------------------------------------------------------- encoding
struct Enc {
    Asm& as;
    Build& b;
    unsigned cpu;
    const Tpl* tpl = nullptr;
    Enc(Asm& a, Build& bb, unsigned c) : as(a), b(bb), cpu(c) {}

    void word(uint32_t v) { put16(b.b, v & 0xFFFF); }
    void orw(int i, uint32_t v) { b.b[(size_t)i * 2] |= (unsigned char)(v >> 8); b.b[(size_t)i * 2 + 1] |= (unsigned char)v; }
    void fail(const std::string& m) { if (b.err.empty()) b.err = m; }
    void fix(int size, const Expr* e, bool pcrel, int pcadj, int back = 0, bool sgn = false) {
        Fixup f;
        f.off = (uint32_t)((int)b.b.size() - back); f.size = size; f.pcrel = pcrel; f.pcadj = pcadj; f.e = e; f.sgn = sgn || pcrel;
        b.fx.push_back(f);
    }
    bool simpleSym(const Expr* e) {
        Val v = as.eval(e, false);
        if (v.bad) return false;
        as.reduce(v, false);
        return v.t.size() == 1 && v.t[0].coef == 1;
    }
    void relax(int rtype, const Expr* e, size_t opOff) {
        b.rtype = rtype; b.rexpr = e; b.relaxAt = b.b.size(); b.relaxOp = opOff;
    }
    void install(char pl, uint32_t v) {
        switch (pl) {
        case 's': orw(0, v); break;
        case 'd': orw(0, v << 9); break;
        case '1': orw(1, v << 12); break;
        case '2': orw(1, v << 6); break;
        case '3': orw(1, v); break;
        case '4': orw(2, v << 12); break;
        case '5': orw(2, v << 6); break;
        case '6': orw(2, v); break;
        case '7': orw(1, v << 7); break;
        case '8': orw(1, v << 10); break;
        case '9': orw(1, v << 5); break;
        case 'D': orw(1, (v << 12) | v); break;
        case 't': orw(1, (v << 10) | (v << 7)); break;
        case 'k': orw(1, v << 4); break;
        case 'e': orw(0, v << 6); break;
        case 'w': case 'W': orw(1, v & 0xFFFF); break;
        case 'C': orw(1, v & 0x7F); break;
        case 'r': orw(1, v); break;
        default: fail("internal: bad operand place"); break;
        }
    }
    void immediate(const Operand& o, char pl) {
        if (pl == 'f' || pl == 'F' || pl == 'x' || pl == 'p') {
            Bytes out;
            if (!as.parseFloatImm(o.text, pl, out, true)) { fail("bad floating point constant '" + o.text + "'"); return; }
            b.b.insert(b.b.end(), out.begin(), out.end());
            return;
        }
        if (!o.disp) { fail("bad immediate operand"); return; }
        int64_t v = o.dconst ? o.dval : 0;
        if (pl == 'b') {
            if (o.dconst) { if (!isbyte(v)) fail("immediate out of range"); word((uint32_t)v & 0xFFFF); }
            else { word(0); fix(1, o.disp, false, 0, 1); }
        } else if (pl == 'l') {
            if (!o.dconst) fix(4, o.disp, false, 0);
            word((uint32_t)((uint64_t)v >> 16)); word((uint32_t)v);
        } else {
            if (o.dconst) { if (!isword(v)) fail("immediate out of range"); }
            else fix(2, o.disp, false, 0);
            word((uint32_t)v);
        }
    }
    // Returns the 6-bit mode/register field. cls is the template's operand class.
    uint32_t ea(const Operand& o, char cls, char pl) {
        bool m20 = (cpu & (U20 | C32)) != 0;
        switch (o.mode) {
        case DREG: return (uint32_t)o.reg;
        case AREG: return 0x08u + (uint32_t)(o.reg - 8);
        case AINDR: return 0x10u + (uint32_t)(o.reg - 8);
        case AINC: return 0x18u + (uint32_t)(o.reg - 8);
        case ADEC: return 0x20u + (uint32_t)(o.reg - 8);
        case IMMED: immediate(o, pl == 'd' || pl == 's' ? 'w' : pl); return 0x3C;
        case DISP: {
            int64_t v = o.dconst ? o.dval : 0;
            bool var = !o.dconst;
            if (!var && v == 0 && o.dsize == 0 && o.reg != PC) return 0x10u + (uint32_t)(o.reg - 8);
            if (!issword(v) || (var && ((o.dsize == 0 && m20) || o.dsize == 3))) {
                if (!m20) { fail("displacement too large for this architecture; needs 68020 or higher"); return 0; }
                uint32_t r = o.reg == PC ? 0x3Bu : 0x30u + (uint32_t)(o.reg - 8);
                if (var) {
                    if (o.reg == PC) {
                        if (o.dsize == 3 || b.rtype || !simpleSym(o.disp)) { word(0x0170); fix(4, o.disp, true, 2); }
                        else { relax(RX_PCREL1632, o.disp, 0); return r; }
                    } else { word(0x0170); fix(4, o.disp, false, 0); }
                } else word(0x0170);
                word((uint32_t)((uint64_t)v >> 16)); word((uint32_t)v);
                return r;
            }
            uint32_t r = o.reg == PC ? 0x3Au : 0x28u + (uint32_t)(o.reg - 8);
            if (var) fix(2, o.disp, o.reg == PC, 0, 0, true);
            word((uint32_t)v);
            return r;
        }
        case BASE: case POST: case PRE: {
            uint32_t nw = 0, r;
            int64_t baseo = o.dconst ? o.dval : 0, outro = o.oconst ? o.oval : 0;
            bool var = !o.dconst, ovar = !o.oconst;
            bool pc = o.reg == PC || o.reg == ZPC;
            if (pc) { r = 0x3B; if (o.reg == ZPC) nw |= 0x80; }
            else if (o.reg < 0) { nw |= 0x80; r = 0x30; }
            else if (o.reg >= ZA0) { nw |= 0x80; r = 0x30u + (uint32_t)(o.reg - ZA0); }
            else r = 0x30u + (uint32_t)(o.reg - 8);
            int siz1 = o.dsize, siz2 = (o.mode == BASE) ? 0 : o.osize;
            if (o.ireg >= 0 && o.ireg < 16) {
                nw |= (uint32_t)o.ireg << 12;
                if (o.isize == 0 || o.isize == 3) nw |= 0x800;
                if (o.isize == 1) fail("byte index register not allowed");
                if (o.iscale != 1 && !m20) fail("scale factor invalid on this architecture; needs cpu32 or 68020 or higher");
                nw |= o.iscale == 2 ? 0x200u : o.iscale == 4 ? 0x400u : o.iscale == 8 ? 0x600u : 0u;
                if (o.mode == BASE && ((o.reg >= 8 && o.reg < 16) || o.reg == PC)) {
                    if (siz1 == 1 || !m20 || (siz1 == 0 && !var && issbyte(baseo))) {
                        nw += (uint32_t)baseo & 0xFF;
                        word(nw);
                        if (var) fix(1, o.disp, o.reg == PC, o.reg == PC ? 1 : 0, 1, true);
                        else if (!issbyte(baseo)) fail("byte displacement out of range");
                        return r;
                    }
                    if (siz1 == 0 && o.reg == PC && var && !b.rtype && simpleSym(o.disp)) {
                        word(nw);
                        relax(RX_PCINDEX, o.disp, 0);
                        return r;
                    }
                }
            } else {
                nw |= 0x40;
                if (o.ireg >= ZD0 && o.ireg < ZD0 + 8) nw |= (uint32_t)(o.ireg - ZD0) << 12;
                else if (o.ireg >= ZA0) nw |= (uint32_t)(o.ireg - ZA0 + 8) << 12;
            }
            if (!m20) { fail("invalid operand mode for this architecture; needs 68020 or higher"); return 0; }
            nw |= 0x100;
            switch (siz1) {
            case 0:
                if (var ? true : !issword(baseo)) { siz1 = 3; nw |= 0x30; }
                else if (!var && baseo == 0) nw |= 0x10;
                else { nw |= 0x20; siz1 = 2; }
                break;
            case 1: case 2: nw |= 0x20; siz1 = 2; break;
            default: nw |= 0x30; break;
            }
            if (o.mode != BASE) {
                if (cpu & C32) fail("invalid operand mode for this architecture; needs 68020 or higher");
                switch (siz2) {
                case 0:
                    if (ovar ? true : !issword(outro)) { siz2 = 3; nw |= 0x3; }
                    else if (!ovar && outro == 0) nw |= 0x1;
                    else { nw |= 0x2; siz2 = 2; }
                    break;
                case 1: case 2: nw |= 0x2; siz2 = 2; break;
                default: nw |= 0x3; break;
                }
                if (o.mode == POST && !(nw & 0x40)) nw |= 0x04;
            }
            word(nw);
            if (siz1 != 0 && var) fix(siz1 == 3 ? 4 : 2, o.disp, pc, pc ? 2 : 0, 0, siz1 != 3);
            if (siz1 == 3) word((uint32_t)((uint64_t)baseo >> 16));
            if (siz1 != 0) word((uint32_t)baseo);
            if (siz2 != 0 && ovar) fix(siz2 == 3 ? 4 : 2, o.odisp, false, 0, 0, siz2 != 3);
            if (siz2 == 3) word((uint32_t)((uint64_t)outro >> 16));
            if (siz2 != 0) word((uint32_t)outro);
            return r;
        }
        case ABSL: {
            int64_t v = o.dconst ? o.dval : 0;
            bool var = !o.dconst;
            if (o.dsize == 1) { fail("invalid byte size on absolute address"); return 0; }
            if (o.dsize == 2 || (o.dsize == 0 && !var && issword(v))) {
                if (var) fix(2, o.disp, false, 0);
                word((uint32_t)v);
                return 0x38;
            }
            if (o.dsize == 0 && var && !strchr("~%&$?]", cls) && !b.rtype && simpleSym(o.disp)) {
                relax(RX_ABSTOPCREL, o.disp, 0);
                return 0x3A;
            }
            if (var) fix(4, o.disp, false, 0);
            word((uint32_t)((uint64_t)v >> 16)); word((uint32_t)v);
            return 0x39;
        }
        }
        fail("bad operand");
        return 0;
    }

    void branch(const Operand& o, char pl) {
        bool m20 = (cpu & (U20 | C32)) != 0;
        switch (pl) {
        case 'B': fix(1, o.disp, true, -1, 1); b.fx.back().nz = true; break;
        case 'W': case 'w': fix(2, o.disp, true, 0); word(0); break;
        case 'L': orw(0, 0xFF); fix(4, o.disp, true, 0); word(0); word(0); break;
        case 'M': fix(4, o.disp, true, 0); word(0); word(0); break; // fbcc.l (opcode already has the long bit)
        case 'g': relax(RX_BRANCHBW, o.disp, 0); break;
        case 'j': {
            bool uncond = (b.b[0] == 0x60 || b.b[0] == 0x61);
            if (o.dconst) { // absolute address: jump right away
                if (uncond) { bool bsr = b.b[0] == 0x61; b.b[0] = 0x4E; b.b[1] = bsr ? 0xB9 : 0xF9; }
                else { b.b[0] ^= 0x01; b.b[1] = 0x06; word(0x4EF9); }
                word((uint32_t)((uint64_t)o.dval >> 16)); word((uint32_t)o.dval);
                break;
            }
            relax(m20 ? RX_BRANCHBWL : uncond ? RX_BRABSJUNC : RX_BRABSJCOND, o.disp, 0);
            break;
        }
        case 'v':
            if (o.dconst) word(0); // GNU as emits a zero displacement for a constant target
            else relax(m20 ? RX_DBCCLBR : RX_DBCCABSJ, o.disp, 0);
            break;
        case 'c': relax(RX_FBRANCH, o.disp, 0); break;
        default: fail("internal: bad branch place");
        }
    }

    void op(const Operand& o, char t, char pl) {
        if (eaClassMask(t)) {
            uint32_t r = ea(o, t, pl);
            if (pl == 'd') orw(0, ((r & 7) << 9) | ((r >> 3) << 6));
            else orw(0, r);
            return;
        }
        switch (t) {
        case 'D': case 'q': case 'T': case 'm': case 'v':
            install(pl, (uint32_t)(o.mode == DREG ? o.reg : o.dval));
            break;
        case 'A': case 'a': case '+': case '-': install(pl, (uint32_t)(o.reg - 8)); break;
        case 'R': install(pl, (uint32_t)o.reg); break;
        case 'r': install(pl, (uint32_t)(o.mode == AINDR ? o.reg : o.ireg)); break;
        case 'F': install(pl, (uint32_t)(o.reg - FP0)); break;
        case 'Q': install(pl, (uint32_t)(o.dval & 7)); break;
        case 'M': case 'N':
            if (o.dconst) orw(0, (uint32_t)o.dval & 0xFF);
            else fix(1, o.disp, false, 0, (int)b.b.size() - 1);
            break;
        case '#':
            if (pl == 'b' || pl == 'w' || pl == 'l') immediate(o, pl);
            else if (pl == 'z') immediate(o, 'w');
            else if (pl == 'W') { if (o.dconst) orw(1, (uint32_t)o.dval & 0xFFFF); else fix(2, o.disp, false, 0, (int)b.b.size() - 2); }
            else install(pl, (uint32_t)o.dval & (pl == 'C' ? 0x7Fu : 0xFFu));
            break;
        case 'B': branch(o, pl); break;
        case '_':
            if (!o.dconst) fix(4, o.disp, false, 0);
            word((uint32_t)((uint64_t)(o.dconst ? o.dval : 0) >> 16)); word((uint32_t)(o.dconst ? o.dval : 0));
            break;
        case 'd':
            install(pl, (uint32_t)(o.reg - 8));
            if (o.mode == AINDR || o.dconst) { int64_t v = o.mode == AINDR ? 0 : o.dval; if (!issword(v)) fail("displacement out of range"); word((uint32_t)v); }
            else { fix(2, o.disp, false, 0, 0, true); word(0); }
            break;
        case 'C': case 'S': case 'U': case 'Y': break;
        case 'J': {
            for (int i = 0; movecRegs[i].r >= 0; i++) if (movecRegs[i].r == o.reg) {
                if (!(movecRegs[i].cpu & cpu)) fail("control register not available on the selected CPU");
                orw(1, movecRegs[i].code);
                return;
            }
            break;
        }
        case 'L': case 'l': {
            uint32_t m = o.mode == REGLST ? o.mask : (1u << o.reg);
            if (t == 'l') { uint32_t rv = 0; for (int i = 0; i < 16; i++) if (m & (1u << i)) rv |= 1u << (15 - i); m = rv; }
            orw(1, m);
            break;
        }
        case 'E': {
            uint32_t m = o.mode == REGLST ? (o.mask >> 16) & 0xFF : (1u << (o.reg - FP0));
            if (pl == '3') { uint32_t rv = 0; for (int i = 0; i < 8; i++) if (m & (1u << i)) rv |= 1u << (7 - i); m = rv; }
            orw(1, m);
            break;
        }
        case 's': case 'i': {
            uint32_t m = o.mode == REGLST ? (o.mask >> 24) & 7 : (o.reg == FPCR ? 4u : o.reg == FPSR ? 2u : 1u);
            orw(1, m << 10);
            break;
        }
        case 'O':
            if (o.mode == DREG) install(pl, 0x20u | (uint32_t)o.reg);
            else install(pl, (uint32_t)o.dval & 0x1F);
            break;
        case 'k': install(pl, (uint32_t)o.reg); break;
        case 'c': install(pl, (uint32_t)(o.reg - NC)); break;
        case 'x':
            if (o.mode == DREG) orw(1, 0x08u | (uint32_t)o.reg);
            else if (o.mode == CONTROL) orw(1, o.reg == DFC ? 1u : 0u);
            else orw(1, 0x10u | (uint32_t)o.dval);
            break;
        case 'P': install(pl, o.reg == TC ? 0u : o.reg == SRP ? 2u : 3u); break;
        case 'W': install(pl, o.reg == TT0 ? 2u : 3u); break;
        default: fail("internal: bad operand type"); break;
        }
    }
};

} // namespace

bool Asm::isMnemonic(const std::string& n0) {
    std::string n;
    for (char c : n0) if (c != '.') n += c;
    return table().byName.count(n) != 0;
}

void Asm::assembleInsn(const std::string& mnem0, const std::string& opsStr) {
    const Table& tb = table();
    std::string name;
    for (char c : mnem0) if (c != '.') name += c;
    auto it = tb.byName.find(name);
    if (it == tb.byName.end()) {
        // size suffixes GNU as does not know but classic Motorola sources use (lea.l, btst.b, ...)
        size_t dot = mnem0.rfind('.');
        if (dot != std::string::npos && dot + 2 == mnem0.size()) {
            std::string base = mnem0.substr(0, dot);
            static const char* const ok[] = {"lea", "pea", "moveq", "btst", "bchg", "bclr", "bset", "swap", "exg", "tas", "nbcd", "abcd", "sbcd",
                                             "jmp", "jsr", "stop", "trap", "rtd", "movec", "bkpt", "ext", nullptr};
            bool allowed = false;
            for (int i = 0; ok[i]; i++) if (base == ok[i]) allowed = true;
            if (base.size() >= 2 && base[0] == 's' && tb.byName.count(base) && base != "sub" && base != "swap") allowed = true; // scc.b
            if (base.compare(0, 2, "db") == 0) allowed = true;
            if (allowed && mot) it = tb.byName.find(base);
        }
        if (it == tb.byName.end()) { error("unknown instruction '" + mnem0 + "'"); return; }
    }
    // split and parse operands
    Parser ps(*this);
    std::vector<std::string> parts;
    {
        const char* p = opsStr.c_str();
        skipWs(p);
        if (*p) {
            std::vector<std::string> raw;
            Parser::splitTop(p, ',', raw);
            for (std::string s : raw) {
                // bit field / k-factor suffix
                std::vector<std::string> extra;
                if (!s.empty() && s.back() == '}') {
                    size_t open = s.rfind('{');
                    if (open != std::string::npos) {
                        std::string inside = s.substr(open + 1, s.size() - open - 2);
                        s = Parser::trim(s.substr(0, open));
                        std::vector<std::string> f;
                        Parser::splitTop(inside, ':', f);
                        for (std::string x : f) {
                            const char* q = x.c_str();
                            if (!x.empty() && x[0] != '#' && ps.reg(q) < 0) x = "#" + x;
                            extra.push_back(x);
                        }
                    }
                }
                // register pairs  Dh:Dl  (An):(Am)  FPc:FPs
                std::vector<std::string> pr;
                Parser::splitTop(s, ':', pr);
                if (pr.size() == 2 && ps.isRegOrInd(pr[0]) && ps.isRegOrInd(pr[1])) { parts.push_back(pr[0]); parts.push_back(pr[1]); }
                else parts.push_back(s);
                parts.insert(parts.end(), extra.begin(), extra.end());
            }
        }
    }
    std::vector<Operand> ops(parts.size());
    for (size_t i = 0; i < parts.size(); i++) {
        if (!ps.operand(parts[i], ops[i])) return;
        Operand& o = ops[i];
        // constants are 32-bit quantities: 0xffff8000 is -32768
        if (o.disp) { o.dconst = evalNow(o.disp, o.dval); if (!o.dconst) o.dval = 0; else o.dval = (int32_t)(uint32_t)o.dval; }
        if (o.odisp) { o.oconst = evalNow(o.odisp, o.oval); if (!o.oconst) o.oval = 0; else o.oval = (int32_t)(uint32_t)o.oval; }
    }
    const Tpl* cpuMiss = nullptr;
    for (int ti : it->second) {
        const Tpl& t = tb.tpls[(size_t)ti];
        size_t na = strlen(t.args) / 2;
        if (na != ops.size()) continue;
        bool ok = true;
        for (size_t i = 0; i < na && ok; i++) ok = matchOp(ops[i], t.args[i * 2], t.args[i * 2 + 1], cpu);
        if (!ok) continue;
        // several FPU control registers cannot be stored into a single register
        if (na == 2 && t.args[0] == 's' && ops[0].mode == REGLST && (ops[0].mask & (ops[0].mask - 1)) && (ops[1].mode == DREG || ops[1].mode == AREG)) continue;
        if (!(t.cpu & cpu)) { if (!cpuMiss) cpuMiss = &t; continue; }
        Build b;
        Enc e(*this, b, cpu);
        e.tpl = &t;
        e.word(t.w0);
        if (t.nw >= 2) e.word(t.w1);
        if (t.nw >= 3) e.word(0);
        for (size_t i = 0; i < na; i++) e.op(ops[i], t.args[i * 2], t.args[i * 2 + 1]);
        if (!b.err.empty()) { error(b.err); return; }
        emitBuild(b);
        return;
    }
    if (cpuMiss) error("instruction '" + mnem0 + "' with these operands is not supported by the selected CPU");
    else error("operands mismatch for '" + mnem0 + "'");
}

// ---------------------------------------------------------------- floating point constants
namespace {
struct Big {
    std::vector<uint32_t> d; // little endian
    void trimz() { while (!d.empty() && d.back() == 0) d.pop_back(); }
    bool zero() const { return d.empty(); }
    int bits() const {
        if (d.empty()) return 0;
        int n = (int)(d.size() - 1) * 32;
        uint32_t t = d.back();
        while (t) { n++; t >>= 1; }
        return n;
    }
    void mulSmall(uint32_t m) {
        uint64_t c = 0;
        for (uint32_t& x : d) { uint64_t v = (uint64_t)x * m + c; x = (uint32_t)v; c = v >> 32; }
        if (c) d.push_back((uint32_t)c);
    }
    void addSmall(uint32_t a) {
        uint64_t c = a;
        for (uint32_t& x : d) { if (!c) break; uint64_t v = (uint64_t)x + c; x = (uint32_t)v; c = v >> 32; }
        if (c) d.push_back((uint32_t)c);
    }
    void shl(int n) {
        if (n <= 0 || d.empty()) return;
        int w = n / 32, b = n % 32;
        if (b) {
            uint32_t c = 0;
            for (uint32_t& x : d) { uint32_t nx = (x << b) | c; c = x >> (32 - b); x = nx; }
            if (c) d.push_back(c);
        }
        if (w) d.insert(d.begin(), (size_t)w, 0u);
    }
    int cmp(const Big& o) const {
        if (d.size() != o.d.size()) return d.size() < o.d.size() ? -1 : 1;
        for (size_t i = d.size(); i-- > 0;) if (d[i] != o.d[i]) return d[i] < o.d[i] ? -1 : 1;
        return 0;
    }
    void sub(const Big& o) {
        int64_t br = 0;
        for (size_t i = 0; i < d.size(); i++) {
            int64_t v = (int64_t)d[i] - (i < o.d.size() ? (int64_t)o.d[i] : 0) - br;
            br = v < 0 ? 1 : 0;
            if (v < 0) v += ((int64_t)1 << 32);
            d[i] = (uint32_t)v;
        }
        trimz();
    }
};

// value = M * 10^e10, rounded to p bits: value ~= mant * 2^(e2-(p-1)), mant has exactly p bits
bool decToBin(const Big& M, int e10, int p, uint64_t& mant, int& e2) {
    if (M.zero() || p <= 0) { mant = 0; e2 = 0; return false; }
    Big N = M, D;
    D.d.push_back(1);
    for (int i = 0; i < e10; i++) N.mulSmall(10);
    for (int i = 0; i < -e10; i++) D.mulSmall(10);
    int sh = p - (N.bits() - D.bits());
    if (sh > 0) N.shl(sh); else D.shl(-sh);
    Big t = D;
    t.shl(p);
    if (N.cmp(t) >= 0) { D.shl(1); sh--; }
    t = D;
    t.shl(p - 1);
    if (N.cmp(t) < 0) { N.shl(1); sh++; }
    uint64_t q = 0;
    for (int i = p - 1; i >= 0; i--) {
        t = D;
        t.shl(i);
        if (N.cmp(t) >= 0) { N.sub(t); q |= (uint64_t)1 << i; }
    }
    N.shl(1);
    int c = N.cmp(D);
    if (c > 0 || (c == 0 && (q & 1))) {
        q++;
        if (p < 64 ? q == ((uint64_t)1 << p) : q == 0) { q = (uint64_t)1 << (p - 1); sh--; }
    }
    mant = q;
    e2 = (p - 1) - sh;
    return true;
}
} // namespace

bool Asm::parseFloatImm(const std::string& s0, int fmt, Bytes& out, bool intIsRaw) {
    std::string s = s0;
    size_t i = 0;
    int nbytes = fmt == 'f' ? 4 : fmt == 'F' ? 8 : 12;
    // integer expression -> raw bit pattern (GNU as behaviour for immediates)
    bool looksFloat = false;
    {
        std::string t = lower(s);
        size_t k = 0;
        if (k < t.size() && (t[k] == '+' || t[k] == '-')) k++;
        if (t.compare(k, 2, "0x") != 0) {
            if (t.size() > k + 2 && t[k] == '0' && (t[k + 1] == 'e' || t[k + 1] == 'f' || t[k + 1] == 'd' || t[k + 1] == 'r')) looksFloat = true;
            for (size_t j = k; j < t.size(); j++) if (t[j] == '.' || ((t[j] == 'e') && j > k && isdigit((unsigned char)t[j - 1]))) looksFloat = true;
            if (t.find("inf") != std::string::npos || t.find("nan") != std::string::npos) looksFloat = true;
            if (!intIsRaw && !looksFloat) { bool alldig = k < t.size(); for (size_t j = k; j < t.size(); j++) if (!isdigit((unsigned char)t[j])) alldig = false; if (alldig) looksFloat = true; }
        }
    }
    if (!looksFloat) {
        const Expr* e = parseExprStr(s);
        int64_t v;
        if (!e || !evalNow(e, v)) return false;
        for (int k = nbytes - 1; k >= 0; k--) out.push_back((unsigned char)(k >= 8 ? (v < 0 ? 0xFF : 0) : ((uint64_t)v >> (8 * k))));
        return true;
    }
    if (fmt == 'p') return false; // packed decimal literals are not supported
    bool neg = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) { neg = s[i] == '-'; i++; }
    if (i + 1 < s.size() && s[i] == '0' && strchr("eEfFdDrR", s[i + 1])) i += 2;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) { neg = neg != (s[i] == '-'); i++; }
    std::string rest = lower(s.substr(i));
    int p = fmt == 'f' ? 24 : fmt == 'F' ? 53 : 64;
    int emin = fmt == 'f' ? -126 : fmt == 'F' ? -1022 : -16382, emax = fmt == 'f' ? 127 : fmt == 'F' ? 1023 : 16383;
    uint64_t mant = 0;
    int e2 = 0;
    int cls = 0; // 0 finite, 1 inf, 2 nan, 3 zero
    if (rest.compare(0, 3, "inf") == 0) cls = 1;
    else if (rest.compare(0, 3, "nan") == 0) cls = 2;
    else {
        Big M;
        int e10 = 0;
        bool any = false, dot = false;
        size_t j = 0;
        for (; j < rest.size(); j++) {
            char c = rest[j];
            if (c >= '0' && c <= '9') { if (M.d.empty()) M.d.push_back(0); M.mulSmall(10); M.addSmall((uint32_t)(c - '0')); M.trimz(); if (dot) e10--; any = true; }
            else if (c == '.' && !dot) dot = true;
            else break;
        }
        if (!any) return false;
        if (j < rest.size() && rest[j] == 'e') {
            j++;
            bool eneg = false;
            if (j < rest.size() && (rest[j] == '+' || rest[j] == '-')) { eneg = rest[j] == '-'; j++; }
            if (j >= rest.size()) return false;
            int ev = 0;
            for (; j < rest.size() && isdigit((unsigned char)rest[j]); j++) if (ev < 100000) ev = ev * 10 + (rest[j] - '0');
            e10 += eneg ? -ev : ev;
        }
        if (j != rest.size()) return false;
        if (e10 > 6000) cls = M.zero() ? 3 : 1;
        else if (e10 < -6000 || M.zero()) cls = 3;
        else {
            decToBin(M, e10, p, mant, e2);
            if (e2 > emax) cls = 1;
            else if (e2 < emin) {
                int pp = p - (emin - e2);
                if (pp <= 0) cls = 3;
                else {
                    decToBin(M, e10, pp, mant, e2);
                    int shift = (e2 - (pp - 1)) - (emin - (p - 1));
                    mant <<= shift;
                    if (mant >> (p - 1)) e2 = emin; // rounded up to the smallest normal
                    else e2 = emin - 1;             // denormal marker
                }
            }
        }
    }
    if (fmt == 'x') {
        uint32_t ex = cls == 1 || cls == 2 ? 0x7FFF : cls == 3 ? 0 : (e2 < emin ? 0 : (uint32_t)(e2 + 16383));
        uint64_t m = cls == 1 ? 0 : cls == 2 ? ~(uint64_t)0 : cls == 3 ? 0 : mant;
        put16(out, (neg ? 0x8000u : 0u) | ex); put16(out, 0);
        put32(out, (uint32_t)(m >> 32)); put32(out, (uint32_t)m);
    } else if (fmt == 'F') {
        uint64_t ex = cls == 1 || cls == 2 ? 0x7FF : cls == 3 ? 0 : (e2 < emin ? 0 : (uint64_t)(e2 + 1023));
        uint64_t m = cls == 1 ? 0 : cls == 2 ? ((uint64_t)1 << 52) - 1 : cls == 3 ? 0 : (mant & (((uint64_t)1 << 52) - 1));
        uint64_t v = ((uint64_t)(neg ? 1 : 0) << 63) | (ex << 52) | m;
        put32(out, (uint32_t)(v >> 32)); put32(out, (uint32_t)v);
    } else {
        uint32_t ex = cls == 1 || cls == 2 ? 0xFF : cls == 3 ? 0 : (e2 < emin ? 0 : (uint32_t)(e2 + 127));
        uint32_t m = cls == 1 ? 0 : cls == 2 ? 0x7FFFFF : cls == 3 ? 0 : ((uint32_t)mant & 0x7FFFFF);
        put32(out, ((neg ? 1u : 0u) << 31) | (ex << 23) | m);
    }
    return true;
}

} } // namespace p68::asmblr
