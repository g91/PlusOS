// P68 assembler - internal declarations shared by assembler.cpp (driver, directives,
// expressions, layout, ELF writer) and m68k.cpp (operand parser, opcode table, encoder).
#pragma once
#include "assembler.h"
#include "../common/elf.h"
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace p68 { namespace asmblr {

struct Symbol;

struct Expr {
    enum Kind { NUM, SYM, UN, BIN };
    Kind k = NUM;
    int op = 0;              // operator code (see assembler.cpp)
    int64_t num = 0;
    Symbol* sym = nullptr;
    const Expr* a = nullptr;
    const Expr* b = nullptr;
};

enum { SEC_UNDEF = -1, SEC_ABS = -2, SEC_COMMON = -3, SEC_EXPR = -4 };

struct Symbol {
    std::string name;
    int sec = SEC_UNDEF;     // section index, or one of SEC_*
    int frag = 0;            // frag index inside the section (labels)
    int64_t value = 0;       // offset in frag (labels) / absolute value / common alignment
    const Expr* expr = nullptr; // SEC_EXPR: defined by .set/.equ to a non-constant expression
    const Expr* sizeExpr = nullptr;
    int64_t size = 0;
    bool global = false, weak = false, isSet = false, temp = false, used = false, declLocal = false;
    int type = 0;            // STT_*
    int vis = 0;             // st_other
    int elfIndex = -1;
    int evalBusy = 0;
};

struct Loc { int file = 0; int line = 0; };

struct Fixup {
    uint32_t off = 0;        // offset in frag data
    int size = 0;            // 1, 2, 4 (8: quad)
    bool pcrel = false;
    int pcadj = 0;           // field address minus the address the displacement is relative to
    const Expr* e = nullptr;
    Loc loc;
    bool sgn = false;        // strictly signed range check
    bool nz = false;         // a resolved value of zero is an error (byte branches)
};

enum { V_NONE, V_ALIGN, V_ORG, V_RELAX };
enum { // relaxation classes (mirrors what GNU as does for m68k)
    RX_BRANCHBW = 1,  // bcc/bra/bsr: byte or word
    RX_BRANCHBWL,     // jcc on 68020+: byte, word, long
    RX_BRABSJUNC,     // jra/jbsr on 68000: byte, word, jmp/jsr abs.l
    RX_BRABSJCOND,    // jcc on 68000: byte, word, inverted bcc.s over jmp abs.l
    RX_FBRANCH,       // fbcc: word, long
    RX_PCREL1632,     // d16(pc) or 32-bit bd (68020+)
    RX_PCINDEX,       // d8(pc,xn) or 32-bit bd (68020+)
    RX_ABSTOPCREL,    // absolute long -> d16(pc) for read-only operands
    RX_DBCCLBR,       // dbcc with long branch (68020+)
    RX_DBCCABSJ       // dbcc with absolute jump
};

struct Frag {
    Bytes data;
    std::vector<Fixup> fix;
    int vtype = V_NONE;
    uint32_t addr = 0, vsize = 0;
    int align = 1, fill = 0, maxSkip = 0; // V_ALIGN
    int64_t org = 0;                      // V_ORG
    int rtype = 0, state = 0, opOff = 0;  // V_RELAX
    const Expr* target = nullptr;
    Loc loc;
};

struct Rela { uint32_t off; int type; Symbol* sym; int secSym; int64_t addend; };

struct Section {
    std::string name;
    uint32_t type = SHT_PROGBITS, flags = 0, align = 1, entsize = 0;
    std::string group;
    std::vector<Frag> frags;
    uint32_t size = 0;
    int elfIndex = 0, symIndex = 0;
    Bytes out;
    std::vector<Rela> rel;
    bool nonZero = false;
};

struct Term { Symbol* s; int64_t coef; };
struct Val {
    int64_t c = 0;
    std::vector<Term> t;
    bool bad = false;
};

// Instruction under construction.
struct Build {
    Bytes b;
    std::vector<Fixup> fx;
    int rtype = 0;           // relaxation class (0: none)
    size_t relaxAt = 0;      // byte offset where the variable part goes
    size_t relaxOp = 0;      // byte offset of the word the relaxation patches
    const Expr* rexpr = nullptr;
    int rstate = 0;          // minimum state
    std::string err;
};

struct SrcLine { std::string text; int file; int line; };

struct Macro {
    std::string name;
    struct Arg { std::string name, def; bool req = false, vararg = false; };
    std::vector<Arg> args;
    std::vector<SrcLine> body;
};

struct Frame {
    std::vector<SrcLine> lines;
    size_t pos = 0;
    size_t condDepth = 0;
    bool isMacro = false;
};

struct Operand; // m68k.cpp

class Asm {
public:
    Asm(const AsOptions& o, std::string& d);
    bool run(const std::string& src, const std::string& fileName, Bytes& elf);

    // ---- shared services ----
    const AsOptions& opt;
    std::string& diag;
    unsigned cpu = 0;         // CPU feature mask (m68k.cpp)
    bool mot = false;         // Motorola syntax
    bool regOpt = false;      // registers without %
    int errors = 0;
    int quiet = 0;            // >0: errors are not recorded (speculative parsing)
    bool failed = false;      // set by error() even when quiet
    Loc cur;
    std::vector<std::string> files;

    void error(const std::string& msg);
    void errorAt(const Loc& l, const std::string& msg);
    void warn(const std::string& msg);

    const Expr* num(int64_t v);
    const Expr* parseExpr(const char*& p);              // nullptr (and error reported) on failure
    const Expr* parseExprStr(const std::string& s);     // whole string must be consumed
    bool evalNow(const Expr* e, int64_t& v);            // true when constant right now
    Val eval(const Expr* e, bool fin);
    void reduce(Val& v, bool fin);
    bool isRegName(const std::string& s);               // m68k.cpp (for prefix-less registers)

    Symbol* sym(const std::string& name);
    Symbol* findSym(const std::string& name);
    Symbol* dotSym();
    Section& sec() { return secs[curSec]; }
    Frag& frag() { return secs[curSec].frags.back(); }
    void emit8(uint32_t v) { frag().data.push_back((unsigned char)v); }
    void emitBytes(const unsigned char* p, size_t n) { Bytes& d = frag().data; d.insert(d.end(), p, p + n); }
    void emitExpr(const Expr* e, int size);
    void newFrag();
    void emitBuild(Build& b);

    // ---- m68k.cpp ----
    void setCpu(int cpuNum, bool fpu, bool mmu);
    bool setCpuByName(const std::string& name);
    void assembleInsn(const std::string& mnem, const std::string& ops);
    bool isMnemonic(const std::string& name);
    bool parseFloatImm(const std::string& s, int fmt, Bytes& out, bool intIsRaw = false); // fmt: 'f','F','x','p'
    std::map<std::string, std::string> regAlias;       // equr

private:
    std::deque<Expr> pool;
    std::deque<Symbol> symPool;
    std::unordered_map<std::string, Symbol*> symTab;
    std::vector<Symbol*> symOrder;
    std::vector<Section> secs;
    int curSec = 0, prevSec = 0;
    std::vector<std::pair<int, int>> secStack;
    std::map<std::string, Macro> macros;
    std::vector<Frame> frames;
    struct Cond { bool active, taken, parentActive, seenElse; };
    std::vector<Cond> conds;
    std::map<int, int> localCount;
    std::string scope;        // Motorola local-label scope
    int64_t rsCounter = 0;
    int macroCounter = 0;
    bool ended = false;
    int dotCounter = 0;
    int includeDepth = 0;
    std::vector<std::string> incDirs;

    bool skipping() const { return !conds.empty() && !conds.back().active; }
    void loadSource(const std::string& src, int fileIdx, std::vector<SrcLine>& out);
    void processLine(const SrcLine& l);
    void statement(const std::string& s, bool col0);
    bool conditional(const std::string& name, const char* args);
    bool directive(const std::string& name, const char* args, const std::string& label);
    void defineLabel(const std::string& name);
    void assign(const std::string& name, const char* p, bool isSet, bool equiv);
    void expandMacro(const Macro& m, const char* args, const std::string& sizeSuffix);
    bool collectBody(std::vector<SrcLine>& body, const char* open1, const char* open2, const char* close1, const char* close2);
    int findSection(const std::string& name, bool create, uint32_t type, uint32_t flags, bool explicitFlags);
    void switchSection(int idx);
    void doAlign(int64_t align, int fill, int maxSkip);
    void dataDir(int size, const char* p, bool motStrings);
    void floatDir(int fmt, const char* p);
    void stringDir(const char* p, bool zero);
    void spaceDir(int64_t count, int size, int64_t fillv);
    bool parseString(const char*& p, std::string& out);
    bool constExpr(const char*& p, int64_t& v);
    std::string localName(const std::string& name);
    const Expr* primary(const char*& p);
    const Expr* binary(const char*& p, int minPrec);
    void layout();
    void convert();
    void applyFixups();
    bool targetAddr(const Expr* e, int secIdx, int64_t& addr);
    void writeElf(Bytes& elf);
    uint32_t symAddr(const Symbol* s) const { return secs[s->sec].frags[s->frag].addr + (uint32_t)s->value; }
};

std::string lower(const std::string& s);
inline bool isSymStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.' || c == '$'; }
inline bool isSymChar(char c) { return isSymStart(c) || (c >= '0' && c <= '9'); }
inline void skipWs(const char*& p) { while (*p == ' ' || *p == '\t' || *p == '\r') p++; }

} } // namespace p68::asmblr
