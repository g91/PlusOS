// cc.h - shared declarations of the P68 C/C++ compiler (Motorola 680x0 target).
// Pipeline:  preprocess (pp.cpp) -> parse + type check (parse.cpp) -> IR (irgen.cpp)
//            -> optimise (opt.cpp) -> select/allocate/emit 68k assembly (m68k.cpp).
#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace p68 {

struct Type;
struct Node;
struct Obj;
struct Member;

// ---------------------------------------------------------------- options
struct Options {
    int cpu = 68000;              // 68000 68010 68020 68030 68040 68060
    bool fpu = false;             // hardware FPU (not used by codegen yet: soft-float)
    int optLevel = 2;             // 0..3 ; -Os == 2 + size bias
    bool optSize = false;
    bool cxx = false;             // C++ mode
    bool functionSections = false;
    bool dataSections = false;
    bool fixedA5 = true;          // Mac: A5 is the application globals pointer
    bool warnings = true;
    bool asmPreprocess = false;   // preprocessing a .S file
    bool shortEnums = false;
    std::vector<std::string> includeDirs;      // -I
    std::vector<std::string> sysIncludeDirs;   // -isystem / built-in
    std::vector<std::string> afterIncludeDirs; // -idirafter
    std::vector<std::string> defines;          // "NAME" or "NAME=VALUE"
    std::vector<std::string> undefs;
    std::string depFile;                       // -MMD output
    std::string depTarget;
};
extern Options opt;

// ---------------------------------------------------------------- diagnostics
struct SrcFile {
    std::string name;
    std::string text;
    int id = 0;
    bool system = false;
};

struct Hideset;

struct Token {
    enum Kind { END, IDENT, PUNCT, NUM, STR, PPNUM, OTHER } kind = END;
    std::string text;          // spelling
    SrcFile *file = nullptr;
    int line = 0;
    bool bol = false;          // first token on its line
    bool space = false;        // preceded by whitespace
    Hideset *hs = nullptr;
    Token *next = nullptr;
    // literals (valid after convertTokens)
    int64_t ival = 0;
    double fval = 0;
    Type *ty = nullptr;
    std::string sval;          // decoded string literal bytes (without NUL)
    int pack = 0;              // #pragma pack value in effect at this token (0 = default)
    bool is(const char *s) const { return text == s && (kind == PUNCT || kind == IDENT); }
};

[[noreturn]] void fatal(const char *fmt, ...);
[[noreturn]] void errorAt(const Token *tok, const char *fmt, ...);
void warnAt(const Token *tok, const char *fmt, ...);
std::string format(const char *fmt, ...);
bool readFile(const std::string &path, std::string &out);

// ---------------------------------------------------------------- preprocessor
SrcFile *loadSource(const std::string &path);
Token *tokenizeFile(SrcFile *f);
Token *preprocess(Token *tok);                       // macro-expands; returns token list
void ppInit();
void ppDefine(const std::string &name, const std::string &body);
void ppUndef(const std::string &name);
std::string tokensToText(Token *tok);                // -E output
extern std::vector<std::string> ppDependencies;      // files read (for -MMD)
void convertTokens(Token *tok);                      // pp-numbers/strings -> literals

// ---------------------------------------------------------------- types
enum TypeKind {
    TY_VOID, TY_BOOL, TY_CHAR, TY_SHORT, TY_INT, TY_LONG, TY_LLONG,
    TY_FLOAT, TY_DOUBLE, TY_ENUM, TY_PTR, TY_ARRAY, TY_FUNC, TY_STRUCT, TY_UNION
};
enum CallConv { CC_C, CC_PASCAL };

struct Member {
    std::string name;
    Type *ty = nullptr;
    int offset = 0;
    bool isBitfield = false;
    int bitOffset = 0;      // from the most significant bit of the storage unit (big endian)
    int bitWidth = 0;
    Token *tok = nullptr;
};

struct Type {
    TypeKind kind = TY_INT;
    int size = 0;
    int align = 1;
    bool isUnsigned = false;
    bool isConst = false;
    bool isVolatile = false;
    Type *base = nullptr;        // pointer/array element, function return type
    int arrayLen = -1;           // -1: incomplete
    // function
    std::vector<Type *> params;
    std::vector<std::string> paramNames;
    bool variadic = false;
    bool oldStyle = false;       // f() with unknown parameters
    CallConv cc = CC_C;
    // struct/union
    std::vector<Member> members;
    std::string tag;
    bool complete = true;
    bool packed = false;
    Type *origin = nullptr;      // unqualified original (for qualified copies of structs)
    Node *vlaLen = nullptr;      // not supported; kept for diagnostics
};

extern Type *tyVoid, *tyBool, *tyChar, *tySChar, *tyUChar, *tyShort, *tyUShort, *tyInt, *tyUInt,
    *tyLong, *tyULong, *tyLLong, *tyULLong, *tyFloat, *tyDouble;

Type *pointerTo(Type *base);
Type *arrayOf(Type *base, int len);
Type *funcType(Type *ret);
Type *copyType(Type *t);
Type *unqual(Type *t);
bool isInteger(const Type *t);
bool isFlonum(const Type *t);
bool isArith(const Type *t);
bool isScalar(const Type *t);
bool isAggregate(const Type *t);
bool sameType(const Type *a, const Type *b);
std::string typeName(const Type *t);

// ---------------------------------------------------------------- AST
enum NodeKind {
    ND_NUM, ND_FNUM, ND_VAR, ND_ADDR, ND_DEREF, ND_MEMBER,
    ND_NEG, ND_NOT, ND_BITNOT,
    ND_ADD, ND_SUB, ND_MUL, ND_DIV, ND_MOD, ND_AND, ND_OR, ND_XOR, ND_SHL, ND_SHR,
    ND_EQ, ND_NE, ND_LT, ND_LE, ND_LOGAND, ND_LOGOR,
    ND_ASSIGN, ND_ASSIGNOP, ND_POSTINC, ND_COND, ND_COMMA, ND_CALL, ND_CAST, ND_STMT_EXPR,
    ND_MEMZERO, ND_VA_START, ND_VA_ARG, ND_NULL_EXPR, ND_ALLOCA,
    // statements
    ND_EXPR_STMT, ND_IF, ND_FOR, ND_DO, ND_SWITCH, ND_CASE, ND_BLOCK, ND_GOTO, ND_LABEL,
    ND_RETURN, ND_ASM, ND_UNREACHABLE
};

struct Node {
    NodeKind kind;
    Token *tok = nullptr;
    Type *ty = nullptr;
    Node *lhs = nullptr, *rhs = nullptr;
    Node *cond = nullptr, *then = nullptr, *els = nullptr, *init = nullptr, *inc = nullptr;
    std::vector<Node *> body;       // block statements / call arguments
    Obj *var = nullptr;             // ND_VAR, ND_MEMZERO
    int64_t ival = 0;               // ND_NUM ; ND_POSTINC: delta ; ND_CASE: low
    int64_t ival2 = 0;              // ND_CASE: high (ranges)
    double fval = 0;                // ND_FNUM
    Member *member = nullptr;       // ND_MEMBER
    NodeKind op = ND_ADD;           // ND_ASSIGNOP operator
    Type *opTy = nullptr;           // ND_ASSIGNOP: type the arithmetic is done in
    Type *funcTy = nullptr;         // ND_CALL: callee type
    Obj *callee = nullptr;          // ND_CALL: direct callee if known
    std::string label;              // goto/label name, asm text
    int labelId = -1;               // break/continue target ids, unique label ids
    int brkId = -1, contId = -1;
    std::vector<Node *> cases;      // ND_SWITCH
    Node *defaultCase = nullptr;
    bool isDefault = false;
    std::vector<std::string> asmConstraints;  // ND_ASM: one per operand in `body`
    int asmNumOut = 0;                        // first asmNumOut operands are outputs
    std::vector<std::string> asmClobbers;
    explicit Node(NodeKind k, Token *t = nullptr) : kind(k), tok(t) {}
};

struct Reloc {
    int offset;
    std::string sym;
    int64_t addend;
};

// `#pragma parameter __D0 Name(__A0, __D1)` register calling convention
struct RegParams {
    bool present = false;
    int retReg = -1;              // 0-7 = d0-d7, 8-15 = a0-a7, -1 = none/void
    std::vector<int> argRegs;
};

struct Obj {
    std::string name;
    std::string asmName;
    Type *ty = nullptr;
    Token *tok = nullptr;
    bool isLocal = false;
    bool isFunction = false;
    bool isDefinition = false;
    bool isStatic = false;        // internal linkage
    bool isExtern = false;
    bool isInline = false;
    bool isTentative = false;
    bool isWeak = false;
    bool isParam = false;
    bool referenced = false;      // emitted only when referenced (static / inline)
    bool noreturn = false;
    bool alwaysInline = false;
    bool noInline = false;
    bool gnuInline = false;       // extern inline: never emit an out-of-line copy
    std::string section;
    std::string unavailable;      // __attribute__((unavailable("...")))
    int align = 0;
    // local variables
    bool addrTaken = false;
    int regAsm = -1;              // register T x __asm__("d0") : physical register
    int vreg = -1;
    int slot = -1;
    // globals
    std::vector<unsigned char> data;
    std::vector<Reloc> relocs;
    bool isBss = true;
    bool isString = false;        // anonymous string literal (mergeable, read-only)
    bool isConstData = false;
    // functions
    std::vector<Obj *> params;
    std::vector<Obj *> locals;
    Node *body = nullptr;
    std::vector<int> inlineWords; // Mac trap glue: f(...) = { 0xA9xx }
    RegParams regParams;
    int usesAlloca = 0;
    bool usesVarargs = false;
};

struct Program {
    std::vector<Obj *> globals;   // in definition order (functions and variables)
    std::vector<std::string> topAsm;
};

Program *parse(Token *tok);
int64_t evalConstExpr(Node *n, const Token *where);

// ---------------------------------------------------------------- IR
enum IRT { I8 = 1, I16 = 2, I32 = 4 };

enum IROp {
    IR_NOP, IR_MOV,
    IR_ADD, IR_SUB, IR_MUL, IR_DIVS, IR_DIVU, IR_MODS, IR_MODU,
    IR_AND, IR_OR, IR_XOR, IR_SHL, IR_SHRS, IR_SHRU, IR_NEG, IR_NOT,
    IR_SEXT, IR_ZEXT, IR_TRUNC,
    IR_LOAD, IR_STORE,
    IR_SETCC,                      // dst = (a cc b) ? 1 : 0
    IR_BR,                         // if (a cc b) goto t1 else goto t2
    IR_JMP, IR_SWITCH, IR_CALL, IR_RET,
    IR_COPYBLK,                    // memory copy  a=dst addr, b=src addr, n bytes
    IR_ZEROBLK,                    // memory clear a=addr, n bytes
    IR_ASM, IR_ALLOCA, IR_GETREG, IR_SETREG, IR_UNREACH
};

enum CC { CC_EQ, CC_NE, CC_LT, CC_LE, CC_GT, CC_GE, CC_ULT, CC_ULE, CC_UGT, CC_UGE };
CC ccNegate(CC c);
CC ccSwap(CC c);

struct Opd {
    enum Kind { NONE, VREG, CONST, SYM, SLOT } kind = NONE;
    int v = 0;                    // vreg id / slot id
    int64_t c = 0;                // constant / offset for SYM,SLOT
    std::string sym;
    static Opd reg(int v) { Opd o; o.kind = VREG; o.v = v; return o; }
    static Opd imm(int64_t c) { Opd o; o.kind = CONST; o.c = c; return o; }
    static Opd symbol(const std::string &s, int64_t off = 0) { Opd o; o.kind = SYM; o.sym = s; o.c = off; return o; }
    static Opd slot(int id, int64_t off = 0) { Opd o; o.kind = SLOT; o.v = id; o.c = off; return o; }
    bool isReg() const { return kind == VREG; }
    bool isConst() const { return kind == CONST; }
    bool isNone() const { return kind == NONE; }
    bool operator==(const Opd &o) const { return kind == o.kind && v == o.v && c == o.c && sym == o.sym; }
};

// How one call argument / return value travels
struct ArgInfo {
    Opd val;
    int size = 4;                 // bytes pushed (1 pushed as 2 for pascal, 4 for C)
    int valSize = 4;              // size of the value itself (1,2,4) or struct size
    bool byMem = false;           // val is the ADDRESS of a block of `valSize` bytes to push
    int reg = -1;                 // register parameter (pragma parameter)
};

struct CallInfo {
    Opd target;                   // SYM or VREG ; NONE when inline words only
    std::vector<ArgInfo> args;    // in source order
    CallConv cc = CC_C;
    bool variadic = false;
    std::vector<int> inlineWords;
    int retReg = 0;               // physical register holding the result (pragma parameter)
    int retSize = 0;              // 0 void, 1/2/4 scalar in register, 8 = d0:d1 pair
    Opd retSlot;                  // 8-byte result is stored here (address operand)
    Opd sret;                     // struct return: address passed in a1
    bool pascalResultOnStack = false;
    int pascalResultSize = 0;
    bool noreturn = false;
    bool regParams = false;
};

struct AsmOpd {
    std::string constraint;       // e.g. "d", "a", "m", "dmi"
    bool isOut = false, isIn = false;
    Opd val;                      // VREG / CONST, or the address of a memory operand
    int64_t off = 0;
    bool isMem = false;
    int size = 4;
};

struct Ins {
    IROp op = IR_NOP;
    IRT ty = I32;                 // operation width (LOAD/STORE: access width)
    IRT srcTy = I32;              // SEXT/ZEXT/TRUNC source width
    int dst = -1;
    Opd a, b;
    int64_t off = 0;              // LOAD/STORE displacement ; COPYBLK/ZEROBLK length
    CC cc = CC_EQ;
    int t1 = -1, t2 = -1;         // branch targets (block ids)
    bool isVolatile = false;
    CallInfo *call = nullptr;
    std::vector<std::pair<int64_t, int>> cases; // IR_SWITCH (value, block), t1 = default
    std::string text;             // IR_ASM
    std::vector<AsmOpd> asmOps;
    std::vector<int> clobbers;    // physical registers
    int reg = -1;                 // GETREG/SETREG physical register
    int line = 0;
};

struct Block {
    int id = 0;
    std::vector<Ins> ins;
    std::vector<int> preds, succs;
    bool reachable = true;
    int loopDepth = 0;
};

struct Slot {
    int size = 0; int align = 2; int offset = 0; bool used = false;
    int argOffset = -1;           // >=0: lives in the incoming argument area (offset from the first argument byte)
};

struct Func {
    Obj *obj = nullptr;
    std::string name;
    std::vector<Block *> blocks;       // layout order; blocks[0] is the entry
    std::vector<IRT> vregTy;
    std::vector<bool> vregPtr;         // hint: holds an address
    std::vector<int> vregFixed;        // physical register for `register T x asm("d0")`, else -1
    std::vector<Slot> slots;
    std::vector<int> paramVregs;       // incoming parameters (vreg or -1)
    bool isPascal = false;
    bool hasAlloca = false;
    bool hasCalls = false;
    int newVreg(IRT t, bool ptr = false) {
        vregTy.push_back(t); vregPtr.push_back(ptr); vregFixed.push_back(-1);
        return (int)vregTy.size() - 1;
    }
    int newSlot(int size, int align) {
        Slot s; s.size = size; s.align = align < 2 ? (size > 1 ? 2 : 1) : align; slots.push_back(s);
        return (int)slots.size() - 1;
    }
    Block *newBlock() { Block *b = new Block; b->id = (int)allBlocks.size(); allBlocks.push_back(b); return b; }
    std::vector<Block *> allBlocks;    // by id
    // incoming parameter description for the backend prologue
    struct ParamIn { int vreg = -1; int slot = -1; int size = 4; int stackOff = 0; int valSize = 4; int reg = -1; };
    std::vector<ParamIn> paramsIn;
    int argBytes = 0;                  // bytes of stack arguments (pascal callee pops)
    int pascalRetSize = 0;
    int sretVreg = -1;                 // hidden struct-return pointer (arrives in a1)
    int retSize = 0;
};

Func *genIR(Obj *fn);
void optimize(Func *f);
std::string dumpIR(Func *f);
void computeCFG(Func *f);

// inlining support: IR of already compiled functions in this translation unit
extern std::map<std::string, Func *> inlineCandidates;

// ---------------------------------------------------------------- backend
void emitFunction(Func *f, std::string &out);
void emitData(Program *prog, std::string &out);
void emitFileHeader(std::string &out);
std::string asmSymbol(const std::string &name);
extern std::set<std::string> referencedSymbols;

} // namespace p68
