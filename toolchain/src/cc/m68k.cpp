// m68k.cpp - 680x0 back end: instruction selection, register allocation, assembly output.
//
//   IR  --select-->  machine instructions over virtual registers
//       --classes--> every vreg is D-only, A-only or either
//       --colour-->  Chaitin/Briggs graph colouring with move-biased selection and spilling
//       --emit---->  GAS-syntax assembly (Motorola operands, % register prefix)
#include "cc.h"

namespace p68 {

int tsize(const Type *t);
int talign(const Type *t);

std::set<std::string> referencedSymbols;

enum { RD0 = 0, RD1 = 1, RD2 = 2, RA0 = 8, RA1 = 9, RA5 = 13, RA6 = 14, RSP = 15, NPHYS = 16 };

struct MOp {
    enum Kind { NONE, REG, IMM, IMMSYM, MEM, ABS, SLOT, PREDEC, POSTINC } kind = NONE;
    int reg = -1;       // REG, MEM base, PREDEC, POSTINC
    int idx = -1;       // MEM index register (long, scale 1)
    int64_t disp = 0;   // IMM value / displacement / offset
    std::string sym;    // IMMSYM, ABS
    int slot = -1;      // SLOT
    bool isReg() const { return kind == REG; }
    bool isMemLike() const { return kind == MEM || kind == ABS || kind == SLOT || kind == PREDEC || kind == POSTINC; }
};
static MOp R(int r) { MOp o; o.kind = MOp::REG; o.reg = r; return o; }
static MOp Imm(int64_t v) { MOp o; o.kind = MOp::IMM; o.disp = v; return o; }
static MOp ImmSym(const std::string &s, int64_t off) { MOp o; o.kind = MOp::IMMSYM; o.sym = s; o.disp = off; return o; }
static MOp Mem(int base, int64_t disp, int idx = -1) { MOp o; o.kind = MOp::MEM; o.reg = base; o.disp = disp; o.idx = idx; return o; }
static MOp Abs(const std::string &s, int64_t off) { MOp o; o.kind = MOp::ABS; o.sym = s; o.disp = off; return o; }
static MOp SlotOp(int slot, int64_t off) { MOp o; o.kind = MOp::SLOT; o.slot = slot; o.disp = off; return o; }
static MOp Push() { MOp o; o.kind = MOp::PREDEC; o.reg = RSP; return o; }
static MOp Pop() { MOp o; o.kind = MOp::POSTINC; o.reg = RSP; return o; }

enum MOpc {
    M_MOVE, M_LEA, M_PEA, M_ADD, M_SUB, M_AND, M_OR, M_EOR, M_CMP, M_TST, M_CLR, M_NEG, M_NOT, M_EXT, M_EXTB,
    M_SWAP, M_MULS, M_MULU, M_DIVS, M_DIVU, M_LSL, M_LSR, M_ASR, M_SCC, M_BCC, M_BRA, M_JSR, M_CALLW, M_RET,
    M_DBRA, M_LABEL, M_LBRA, M_JTAB, M_ASM
};

struct MI {
    MOpc op = M_MOVE;
    char size = 0;                 // 'b' 'w' 'l'
    MOp src, dst;
    CC cc = CC_EQ;
    int target = -1;               // block id (BCC/BRA) or local label number (LABEL/DBRA/LBRA)
    int spAdj = 0;                 // bytes this instruction pushes (+) or pops (-)
    bool partial = false;          // MOVE into the low part of a register whose upper bits are kept
    uint32_t useMask = 0, defMask = 0; // implicit physical register uses / clobbers
    std::vector<int> words;        // CALLW
    std::string text;              // ASM template
    std::vector<MOp> asmOps;
    std::vector<AsmOpd> asmInfo;
    std::vector<std::pair<int64_t, int>> cases; // JTAB: (value-min) table entries -> block id
    int tabDefault = -1;
    int64_t tabCount = 0;
};

struct MBlock {
    int id = 0;
    std::vector<MI> mis;
    std::vector<int> succs;
    int loopDepth = 0;
};

static int sizeBytes(char s) { return s == 'b' ? 1 : s == 'w' ? 2 : 4; }
static char sizeChar(IRT t) { return t == I8 ? 'b' : t == I16 ? 'w' : 'l'; }

// ------------------------------------------------------------ selection state
enum { CLS_ANY = 0, CLS_D = 1, CLS_A = 2 };

struct Sel {
    Func *f = nullptr;
    std::vector<MBlock> blocks;
    MBlock *mb = nullptr;
    std::vector<int> useCount, defCount;
    struct Pend { const Ins *ins; int order; };
    std::map<int, Pend> pending;       // IR vreg -> deferred defining instruction
    int order = 0;
    int localLabel = 0;
    std::vector<char> noSpill;         // per IR vreg
    std::vector<char> forceCls;        // CLS_* forced by inline asm constraints

    int vr(int irVreg) const { return irVreg + NPHYS; }
    int newTemp(bool ptr = false) {
        int v = f->newVreg(I32, ptr);
        useCount.push_back(0);
        defCount.push_back(0);
        noSpill.push_back(0);
        forceCls.push_back(CLS_ANY);
        return v + NPHYS;
    }
    MI &E(MOpc op, char size, MOp src = MOp(), MOp dst = MOp()) {
        MI m;
        m.op = op;
        m.size = size;
        m.src = src;
        m.dst = dst;
        mb->mis.push_back(m);
        return mb->mis.back();
    }
};
static Sel *S;

static void selectIns(const Ins &i);

static bool usesVreg(const Ins &i, int v) {
    if (i.a.isReg() && i.a.v == v) return true;
    if (i.b.isReg() && i.b.v == v) return true;
    return false;
}

static void materialize(int v) {
    auto it = S->pending.find(v);
    if (it == S->pending.end()) return;
    const Ins *ins = it->second.ins;
    S->pending.erase(it);
    selectIns(*ins);
}

// materialise deferred instructions in their original order
template <class Pred> static void flushPending(Pred pred) {
    for (;;) {
        int best = -1, bestOrder = INT32_MAX;
        for (auto &kv : S->pending)
            if (pred(*kv.second.ins) && kv.second.order < bestOrder) { best = kv.first; bestOrder = kv.second.order; }
        if (best < 0) break;
        materialize(best);
    }
}
static void flushLoads() {
    flushPending([](const Ins &i) { return i.op == IR_LOAD; });
}
static void flushAll() {
    flushPending([](const Ins &) { return true; });
}
static void flushUsing(int v) {
    flushPending([v](const Ins &i) { return usesVreg(i, v); });
}

// address operand for [a + off]
static MOp memOperand(Opd a, int64_t off) {
    switch (a.kind) {
    case Opd::SYM: return Abs(a.sym, a.c + off);
    case Opd::SLOT: return SlotOp(a.v, a.c + off);
    case Opd::CONST: return Abs("", (int64_t)(uint32_t)(a.c + off));
    case Opd::VREG: {
        auto it = S->pending.find(a.v);
        if (it != S->pending.end()) {
            const Ins *p = it->second.ins;
            if (p->op == IR_ADD && p->ty == I32 && p->a.isReg() && p->b.isReg() && off >= -128 && off <= 127 &&
                !S->pending.count(p->a.v) && !S->pending.count(p->b.v) && S->f->vregFixed[p->a.v] < 0) {
                S->pending.erase(it);
                return Mem(S->vr(p->a.v), off, S->vr(p->b.v));
            }
            materialize(a.v);
        }
        if (off < -32768 || off > 32767) {
            int t = S->newTemp(true);
            S->E(M_MOVE, 'l', R(S->vr(a.v)), R(t));
            S->E(M_ADD, 'l', Imm(off), R(t));
            return Mem(t, 0);
        }
        return Mem(S->vr(a.v), off);
    }
    default: break;
    }
    fatal("internal: bad address operand");
}

// operand usable as a source. allowMem: may be a memory operand (folded load)
static MOp useOperand(Opd o, char size, bool allowMem, bool allowImm = true) {
    switch (o.kind) {
    case Opd::CONST: {
        if (allowImm) return Imm(o.c);
        int t = S->newTemp();
        S->E(M_MOVE, 'l', Imm(o.c), R(t));
        return R(t);
    }
    case Opd::SYM: {
        if (allowImm && size == 'l') return ImmSym(o.sym, o.c);
        int t = S->newTemp(true);
        S->E(M_MOVE, 'l', ImmSym(o.sym, o.c), R(t));
        return R(t);
    }
    case Opd::SLOT: {
        int t = S->newTemp(true);
        S->E(M_LEA, 'l', SlotOp(o.v, o.c), R(t));
        return R(t);
    }
    case Opd::VREG: {
        auto it = S->pending.find(o.v);
        if (it != S->pending.end()) {
            const Ins *p = it->second.ins;
            if (allowMem && p->op == IR_LOAD && sizeChar(p->ty) == size) {
                Opd a = p->a;
                int64_t off = p->off;
                S->pending.erase(it);
                return memOperand(a, off);
            }
            materialize(o.v);
        }
        return R(S->vr(o.v));
    }
    default: break;
    }
    fatal("internal: bad operand");
}

static bool isDeferrable(const Ins &i) {
    Func *f = S->f;
    if (i.dst < 0 || f->vregFixed[i.dst] >= 0) return false;
    if (S->useCount[i.dst] != 1 || S->defCount[i.dst] != 1) return false;
    auto fixedOp = [&](const Opd &o) { return o.isReg() && f->vregFixed[o.v] >= 0; };
    if (fixedOp(i.a) || fixedOp(i.b)) return false;
    switch (i.op) {
    case IR_LOAD: return !i.isVolatile;
    case IR_ADD: case IR_SUB: case IR_AND: case IR_OR: case IR_XOR: case IR_NEG: case IR_NOT:
        return true;
    default: return false;
    }
}

static const char *ccName(CC c) {
    switch (c) {
    case CC_EQ: return "eq";
    case CC_NE: return "ne";
    case CC_LT: return "lt";
    case CC_LE: return "le";
    case CC_GT: return "gt";
    case CC_GE: return "ge";
    case CC_ULT: return "cs";
    case CC_ULE: return "ls";
    case CC_UGT: return "hi";
    case CC_UGE: return "cc";
    }
    return "eq";
}

// Emit a comparison for (a cc b). Returns the condition to test afterwards; *always is set
// to 0/1 when the outcome is known statically.
static CC emitCompare(CC cc, IRT ty, Opd a, Opd b, int *known) {
    *known = -1;
    char sz = sizeChar(ty);
    if (b.isConst() && b.c == 0) {
        // unsigned comparisons against zero degenerate
        if (cc == CC_ULT) { *known = 0; return cc; }
        if (cc == CC_UGE) { *known = 1; return cc; }
        if (cc == CC_UGT) cc = CC_NE;
        if (cc == CC_ULE) cc = CC_EQ;
        MOp x = useOperand(a, sz, true, false);
        S->E(M_TST, sz, MOp(), x);
        return cc;
    }
    bool aMem = a.isReg() && S->pending.count(a.v) && S->pending[a.v].ins->op == IR_LOAD;
    if (b.isConst() || b.kind == Opd::SYM) {
        MOp x = useOperand(a, sz, true, false);
        MOp y = b.isConst() ? Imm(b.c) : ImmSym(b.sym, b.c);
        S->E(M_CMP, sz, y, x);
        return cc;
    }
    if (aMem && b.isReg() && !S->pending.count(b.v)) {
        // cmp mem,reg computes reg - mem: swap the sense
        MOp x = useOperand(a, sz, true, false);
        MOp y = useOperand(b, sz, false, false);
        S->E(M_CMP, sz, x, y);
        return ccSwap(cc);
    }
    MOp x = useOperand(a, sz, false, false);
    MOp y = useOperand(b, sz, true, false);
    if (x.kind != MOp::REG) fatal("internal: compare destination is not a register");
    S->E(M_CMP, sz, y, x);
    return cc;
}

static void emitCopyLoop(MOp srcA, MOp dstA, int64_t n, bool aligned, bool zero) {
    // srcA/dstA are address registers (vregs) that this sequence may advance
    auto post = [](const MOp &r) { MOp o; o.kind = MOp::POSTINC; o.reg = r.reg; return o; };
    int unit = aligned ? 4 : 1;
    char usz = aligned ? 'l' : 'b';
    int64_t units = n / unit;
    auto one = [&](char sz) {
        if (zero) S->E(M_CLR, sz, MOp(), post(dstA));
        else S->E(M_MOVE, sz, post(srcA), post(dstA));
    };
    if (units <= (opt.optSize ? 4 : 8)) {
        for (int64_t k = 0; k < units; k++) one(usz);
    } else {
        int cnt = S->newTemp();
        int lbl = S->localLabel++;
        int64_t iters = units;
        if (iters - 1 <= 32767) {
            S->E(M_MOVE, 'l', Imm(iters - 1), R(cnt));
            MI &l = S->E(M_LABEL, 0);
            l.target = lbl;
            one(usz);
            MI &d = S->E(M_DBRA, 'w', MOp(), R(cnt));
            d.target = lbl;
        } else {
            S->E(M_MOVE, 'l', Imm(iters), R(cnt));
            MI &l = S->E(M_LABEL, 0);
            l.target = lbl;
            one(usz);
            S->E(M_SUB, 'l', Imm(1), R(cnt));
            MI &bne = S->E(M_LBRA, 0);
            bne.cc = CC_NE;
            bne.target = lbl;
        }
    }
    if (aligned) {
        if (n & 2) one('w');
        if (n & 1) one('b');
    }
}

static void emitCall(const Ins &ins) {
    CallInfo *ci = ins.call;
    Func *f = S->f;
    bool trap = !ci->inlineWords.empty();
    // 1. evaluate argument operands (may fold loads)
    struct A { MOp op; const ArgInfo *ai; Opd addr; bool isAddr = false; };
    std::vector<A> args;
    for (auto &ai : ci->args) {
        A a;
        a.ai = &ai;
        if (ai.byMem) {
            a.addr = ai.val;
            if (ai.val.isReg()) materialize(ai.val.v);
        } else if (ai.reg >= 0) {
            a.op = useOperand(ai.val, 'l', true);
        } else if (ai.val.kind == Opd::SLOT) {
            a.op = SlotOp(ai.val.v, ai.val.c); // pushed with PEA
            a.isAddr = true;
        } else {
            char sz = ai.valSize == 1 ? 'b' : ai.valSize == 2 ? 'w' : 'l';
            a.op = useOperand(ai.val, sz, true);
        }
        args.push_back(a);
    }
    MOp target;
    if (!trap) {
        if (ci->target.kind == Opd::SYM) target = Abs(ci->target.sym, ci->target.c);
        else if (ci->target.isReg()) {
            materialize(ci->target.v);
            target = Mem(S->vr(ci->target.v), 0);
        } else if (ci->target.isConst()) target = Abs("", ci->target.c);
        else fatal("internal: bad call target");
    }
    flushLoads();
    // 2. pascal result space
    int pushed = 0;
    if (ci->pascalResultOnStack) {
        MI &m = S->E(M_SUB, 'l', Imm(ci->pascalResultSize), R(RSP));
        m.spAdj = ci->pascalResultSize;
    }
    // 3. push / load arguments
    uint32_t useMask = 0;
    auto pushArg = [&](A &a) {
        const ArgInfo &ai = *a.ai;
        if (ai.byMem) {
            if (ai.valSize == 8 || ai.valSize == 4 || ai.valSize == 2) {
                // push the block high address first so it ends up in memory order
                for (int off = ai.valSize; off > 0;) {
                    int chunk = (off >= 4) ? 4 : 2;
                    off -= chunk;
                    MI &m = S->E(M_MOVE, chunk == 4 ? 'l' : 'w', memOperand(a.addr, off), Push());
                    m.spAdj = chunk;
                }
                int pad = ai.size - ai.valSize;
                (void)pad;
                pushed += ai.size;
            } else {
                // general aggregate: reserve space and copy
                MI &m = S->E(M_SUB, 'l', Imm(ai.size), R(RSP));
                m.spAdj = ai.size;
                int d = S->newTemp(true), s = S->newTemp(true);
                S->E(M_MOVE, 'l', R(RSP), R(d));
                MOp sa = memOperand(a.addr, 0);
                S->E(M_LEA, 'l', sa, R(s));
                emitCopyLoop(R(s), R(d), ai.valSize, false, false);
                pushed += ai.size;
            }
            return;
        }
        if (a.isAddr) {
            MI &m = S->E(M_PEA, 'l', a.op);
            m.spAdj = 4;
            pushed += 4;
            return;
        }
        char sz = ai.size == 4 ? 'l' : (ai.valSize == 1 ? 'b' : 'w');
        MI &m = S->E(M_MOVE, sz, a.op, Push());
        m.spAdj = ai.size == 4 ? 4 : 2;
        pushed += m.spAdj;
    };
    if (ci->regParams) {
        for (auto &a : args) {
            S->E(M_MOVE, 'l', a.op, R(a.ai->reg));
            useMask |= 1u << a.ai->reg;
        }
    } else if (ci->cc == CC_PASCAL) {
        for (size_t k = 0; k < args.size(); k++) pushArg(args[k]);
    } else {
        for (size_t k = args.size(); k-- > 0;) pushArg(args[k]);
    }
    if (!ci->sret.isNone()) {
        S->E(M_LEA, 'l', memOperand(ci->sret, 0), R(RA1));
        useMask |= 1u << RA1;
    }
    // 4. the call itself
    uint32_t clob = (1u << RD0) | (1u << RD1) | (1u << RA0) | (1u << RA1);
    if (trap || ci->cc == CC_PASCAL || ci->regParams) clob |= 1u << RD2;
    if (trap) {
        MI &m = S->E(M_CALLW, 0);
        m.words = ci->inlineWords;
        m.useMask = useMask;
        m.defMask = clob;
    } else {
        MI &m = S->E(M_JSR, 0, target);
        m.useMask = useMask;
        m.defMask = clob;
    }
    if (ci->cc == CC_PASCAL && !ci->regParams) S->mb->mis.back().spAdj = -pushed; // the callee pops its arguments
    // 5. clean up and fetch the result
    if (ci->cc == CC_PASCAL && !ci->regParams) {
        if (ci->pascalResultOnStack) {
            char sz = ci->retSize == 1 ? 'b' : ci->retSize == 2 ? 'w' : 'l';
            if (ins.dst >= 0) {
                MI &m = S->E(M_MOVE, sz, Pop(), R(S->vr(ins.dst)));
                m.spAdj = -ci->pascalResultSize;
            } else {
                MI &m = S->E(M_ADD, 'l', Imm(ci->pascalResultSize), R(RSP));
                m.spAdj = -ci->pascalResultSize;
            }
        }
        return;
    }
    if (pushed && !ci->regParams) {
        MI &m = S->E(M_ADD, 'l', Imm(pushed), R(RSP));
        m.spAdj = -pushed;
    }
    if (ci->retSize == 8) {
        S->E(M_MOVE, 'l', R(RD0), memOperand(ci->retSlot, 0));
        S->E(M_MOVE, 'l', R(RD1), memOperand(ci->retSlot, 4));
    } else if (ins.dst >= 0) {
        int rr = ci->regParams ? ci->retReg : RD0;
        S->E(M_MOVE, 'l', R(rr), R(S->vr(ins.dst)));
    }
    (void)f;
}

// is the vreg known to hold a 16-bit value extended to 32 bits? returns 1 signed, 2 unsigned
static int ext16Kind(Opd o, const std::vector<const Ins *> &defs) {
    if (o.isConst()) return (o.c >= -32768 && o.c <= 32767) ? 3 : (o.c >= 0 && o.c <= 65535 ? 2 : 0);
    if (!o.isReg()) return 0;
    const Ins *d = defs[o.v];
    if (!d) return 0;
    if (d->op == IR_SEXT && d->srcTy <= I16) return 1;
    if (d->op == IR_ZEXT && d->srcTy == I8) return 3;
    if (d->op == IR_ZEXT && d->srcTy == I16) return 2;
    if (d->op == IR_AND && d->ty == I32 && d->b.isConst() && d->b.c >= 0 && d->b.c <= 0xFFFF) return d->b.c <= 0x7FFF ? 3 : 2;
    if (d->op == IR_SHRU && d->ty == I32 && d->b.isConst() && d->b.c >= 16 && d->b.c < 32) return d->b.c > 16 ? 3 : 2;
    return 0;
}
static std::vector<const Ins *> singleDefs;

static void emitHelperCall(const char *name, Opd a, Opd b, int dst) {
    MOp y = useOperand(b, 'l', true);
    MOp x = useOperand(a, 'l', true);
    flushLoads();
    MI &p1 = S->E(M_MOVE, 'l', y, Push());
    p1.spAdj = 4;
    MI &p2 = S->E(M_MOVE, 'l', x, Push());
    p2.spAdj = 4;
    MI &j = S->E(M_JSR, 0, Abs(name, 0));
    j.defMask = (1u << RD0) | (1u << RD1) | (1u << RA0) | (1u << RA1);
    MI &pop = S->E(M_ADD, 'l', Imm(8), R(RSP));
    pop.spAdj = -8;
    S->E(M_MOVE, 'l', R(RD0), R(dst));
    referencedSymbols.insert(name);
    S->f->hasCalls = true;
}

// dst = a OP b for two-address instructions
static void emitBinary(MOpc op, char sz, const Ins &i, bool commutative, bool srcMemOk) {
    int d = S->vr(i.dst);
    Opd a = i.a, b = i.b;
    if (commutative && b.isReg() && b.v == i.dst && !(a.isReg() && a.v == i.dst)) std::swap(a, b);
    if (a.isReg() && a.v == i.dst) {
        MOp y = useOperand(b, sz, srcMemOk);
        S->E(op, sz, y, R(d));
        return;
    }
    if (b.isReg() && b.v == i.dst) {
        // d = a - d  (non-commutative with the destination as right operand)
        int t = S->newTemp();
        MOp x = useOperand(a, sz, true);
        S->E(M_MOVE, 'l', x.kind == MOp::REG ? x : x, R(t));
        if (x.kind != MOp::REG && sz != 'l') { S->mb->mis.back().size = sz; }
        S->E(op, sz, R(d), R(t));
        S->E(M_MOVE, 'l', R(t), R(d));
        return;
    }
    MOp x = useOperand(a, sz, true);
    MOp y = useOperand(b, sz, srcMemOk);
    S->E(M_MOVE, x.kind == MOp::REG || x.kind == MOp::IMM || x.kind == MOp::IMMSYM ? 'l' : sz, x, R(d));
    S->E(op, sz, y, R(d));
}

static void selectIns(const Ins &i) {
    Func *f = S->f;
    char sz = sizeChar(i.ty);
    int d = i.dst >= 0 ? S->vr(i.dst) : -1;
    switch (i.op) {
    case IR_NOP: case IR_UNREACH:
        return;
    case IR_MOV: {
        if (i.a.kind == Opd::SLOT) { S->E(M_LEA, 'l', SlotOp(i.a.v, i.a.c), R(d)); return; }
        MOp x = useOperand(i.a, sz, true);
        char msz = (x.kind == MOp::REG || x.kind == MOp::IMM || x.kind == MOp::IMMSYM) ? 'l' : sz;
        S->E(M_MOVE, msz, x, R(d));
        return;
    }
    case IR_ADD: {
        // pointer-style three-operand add through LEA
        bool ptr = i.ty == I32 && f->vregPtr[i.dst] && i.a.isReg() && i.a.v != i.dst && f->vregPtr[i.a.v] && f->vregFixed[i.a.v] < 0;
        if (ptr && i.b.isConst() && i.b.c >= -32768 && i.b.c <= 32767) {
            materialize(i.a.v);
            S->E(M_LEA, 'l', Mem(S->vr(i.a.v), i.b.c), R(d));
            return;
        }
        if (ptr && i.b.isReg() && i.b.v != i.dst && f->vregFixed[i.b.v] < 0) {
            materialize(i.a.v);
            materialize(i.b.v);
            S->E(M_LEA, 'l', Mem(S->vr(i.a.v), 0, S->vr(i.b.v)), R(d));
            return;
        }
        emitBinary(M_ADD, sz, i, true, true);
        return;
    }
    case IR_SUB: emitBinary(M_SUB, sz, i, false, true); return;
    case IR_AND: emitBinary(M_AND, sz, i, true, true); return;
    case IR_OR: emitBinary(M_OR, sz, i, true, true); return;
    case IR_XOR: emitBinary(M_EOR, sz, i, true, false); return;
    case IR_NEG: case IR_NOT: {
        MOp x = useOperand(i.a, sz, true);
        if (!(x.kind == MOp::REG && x.reg == d)) S->E(M_MOVE, x.kind == MOp::REG ? 'l' : sz, x, R(d));
        S->E(i.op == IR_NEG ? M_NEG : M_NOT, sz, MOp(), R(d));
        return;
    }
    case IR_MUL: {
        if (i.ty != I32) { emitBinary(M_MULS, 'w', i, true, true); return; }
        if (opt.cpu >= 68020) { emitBinary(M_MULS, 'l', i, true, true); return; }
        int ka = ext16Kind(i.a, singleDefs), kb = ext16Kind(i.b, singleDefs);
        if (ka && kb && ((ka & 1) && (kb & 1))) { emitBinary(M_MULS, 'w', i, true, true); return; }
        if (ka && kb && ((ka & 2) && (kb & 2))) { emitBinary(M_MULU, 'w', i, true, true); return; }
        emitHelperCall("__mulsi3", i.a, i.b, d);
        return;
    }
    case IR_DIVS: case IR_DIVU: case IR_MODS: case IR_MODU: {
        bool sgn = i.op == IR_DIVS || i.op == IR_MODS;
        bool mod = i.op == IR_MODS || i.op == IR_MODU;
        int ka = ext16Kind(i.a, singleDefs), kb = ext16Kind(i.b, singleDefs);
        bool narrow = i.ty != I32 ? false : (sgn ? ((ka & 1) && (kb & 1)) : ((ka & 2) && (kb & 2)));
        if (i.b.isConst() && i.b.c == 0) narrow = false;
        if (sgn && i.b.isConst() && i.b.c == -1) narrow = false;
        if (i.ty == I32 && narrow) {
            // 32/16 -> 16 hardware divide; operands are known to fit
            MOp x = useOperand(i.a, 'l', true);
            MOp y = useOperand(i.b, 'w', true);
            int t = S->newTemp();
            if (y.kind == MOp::REG && y.reg == d) {
                S->E(M_MOVE, 'l', y, R(t));
                y = R(t);
            }
            if (!(x.kind == MOp::REG && x.reg == d)) S->E(M_MOVE, 'l', x, R(d));
            S->E(sgn ? M_DIVS : M_DIVU, 'w', y, R(d));
            if (mod) S->E(M_SWAP, 'w', MOp(), R(d));
            if (sgn) S->E(M_EXT, 'l', MOp(), R(d));
            else S->E(M_AND, 'l', Imm(0xFFFF), R(d));
            return;
        }
        if (i.ty == I32 && opt.cpu >= 68020 && !mod) {
            emitBinary(sgn ? M_DIVS : M_DIVU, 'l', i, false, true);
            return;
        }
        if (i.ty != I32) {
            // 8/16-bit operands: widen and use the hardware divide
            int ta = S->newTemp(), tb = S->newTemp();
            MOp x = useOperand(i.a, sz, true), y = useOperand(i.b, sz, true);
            if (sgn) {
                S->E(M_MOVE, sz, x, R(ta));
                if (sz == 'b') S->E(M_EXT, 'w', MOp(), R(ta));
                S->E(M_EXT, 'l', MOp(), R(ta));
                S->E(M_MOVE, sz, y, R(tb));
                if (sz == 'b') S->E(M_EXT, 'w', MOp(), R(tb));
            } else {
                S->E(M_MOVE, 'l', Imm(0), R(ta));
                MI &m = S->E(M_MOVE, sz, x, R(ta));
                m.partial = true;
                S->E(M_MOVE, 'l', Imm(0), R(tb));
                MI &m2 = S->E(M_MOVE, sz, y, R(tb));
                m2.partial = true;
            }
            S->E(sgn ? M_DIVS : M_DIVU, 'w', R(tb), R(ta));
            if (mod) S->E(M_SWAP, 'w', MOp(), R(ta));
            S->E(M_MOVE, 'l', R(ta), R(d));
            return;
        }
        const char *name = i.op == IR_DIVS ? "__divsi3" : i.op == IR_DIVU ? "__udivsi3" : i.op == IR_MODS ? "__modsi3" : "__umodsi3";
        emitHelperCall(name, i.a, i.b, d);
        return;
    }
    case IR_SHL: case IR_SHRS: case IR_SHRU: {
        MOpc op = i.op == IR_SHL ? M_LSL : i.op == IR_SHRS ? M_ASR : M_LSR;
        if (i.b.isConst()) {
            int64_t n = i.b.c & 63;
            MOp x = useOperand(i.a, sz, true);
            if (!(x.kind == MOp::REG && x.reg == d)) S->E(M_MOVE, x.kind == MOp::REG ? 'l' : sz, x, R(d));
            if (n == 0) return;
            if (i.ty == I32 && n >= 16 && n < 32) {
                if (i.op == IR_SHL) {
                    S->E(M_SWAP, 'w', MOp(), R(d));
                    S->E(M_CLR, 'w', MOp(), R(d));
                } else if (i.op == IR_SHRU) {
                    S->E(M_CLR, 'w', MOp(), R(d));
                    S->E(M_SWAP, 'w', MOp(), R(d));
                } else {
                    S->E(M_SWAP, 'w', MOp(), R(d));
                    S->E(M_EXT, 'l', MOp(), R(d));
                }
                n -= 16;
                if (n == 0) return;
                if (n <= 8) { S->E(op, i.op == IR_SHL ? 'l' : (i.op == IR_SHRU ? 'w' : 'l'), Imm(n), R(d)); return; }
                // remaining 9..15
                S->E(op, 'l', Imm(8), R(d));
                S->E(op, 'l', Imm(n - 8), R(d));
                return;
            }
            if (n <= 8) { S->E(op, sz, Imm(n), R(d)); return; }
            if (n <= 16 && opt.optSize == false) {
                S->E(op, sz, Imm(8), R(d));
                S->E(op, sz, Imm(n - 8), R(d));
                return;
            }
            int t = S->newTemp();
            S->E(M_MOVE, 'l', Imm(n), R(t));
            S->E(op, sz, R(t), R(d));
            return;
        }
        // variable count
        MOp cnt = useOperand(i.b, 'l', false, false);
        MOp x = useOperand(i.a, sz, true);
        if (cnt.kind == MOp::REG && cnt.reg == d) {
            int t = S->newTemp();
            S->E(M_MOVE, 'l', cnt, R(t));
            cnt = R(t);
        }
        if (!(x.kind == MOp::REG && x.reg == d)) S->E(M_MOVE, x.kind == MOp::REG ? 'l' : sz, x, R(d));
        S->E(op, sz, cnt, R(d));
        return;
    }
    case IR_SEXT: {
        char ssz = sizeChar(i.srcTy);
        MOp x = useOperand(i.a, ssz, true);
        if (!(x.kind == MOp::REG && x.reg == d)) S->E(M_MOVE, x.kind == MOp::REG ? 'l' : ssz, x, R(d));
        if (i.srcTy == I8 && i.ty == I32 && opt.cpu >= 68020) { S->E(M_EXTB, 'l', MOp(), R(d)); return; }
        if (i.srcTy == I8) S->E(M_EXT, 'w', MOp(), R(d));
        if (i.ty == I32) S->E(M_EXT, 'l', MOp(), R(d));
        return;
    }
    case IR_ZEXT: {
        char ssz = sizeChar(i.srcTy);
        MOp x = useOperand(i.a, ssz, true);
        if (x.kind == MOp::REG && x.reg == d) {
            S->E(M_AND, sz, Imm(i.srcTy == I8 ? 0xFF : 0xFFFF), R(d));
            return;
        }
        S->E(M_MOVE, 'l', Imm(0), R(d));
        MI &m = S->E(M_MOVE, ssz, x, R(d));
        m.partial = true;
        return;
    }
    case IR_TRUNC: {
        // the low part of a register is the truncated value; from memory read the low bytes
        if (i.a.isReg()) {
            auto it = S->pending.find(i.a.v);
            if (it != S->pending.end() && it->second.ins->op == IR_LOAD) {
                const Ins *p = it->second.ins;
                Opd a = p->a;
                int64_t off = p->off + ((int)p->ty - (int)i.ty);
                S->pending.erase(it);
                S->E(M_MOVE, sz, memOperand(a, off), R(d));
                return;
            }
        }
        MOp x = useOperand(i.a, 'l', false);
        S->E(M_MOVE, 'l', x, R(d));
        return;
    }
    case IR_LOAD: {
        MOp ea = memOperand(i.a, i.off);
        S->E(M_MOVE, sz, ea, R(d));
        return;
    }
    case IR_STORE: {
        // read-modify-write:  [ea] = [ea] op x
        if (i.b.isReg() && !i.isVolatile) {
            auto it = S->pending.find(i.b.v);
            if (it != S->pending.end()) {
                const Ins *p = it->second.ins;
                bool unary = p->op == IR_NEG || p->op == IR_NOT;
                bool binop = p->op == IR_ADD || p->op == IR_SUB || p->op == IR_AND || p->op == IR_OR || p->op == IR_XOR;
                if ((unary || binop) && p->ty == i.ty && p->a.isReg()) {
                    auto il = S->pending.find(p->a.v);
                    if (il != S->pending.end()) {
                        const Ins *ld = il->second.ins;
                        bool sameAddr = ld->op == IR_LOAD && ld->ty == i.ty && ld->a == i.a && ld->off == i.off;
                        bool rhsOk = unary || p->b.isConst() || (p->b.isReg() && !S->pending.count(p->b.v) && p->b.v != p->a.v);
                        if (sameAddr && rhsOk) {
                            Ins pc = *p;
                            S->pending.erase(i.b.v);
                            S->pending.erase(pc.a.v);
                            MOp ea = memOperand(i.a, i.off);
                            flushLoads();
                            if (unary) S->E(pc.op == IR_NEG ? M_NEG : M_NOT, sz, MOp(), ea);
                            else {
                                MOpc op = pc.op == IR_ADD ? M_ADD : pc.op == IR_SUB ? M_SUB : pc.op == IR_AND ? M_AND : pc.op == IR_OR ? M_OR : M_EOR;
                                MOp y = pc.b.isConst() ? Imm(pc.b.c) : R(S->vr(pc.b.v));
                                S->E(op, sz, y, ea);
                            }
                            return;
                        }
                    }
                }
            }
        }
        MOp val = useOperand(i.b, sz, true);
        MOp ea = memOperand(i.a, i.off);
        flushLoads();
        if (val.kind == MOp::IMM && val.disp == 0 && !i.isVolatile) S->E(M_CLR, sz, MOp(), ea);
        else S->E(M_MOVE, sz, val, ea);
        return;
    }
    case IR_SETCC: {
        int known;
        CC cc = emitCompare(i.cc, i.ty, i.a, i.b, &known);
        IRT dt = f->vregTy[i.dst];
        if (known >= 0) { S->E(M_MOVE, 'l', Imm(known), R(d)); return; }
        MI &s = S->E(M_SCC, 'b', MOp(), R(d));
        s.cc = cc;
        S->E(M_NEG, 'b', MOp(), R(d));
        if (dt != I8) S->E(M_EXT, 'w', MOp(), R(d));
        if (dt == I32) S->E(M_EXT, 'l', MOp(), R(d));
        return;
    }
    case IR_CALL:
        emitCall(i);
        return;
    case IR_COPYBLK: case IR_ZEROBLK: {
        bool zero = i.op == IR_ZEROBLK;
        bool aligned = i.ty != I8;
        int64_t n = i.off;
        flushLoads();
        // small aligned blocks: direct moves with displacements
        if (aligned && n <= (opt.optSize ? 8 : 16)) {
            auto ea = [&](Opd a, int64_t off) { return memOperand(a, off); };
            // addresses in vregs must not be consumed as pending adds more than once: materialise
            if (i.a.isReg()) materialize(i.a.v);
            if (!zero && i.b.isReg()) materialize(i.b.v);
            int64_t off = 0;
            while (n - off >= 4) {
                if (zero) S->E(M_CLR, 'l', MOp(), ea(i.a, off));
                else S->E(M_MOVE, 'l', ea(i.b, off), ea(i.a, off));
                off += 4;
            }
            if (n - off >= 2) {
                if (zero) S->E(M_CLR, 'w', MOp(), ea(i.a, off));
                else S->E(M_MOVE, 'w', ea(i.b, off), ea(i.a, off));
                off += 2;
            }
            if (n - off >= 1) {
                if (zero) S->E(M_CLR, 'b', MOp(), ea(i.a, off));
                else S->E(M_MOVE, 'b', ea(i.b, off), ea(i.a, off));
            }
            return;
        }
        int da = S->newTemp(true), sa = -1;
        if (i.a.isReg()) { materialize(i.a.v); S->E(M_MOVE, 'l', R(S->vr(i.a.v)), R(da)); }
        else S->E(M_LEA, 'l', memOperand(i.a, 0), R(da));
        if (!zero) {
            sa = S->newTemp(true);
            if (i.b.isReg()) { materialize(i.b.v); S->E(M_MOVE, 'l', R(S->vr(i.b.v)), R(sa)); }
            else S->E(M_LEA, 'l', memOperand(i.b, 0), R(sa));
        }
        emitCopyLoop(zero ? MOp() : R(sa), R(da), n, aligned, zero);
        return;
    }
    case IR_GETREG:
        S->E(M_MOVE, 'l', R(i.reg), R(d));
        return;
    case IR_SETREG: {
        MOp x = useOperand(i.a, 'l', true);
        S->E(M_MOVE, 'l', x, R(i.reg));
        return;
    }
    case IR_ASM: {
        flushAll();
        MI m;
        m.op = M_ASM;
        m.text = i.text;
        m.asmInfo = i.asmOps;
        for (auto &o : i.asmOps) {
            if (o.isMem) m.asmOps.push_back(memOperand(o.val, o.off));
            else if (o.val.isConst()) m.asmOps.push_back(Imm(o.val.c));
            else if (o.val.isReg()) {
                m.asmOps.push_back(R(S->vr(o.val.v)));
                bool wantD = o.constraint.find('d') != std::string::npos;
                bool wantA = o.constraint.find('a') != std::string::npos;
                if (wantD && !wantA) S->forceCls[o.val.v] = CLS_D;
                else if (wantA && !wantD) S->forceCls[o.val.v] = CLS_A;
            } else
                fatal("internal: bad asm operand");
        }
        for (int r : i.clobbers) m.defMask |= 1u << r;
        m.defMask &= ~(1u << RSP);
        S->mb->mis.push_back(m);
        return;
    }
    case IR_RET: {
        uint32_t use = 0;
        if (i.off == 8) {
            if (i.a.isReg()) materialize(i.a.v);
            MOp hi = memOperand(i.a, 0), lo = memOperand(i.a, 4);
            flushLoads();
            S->E(M_MOVE, 'l', lo, R(RD1));
            S->E(M_MOVE, 'l', hi, R(RD0));
            use = 3;
        } else if (!i.a.isNone()) {
            MOp x = useOperand(i.a, 'l', true);
            flushAll();
            S->E(M_MOVE, 'l', x, R(RD0));
            use = 1;
        }
        flushAll();
        MI &m = S->E(M_RET, 0);
        m.useMask = use;
        return;
    }
    case IR_JMP: {
        flushAll();
        MI &m = S->E(M_BRA, 0);
        m.target = i.t1;
        return;
    }
    case IR_BR: {
        int known;
        CC cc = emitCompare(i.cc, i.ty, i.a, i.b, &known);
        // nothing that changes the flags may follow the compare: pending loads were already
        // consumed or materialised by emitCompare's operands; flush the rest BEFORE the compare
        // is not possible here, so make sure nothing is pending
        if (!S->pending.empty()) {
            // re-emit: move the compare after the flush
            std::vector<MI> tail;
            // find the compare we just emitted (last instruction when known < 0)
            if (known < 0) {
                tail.push_back(S->mb->mis.back());
                S->mb->mis.pop_back();
            }
            flushAll();
            for (auto &t : tail) S->mb->mis.push_back(t);
        }
        if (known >= 0) {
            MI &m = S->E(M_BRA, 0);
            m.target = known ? i.t1 : i.t2;
            return;
        }
        MI &m = S->E(M_BCC, 0);
        m.cc = cc;
        m.target = i.t1;
        MI &j = S->E(M_BRA, 0);
        j.target = i.t2;
        return;
    }
    case IR_SWITCH: {
        MOp x = useOperand(i.a, 'l', false, false);
        flushAll();
        std::vector<std::pair<int64_t, int>> cs = i.cases;
        std::sort(cs.begin(), cs.end());
        int64_t lo = cs.front().first, hi = cs.back().first;
        int64_t range = hi - lo + 1;
        if (cs.size() >= 5 && range <= (int64_t)cs.size() * 3 && range < 4000) {
            int t = S->newTemp();
            S->E(M_MOVE, 'l', x, R(t));
            if (lo) S->E(M_SUB, 'l', Imm(lo), R(t));
            S->E(M_CMP, 'l', Imm(range - 1), R(t));
            MI &b = S->E(M_BCC, 0);
            b.cc = CC_UGT;
            b.target = i.t1;
            MI &jt = S->E(M_JTAB, 0, MOp(), R(t));
            jt.tabDefault = i.t1;
            jt.tabCount = range;
            for (auto &c : cs) jt.cases.push_back({c.first - lo, c.second});
            return;
        }
        // compare chain (binary search for larger sets)
        std::function<void(size_t, size_t)> chain = [&](size_t a, size_t b) {
            if (b - a <= 4) {
                for (size_t k = a; k < b; k++) {
                    if (cs[k].first == 0) S->E(M_TST, 'l', MOp(), x);
                    else S->E(M_CMP, 'l', Imm(cs[k].first), x);
                    MI &m = S->E(M_BCC, 0);
                    m.cc = CC_EQ;
                    m.target = cs[k].second;
                }
                MI &j = S->E(M_BRA, 0);
                j.target = i.t1;
                return;
            }
            size_t mid = (a + b) / 2;
            int lbl = S->localLabel++;
            S->E(M_CMP, 'l', Imm(cs[mid].first), x);
            MI &m = S->E(M_LBRA, 0);
            m.cc = CC_GE;
            m.target = lbl;
            chain(a, mid);
            MI &l = S->E(M_LABEL, 0);
            l.target = lbl;
            chain(mid, b);
        };
        chain(0, cs.size());
        return;
    }
    case IR_ALLOCA:
        fatal("%s: alloca() is not supported by this compiler yet", f->name.c_str());
    default:
        fatal("internal: unhandled IR op %d", (int)i.op);
    }
}

// ------------------------------------------------------------ register references
struct RegRef {
    int *reg;
    bool use, def;
    int cls;       // required class at this position (CLS_ANY/D/A)
    int where;     // 0 src operand, 1 dst operand, 2 base/index/other
};

static void memRefs(MOp &o, std::vector<RegRef> &out) {
    if (o.kind == MOp::MEM) {
        if (o.reg >= 0) out.push_back({&o.reg, true, false, CLS_A, 2});
        if (o.idx >= 0) out.push_back({&o.idx, true, false, CLS_ANY, 2});
    } else if (o.kind == MOp::PREDEC || o.kind == MOp::POSTINC) {
        out.push_back({&o.reg, true, true, CLS_A, 2});
    }
}

static void regRefs(MI &m, std::vector<RegRef> &out) {
    out.clear();
    memRefs(m.src, out);
    memRefs(m.dst, out);
    bool notLong = m.size == 'b' || m.size == 'w';
    int sizeCls = notLong ? CLS_D : CLS_ANY;
    auto S_ = [&](bool use, bool def, int cls) { if (m.src.kind == MOp::REG) out.push_back({&m.src.reg, use, def, cls, 0}); };
    auto D_ = [&](bool use, bool def, int cls) { if (m.dst.kind == MOp::REG) out.push_back({&m.dst.reg, use, def, cls, 1}); };
    switch (m.op) {
    case M_MOVE:
        S_(true, false, sizeCls);
        D_(m.partial, true, sizeCls);
        break;
    case M_LEA: D_(false, true, CLS_A); break;
    case M_PEA: break;
    case M_ADD: case M_SUB:
        S_(true, false, (m.dst.kind != MOp::REG) ? CLS_D : sizeCls);
        D_(true, true, sizeCls);
        break;
    case M_AND: case M_OR: case M_EOR:
        S_(true, false, CLS_D);
        D_(true, true, CLS_D);
        break;
    case M_CMP:
        S_(true, false, sizeCls);
        D_(true, false, sizeCls);
        break;
    case M_TST: D_(true, false, sizeCls); break;
    case M_CLR: D_(false, true, CLS_D); break;
    case M_NEG: case M_NOT: case M_EXT: case M_EXTB: case M_SWAP:
        D_(true, true, CLS_D);
        break;
    case M_MULS: case M_MULU: case M_DIVS: case M_DIVU: case M_LSL: case M_LSR: case M_ASR:
        S_(true, false, CLS_D);
        D_(true, true, CLS_D);
        break;
    case M_SCC: D_(false, true, CLS_D); break;
    case M_DBRA: case M_JTAB: D_(true, true, CLS_D); break;
    case M_ASM:
        for (size_t k = 0; k < m.asmOps.size(); k++) {
            MOp &o = m.asmOps[k];
            memRefs(o, out);
            if (o.kind == MOp::REG) {
                const AsmOpd &info = m.asmInfo[k];
                bool wantD = info.constraint.find('d') != std::string::npos;
                bool wantA = info.constraint.find('a') != std::string::npos;
                out.push_back({&o.reg, info.isIn, info.isOut, wantD && !wantA ? CLS_D : wantA && !wantD ? CLS_A : CLS_ANY, 2});
            }
        }
        break;
    default: break;
    }
}

// can the operand (`where` 0 = src, 1 = dst) be replaced by a stack slot?
static bool memOk(const MI &m, int where, const std::vector<int> &cls) {
    auto isDreg = [&](const MOp &o) {
        if (o.kind != MOp::REG) return false;
        if (o.reg < NPHYS) return o.reg < 8;
        return cls[o.reg - NPHYS] == CLS_D;
    };
    switch (m.op) {
    case M_MOVE: return !m.partial || where == 0;
    case M_ADD: case M_SUB: case M_AND: case M_OR:
        if (where == 0) return m.dst.kind == MOp::REG;
        return m.src.kind == MOp::IMM || isDreg(m.src);
    case M_EOR: return where == 1 && (m.src.kind == MOp::IMM || isDreg(m.src));
    case M_CMP:
        if (where == 0) return m.dst.kind == MOp::REG;
        return m.src.kind == MOp::IMM;
    case M_TST: case M_CLR: case M_NEG: case M_NOT: case M_SCC: return where == 1;
    case M_MULS: case M_MULU: case M_DIVS: case M_DIVU: return where == 0;
    default: return false;
    }
}

// ------------------------------------------------------------ register allocation
struct RA {
    Sel *s;
    Func *f;
    int n = 0;                              // nodes: phys + vregs
    std::vector<std::vector<uint32_t>> adjBits;
    std::vector<std::vector<int>> adj;
    std::vector<int> color;
    std::vector<int> cls;                   // per vreg index (node - NPHYS)
    std::vector<double> cost;
    std::vector<std::vector<int>> movePartners;
    std::vector<int> spillSlot;             // per vreg
    uint32_t usedCalleeSaved = 0;
};

static bool isCalleeSaved(int r) { return (r >= 2 && r <= 7) || (r >= 10 && r <= 14); }

static uint32_t allowedMask(int cls) {
    uint32_t d = 0x00FF;
    uint32_t a = 0x1F00 | (1u << RA6);       // a0-a4, a6
    if (!opt.fixedA5) a |= 1u << RA5;
    return cls == CLS_D ? d : cls == CLS_A ? a : (d | a);
}

static void addEdge(RA &ra, int a, int b) {
    if (a == b || a == RSP || b == RSP) return;
    if (a < NPHYS && b < NPHYS) return;
    if (ra.adjBits[a][b >> 5] >> (b & 31) & 1) return;
    ra.adjBits[a][b >> 5] |= 1u << (b & 31);
    ra.adjBits[b][a >> 5] |= 1u << (a & 31);
    ra.adj[a].push_back(b);
    ra.adj[b].push_back(a);
}

struct LiveSet {
    std::vector<uint32_t> w;
    void init(int n) { w.assign((n + 31) / 32, 0); }
    bool get(int i) const { return w[i >> 5] >> (i & 31) & 1; }
    void set(int i) { w[i >> 5] |= 1u << (i & 31); }
    void clr(int i) { w[i >> 5] &= ~(1u << (i & 31)); }
};

static void determineClasses(Sel &s, std::vector<int> &cls) {
    Func *f = s.f;
    size_t nv = f->vregTy.size();
    cls.assign(nv, CLS_ANY);
    std::vector<char> needD(nv, 0), needA(nv, 0);
    std::vector<RegRef> refs;
    for (auto &b : s.blocks)
        for (auto &m : b.mis) {
            regRefs(m, refs);
            for (auto &r : refs) {
                if (*r.reg < NPHYS) continue;
                int v = *r.reg - NPHYS;
                if (r.cls == CLS_D) needD[v] = 1;
                else if (r.cls == CLS_A) needA[v] = 1;
            }
        }
    for (size_t v = 0; v < nv; v++) {
        if (f->vregFixed[v] >= 0) { cls[v] = f->vregFixed[v] < 8 ? CLS_D : CLS_A; continue; }
        if (s.forceCls[v] != CLS_ANY) cls[v] = s.forceCls[v];
        else if (needD[v]) cls[v] = CLS_D;
        else if (needA[v]) cls[v] = CLS_A;
    }
    // positions whose class requirement conflicts with the vreg's class get a copy
    for (auto &b : s.blocks) {
        for (size_t k = 0; k < b.mis.size(); k++) {
            regRefs(b.mis[k], refs);
            std::vector<MI> before, after;
            for (auto &r : refs) {
                if (*r.reg < NPHYS) {
                    // physical register in the wrong class position (e.g. d0 as a base): copy
                    bool isA = *r.reg >= 8;
                    if ((r.cls == CLS_A && !isA) || (r.cls == CLS_D && isA)) {
                        int t = s.newTemp(r.cls == CLS_A);
                        cls.push_back(r.cls);
                        s.noSpill[t - NPHYS] = 1;
                        MI mv;
                        mv.op = M_MOVE;
                        mv.size = 'l';
                        if (r.use) { mv.src = R(*r.reg); mv.dst = R(t); before.push_back(mv); }
                        if (r.def) { mv.src = R(t); mv.dst = R(*r.reg); after.push_back(mv); }
                        *r.reg = t;
                    }
                    continue;
                }
                int v = *r.reg - NPHYS;
                if (r.cls == CLS_ANY || cls[v] == r.cls || cls[v] == CLS_ANY) continue;
                int t = s.newTemp(r.cls == CLS_A);
                cls.push_back(r.cls);
                s.noSpill[t - NPHYS] = 1;
                MI mv;
                mv.op = M_MOVE;
                mv.size = 'l';
                if (r.use) { mv.src = R(*r.reg); mv.dst = R(t); before.push_back(mv); }
                if (r.def) { mv.src = R(t); mv.dst = R(*r.reg); after.push_back(mv); }
                // replace every reference to this vreg in the instruction with the temp
                int old = *r.reg;
                for (auto &r2 : refs)
                    if (*r2.reg == old && r2.cls == r.cls) *r2.reg = t;
            }
            if (!before.empty() || !after.empty()) {
                b.mis.insert(b.mis.begin() + k + 1, after.begin(), after.end());
                b.mis.insert(b.mis.begin() + k, before.begin(), before.end());
                k += before.size() + after.size();
            }
        }
    }
    cls.resize(f->vregTy.size(), CLS_ANY);
}

// returns true when colouring succeeded without spills
static bool colorOnce(RA &ra) {
    Sel &s = *ra.s;
    Func *f = ra.f;
    int n = NPHYS + (int)f->vregTy.size();
    ra.n = n;
    int words = (n + 31) / 32;
    ra.adjBits.assign(n, std::vector<uint32_t>(words, 0));
    ra.adj.assign(n, {});
    ra.cost.assign(n, 0);
    ra.movePartners.assign(n, {});
    // ---- liveness over machine blocks
    size_t nb = s.blocks.size();
    std::map<int, int> blockIndex;
    for (size_t k = 0; k < nb; k++) blockIndex[s.blocks[k].id] = (int)k;
    std::vector<LiveSet> use(nb), def(nb), in(nb), out(nb);
    std::vector<RegRef> refs;
    for (size_t k = 0; k < nb; k++) {
        use[k].init(n);
        def[k].init(n);
        in[k].init(n);
        out[k].init(n);
        for (auto &m : s.blocks[k].mis) {
            regRefs(m, refs);
            for (auto &r : refs)
                if (r.use && !def[k].get(*r.reg)) use[k].set(*r.reg);
            for (int p = 0; p < NPHYS; p++)
                if ((m.useMask >> p & 1) && !def[k].get(p)) use[k].set(p);
            for (auto &r : refs)
                if (r.def) def[k].set(*r.reg);
            for (int p = 0; p < NPHYS; p++)
                if (m.defMask >> p & 1) def[k].set(p);
        }
    }
    for (bool changed = true; changed;) {
        changed = false;
        for (size_t k = nb; k-- > 0;) {
            for (int sid : s.blocks[k].succs) {
                int si = blockIndex[sid];
                for (int w = 0; w < words; w++) out[k].w[w] |= in[si].w[w];
            }
            for (int w = 0; w < words; w++) {
                uint32_t nw = use[k].w[w] | (out[k].w[w] & ~def[k].w[w]);
                if (nw != in[k].w[w]) { in[k].w[w] = nw; changed = true; }
            }
        }
    }
    // ---- interference graph
    for (size_t k = 0; k < nb; k++) {
        LiveSet live = out[k];
        double weight = 1;
        for (int d = 0; d < s.blocks[k].loopDepth && d < 6; d++) weight *= 8;
        auto &mis = s.blocks[k].mis;
        for (size_t idx = mis.size(); idx-- > 0;) {
            MI &m = mis[idx];
            regRefs(m, refs);
            bool isMove = m.op == M_MOVE && m.src.kind == MOp::REG && m.dst.kind == MOp::REG && !m.partial;
            if (isMove) {
                ra.movePartners[m.src.reg].push_back(m.dst.reg);
                ra.movePartners[m.dst.reg].push_back(m.src.reg);
            }
            // definitions interfere with everything live after the instruction
            auto defNode = [&](int dnode) {
                for (int w = 0; w < words; w++) {
                    uint32_t bits = live.w[w];
                    while (bits) {
                        int bit = 0;
                        while (!(bits >> bit & 1)) bit++;
                        bits &= bits - 1;
                        int l = w * 32 + bit;
                        if (isMove && l == m.src.reg) continue;
                        addEdge(ra, dnode, l);
                    }
                }
            };
            for (auto &r : refs)
                if (r.def) defNode(*r.reg);
            for (int p = 0; p < NPHYS; p++)
                if (m.defMask >> p & 1) defNode(p);
            // several definitions in one instruction interfere with each other
            for (auto &r : refs)
                if (r.def)
                    for (auto &r2 : refs)
                        if (r2.def && *r2.reg != *r.reg) addEdge(ra, *r.reg, *r2.reg);
            for (auto &r : refs)
                if (r.def) live.clr(*r.reg);
            for (int p = 0; p < NPHYS; p++)
                if (m.defMask >> p & 1) live.clr(p);
            for (auto &r : refs) {
                if (r.use) live.set(*r.reg);
                ra.cost[*r.reg] += weight;
            }
            for (int p = 0; p < NPHYS; p++)
                if (m.useMask >> p & 1) live.set(p);
        }
    }
    // ---- simplify / select
    ra.color.assign(n, -1);
    for (int p = 0; p < NPHYS; p++) ra.color[p] = p;
    std::vector<int> degree(n, 0);
    std::vector<char> removed(n, 0);
    std::vector<int> stack;
    std::vector<int> nodes;
    for (int v = NPHYS; v < n; v++) {
        int fx = f->vregFixed[v - NPHYS];
        if (fx >= 0) { ra.color[v] = fx; continue; }
        if (ra.adj[v].empty() && ra.cost[v] == 0) continue; // unused
        nodes.push_back(v);
        degree[v] = (int)ra.adj[v].size();
    }
    auto kOf = [&](int v) {
        uint32_t m = allowedMask(ra.cls[v - NPHYS]);
        int k = 0;
        for (int p = 0; p < NPHYS; p++) k += m >> p & 1;
        return k;
    };
    size_t remaining = nodes.size();
    while (remaining) {
        int pick = -1;
        for (int v : nodes)
            if (!removed[v] && degree[v] < kOf(v)) { pick = v; break; }
        if (pick < 0) {
            // optimistic: remove the cheapest node per degree
            double best = 1e300;
            for (int v : nodes) {
                if (removed[v]) continue;
                double c = s.noSpill[v - NPHYS] ? 1e200 : ra.cost[v] / (degree[v] + 1);
                if (c < best) { best = c; pick = v; }
            }
        }
        removed[pick] = 1;
        stack.push_back(pick);
        remaining--;
        for (int nb2 : ra.adj[pick])
            if (nb2 >= NPHYS && !removed[nb2]) degree[nb2]--;
    }
    static const int orderD[] = {0, 1, 2, 3, 4, 5, 6, 7};
    static const int orderA[] = {8, 9, 10, 11, 12, 14, 13};
    static const int orderAny[] = {0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 14, 13};
    std::vector<int> spilled;
    while (!stack.empty()) {
        int v = stack.back();
        stack.pop_back();
        uint32_t avail = allowedMask(ra.cls[v - NPHYS]);
        for (int nb2 : ra.adj[v])
            if (ra.color[nb2] >= 0) avail &= ~(1u << ra.color[nb2]);
        if (!avail) { spilled.push_back(v); continue; }
        int c = -1;
        // prefer the colour of a move partner
        for (int p : ra.movePartners[v])
            if (ra.color[p] >= 0 && (avail >> ra.color[p] & 1)) { c = ra.color[p]; break; }
        if (c < 0) {
            // prefer a colour that a not-yet-coloured partner could also take: the partner's partners
            const int *order = ra.cls[v - NPHYS] == CLS_D ? orderD : ra.cls[v - NPHYS] == CLS_A ? orderA : orderAny;
            int cnt = ra.cls[v - NPHYS] == CLS_D ? 8 : ra.cls[v - NPHYS] == CLS_A ? 7 : 15;
            for (int k = 0; k < cnt; k++)
                if (avail >> order[k] & 1) { c = order[k]; break; }
        }
        ra.color[v] = c;
    }
    if (spilled.empty()) return true;
    // ---- spill: rewrite the code
    if ((int)ra.spillSlot.size() < n) ra.spillSlot.resize(n, -1);
    std::set<int> sp(spilled.begin(), spilled.end());
    for (int v : spilled) {
        if (s.noSpill[v - NPHYS]) fatal("%s: register allocation failed (out of registers for a spill temporary)", f->name.c_str());
        ra.spillSlot[v] = f->newSlot(4, 2);
    }
    for (auto &b : s.blocks) {
        for (size_t k = 0; k < b.mis.size(); k++) {
            MI &m = b.mis[k];
            regRefs(m, refs);
            bool any = false;
            for (auto &r : refs)
                if (sp.count(*r.reg)) any = true;
            if (!any) continue;
            std::vector<MI> before, after;
            // try memory operands for plain register positions
            for (int where = 0; where < 2; where++) {
                MOp &o = where == 0 ? m.src : m.dst;
                if (o.kind != MOp::REG || !sp.count(o.reg)) continue;
                // the same vreg used in both positions needs a register
                MOp &other = where == 0 ? m.dst : m.src;
                bool otherSame = other.kind == MOp::REG && other.reg == o.reg;
                bool otherMem = other.isMemLike();
                bool okMem = memOk(m, where, ra.cls) && !otherSame;
                if (otherMem && m.op != M_MOVE) okMem = false;
                if (okMem) {
                    int slot = ra.spillSlot[o.reg];
                    o = SlotOp(slot, 4 - sizeBytes(m.size ? m.size : 'l'));
                }
            }
            regRefs(m, refs);
            std::map<int, int> tempFor;
            for (auto &r : refs) {
                if (!sp.count(*r.reg)) continue;
                int old = *r.reg;
                int t;
                auto it = tempFor.find(old);
                if (it != tempFor.end()) t = it->second;
                else {
                    t = s.newTemp();
                    ra.cls.push_back(r.cls != CLS_ANY ? r.cls : ra.cls[old - NPHYS]);
                    s.noSpill[t - NPHYS] = 1;
                    tempFor[old] = t;
                    bool u = false, d = false;
                    for (auto &r2 : refs)
                        if (*r2.reg == old) { u = u || r2.use; d = d || r2.def; }
                    MI mv;
                    mv.op = M_MOVE;
                    mv.size = 'l';
                    if (u) { mv.src = SlotOp(ra.spillSlot[old], 0); mv.dst = R(t); before.push_back(mv); }
                    if (d) { mv.src = R(t); mv.dst = SlotOp(ra.spillSlot[old], 0); after.push_back(mv); }
                }
                // a class-A position on a D-class temp (or vice versa) is impossible: the temp takes the position's class
                if (r.cls != CLS_ANY) ra.cls[t - NPHYS] = r.cls;
                *r.reg = t;
            }
            // values popped from / pushed to the stack keep their sp adjustment on the original instruction
            b.mis.insert(b.mis.begin() + k + 1, after.begin(), after.end());
            b.mis.insert(b.mis.begin() + k, before.begin(), before.end());
            k += before.size() + after.size();
        }
    }
    ra.cls.resize(f->vregTy.size(), CLS_ANY);
    return false;
}

// ------------------------------------------------------------ assembly output
static int funcCounter = 0;

std::string asmSymbol(const std::string &name) { return name; }

static const char *regName(int r) {
    static const char *names[] = {"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
                                  "%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp"};
    if (r < 0 || r >= NPHYS) fatal("internal: unallocated register %d in output", r);
    return names[r];
}

struct Emit {
    Func *f;
    RA *ra;
    std::string *out;
    int frameSize = 0;
    int savedBytes = 0;
    int depth = 0;
    int fn = 0;
    std::string symOff(const std::string &sym, int64_t off) {
        if (sym.empty()) return format("%lld", (long long)off);
        if (off == 0) return sym;
        return format("%s%+lld", sym.c_str(), (long long)off);
    }
    std::string opd(const MOp &o) {
        switch (o.kind) {
        case MOp::REG: return regName(o.reg);
        case MOp::IMM: return format("#%lld", (long long)o.disp);
        case MOp::IMMSYM: return "#" + symOff(o.sym, o.disp);
        case MOp::MEM:
            if (o.idx >= 0) return format("%lld(%s,%s.l)", (long long)o.disp, regName(o.reg), regName(o.idx));
            if (o.disp == 0) return format("(%s)", regName(o.reg));
            return format("%lld(%s)", (long long)o.disp, regName(o.reg));
        case MOp::ABS:
            if (o.sym.empty()) {
                uint32_t a = (uint32_t)o.disp;
                if (a < 0x8000 || a >= 0xFFFF8000u) return format("0x%X.w", a & 0xFFFF);
                return format("0x%X", a);
            }
            return symOff(o.sym, o.disp);
        case MOp::SLOT: {
            const Slot &sl = f->slots[o.slot];
            int64_t off = o.disp + depth;
            if (sl.argOffset >= 0) off += frameSize + savedBytes + 4 + sl.argOffset;
            else off += sl.offset;
            if (off == 0) return "(%sp)";
            return format("%lld(%%sp)", (long long)off);
        }
        case MOp::PREDEC: return format("-(%s)", regName(o.reg));
        case MOp::POSTINC: return format("(%s)+", regName(o.reg));
        default: return "?";
        }
    }
    void line(const std::string &s) { *out += "\t" + s + "\n"; }
    std::string blockLabel(int id) { return format(".L%d_%d", fn, id); }
    std::string localLabel(int id) { return format(".Ll%d_%d", fn, id); }
};

static bool sameOp(const MOp &a, const MOp &b) {
    return a.kind == b.kind && a.reg == b.reg && a.idx == b.idx && a.disp == b.disp && a.sym == b.sym && a.slot == b.slot;
}
static bool opUsesReg(const MOp &o, int r) {
    return ((o.kind == MOp::REG || o.kind == MOp::MEM || o.kind == MOp::PREDEC || o.kind == MOp::POSTINC) && o.reg == r) ||
           (o.kind == MOp::MEM && o.idx == r);
}

// post-allocation peephole over one block
static void peephole(Sel &s, RA &ra) {
    std::vector<RegRef> refs;
    for (auto &b : s.blocks) {
        // map vregs to colours
        for (auto &m : b.mis) {
            regRefs(m, refs);
            for (auto &r : refs)
                if (*r.reg >= NPHYS) {
                    int c = ra.color[*r.reg];
                    if (c < 0) fatal("internal: vreg without a register");
                    *r.reg = c;
                }
        }
        // remove no-op moves and redundant tests
        std::vector<MI> outv;
        MOp flagsOf;      // operand whose value the N/Z flags currently describe
        char flagsSize = 0;
        bool flagsFull = false; // V and C are also as TST would leave them
        for (size_t k = 0; k < b.mis.size(); k++) {
            MI &m = b.mis[k];
            if (m.op == M_MOVE && m.src.kind == MOp::REG && m.dst.kind == MOp::REG && m.src.reg == m.dst.reg && !m.spAdj) continue;
            if (m.op == M_TST && flagsSize == m.size && flagsOf.kind != MOp::NONE && sameOp(flagsOf, m.dst)) {
                bool ok = flagsFull;
                if (!ok && k + 1 < b.mis.size()) {
                    const MI &nx = b.mis[k + 1];
                    if ((nx.op == M_BCC || nx.op == M_SCC || nx.op == M_LBRA) && (nx.cc == CC_EQ || nx.cc == CC_NE)) {
                        // the flags must be dead after that branch: only equality is preserved by add/sub
                        ok = nx.op == M_BCC && k + 2 < b.mis.size() && b.mis[k + 2].op == M_BRA;
                    }
                }
                if (ok) continue;
            }
            // track flags
            bool setsFlags = true;
            MOp newFlags;
            char newSize = 0;
            bool full = false;
            switch (m.op) {
            case M_MOVE:
                if (m.dst.kind == MOp::REG && m.dst.reg >= 8) { setsFlags = false; break; } // movea
                if (m.dst.kind == MOp::PREDEC || m.dst.kind == MOp::POSTINC) { newFlags = m.src; }
                else newFlags = m.dst;
                if (m.partial) newFlags = MOp();
                newSize = m.size;
                full = true;
                // moveq/whole-register immediates describe the register as a long
                break;
            case M_TST: newFlags = m.dst; newSize = m.size; full = true; break;
            case M_AND: case M_OR: case M_EOR: case M_NOT: case M_EXT: case M_EXTB:
                newFlags = m.dst; newSize = m.size; full = true; break;
            case M_ADD: case M_SUB: case M_NEG:
                if (m.dst.kind == MOp::REG && m.dst.reg >= 8) { setsFlags = false; break; } // adda/suba
                newFlags = m.dst; newSize = m.size; full = false; break;
            case M_LEA: case M_PEA: case M_LABEL:
                setsFlags = false;
                break;
            default: break;
            }
            if (m.op == M_LABEL) { flagsOf = MOp(); flagsSize = 0; }
            if (setsFlags) {
                flagsOf = newFlags;
                flagsSize = newSize;
                flagsFull = full;
                if (flagsOf.kind == MOp::IMM || flagsOf.kind == MOp::IMMSYM || flagsOf.kind == MOp::PREDEC || flagsOf.kind == MOp::POSTINC) flagsOf = MOp();
            } else {
                // an instruction that leaves the flags alone may still change the tracked operand
                regRefs(m, refs);
                for (auto &r : refs)
                    if (r.def && flagsOf.kind != MOp::NONE && opUsesReg(flagsOf, *r.reg)) flagsOf = MOp();
            }
            // a memory write may change a tracked memory operand
            if (flagsOf.kind != MOp::NONE && flagsOf.kind != MOp::REG && m.dst.isMemLike() && !sameOp(m.dst, flagsOf)) flagsOf = MOp();
            if (m.defMask) {
                for (int p = 0; p < NPHYS; p++)
                    if ((m.defMask >> p & 1) && flagsOf.kind != MOp::NONE && opUsesReg(flagsOf, p)) flagsOf = MOp();
                if (flagsOf.kind != MOp::REG) flagsOf = MOp();
            }
            if (m.op == M_JSR || m.op == M_CALLW || m.op == M_ASM || m.op == M_DBRA || m.op == M_JTAB || m.op == M_SCC || m.op == M_CMP ||
                m.op == M_CLR || m.op == M_MULS || m.op == M_MULU || m.op == M_DIVS || m.op == M_DIVU || m.op == M_LSL || m.op == M_LSR ||
                m.op == M_ASR || m.op == M_SWAP)
                flagsOf = MOp();
            // a register redefinition of the tracked operand's base invalidates it
            if (flagsOf.kind == MOp::MEM || flagsOf.kind == MOp::REG) {
                regRefs(m, refs);
                for (auto &r : refs)
                    if (r.def && !(m.dst.kind == MOp::REG && r.reg == &m.dst.reg && sameOp(flagsOf, m.dst)) && opUsesReg(flagsOf, *r.reg)) flagsOf = MOp();
            }
            outv.push_back(m);
        }
        b.mis.swap(outv);
    }
}

// combine  move (aN),x ; addq #size,aN  into  move (aN)+,x   (and the pre-decrement dual)
static void formAutoInc(Sel &s) {
    std::vector<RegRef> refs;
    for (auto &b : s.blocks) {
        auto &mis = b.mis;
        for (size_t k = 0; k + 1 < mis.size(); k++) {
            MI &m = mis[k];
            if (m.op != M_MOVE && m.op != M_ADD && m.op != M_SUB && m.op != M_CMP && m.op != M_TST && m.op != M_CLR && m.op != M_AND && m.op != M_OR) continue;
            int sz = sizeBytes(m.size ? m.size : 'l');
            for (int which = 0; which < 2; which++) {
                MOp &o = which == 0 ? m.src : m.dst;
                if (o.kind != MOp::MEM || o.idx >= 0 || o.disp != 0 || o.reg < NPHYS) continue;
                int r = o.reg;
                // the register must not appear elsewhere in this instruction
                MOp &other = which == 0 ? m.dst : m.src;
                if (opUsesReg(other, r)) continue;
                MI &nx = mis[k + 1];
                if (nx.op == M_ADD && nx.size == 'l' && nx.src.kind == MOp::IMM && nx.src.disp == sz && nx.dst.kind == MOp::REG && nx.dst.reg == r) {
                    o.kind = MOp::POSTINC;
                    mis.erase(mis.begin() + k + 1);
                    break;
                }
            }
        }
        for (size_t k = 1; k < mis.size(); k++) {
            MI &m = mis[k];
            if (m.op != M_MOVE && m.op != M_CLR && m.op != M_TST) continue;
            int sz = sizeBytes(m.size ? m.size : 'l');
            for (int which = 0; which < 2; which++) {
                MOp &o = which == 0 ? m.src : m.dst;
                if (o.kind != MOp::MEM || o.idx >= 0 || o.disp != 0 || o.reg < NPHYS) continue;
                int r = o.reg;
                MOp &other = which == 0 ? m.dst : m.src;
                if (opUsesReg(other, r)) continue;
                MI &pv = mis[k - 1];
                bool subImm = pv.op == M_SUB && pv.src.kind == MOp::IMM && pv.src.disp == sz;
                bool addNeg = pv.op == M_ADD && pv.src.kind == MOp::IMM && pv.src.disp == -sz;
                if ((subImm || addNeg) && pv.size == 'l' && pv.dst.kind == MOp::REG && pv.dst.reg == r) {
                    o.kind = MOp::PREDEC;
                    mis.erase(mis.begin() + k - 1);
                    k--;
                    break;
                }
            }
        }
    }
}

static std::string movemList(uint32_t mask) {
    std::string s;
    for (int r = 0; r < 15; r++)
        if (mask >> r & 1) {
            if (!s.empty()) s += "/";
            s += regName(r);
        }
    return s;
}

static void emitEpilogue(Emit &e, uint32_t saved) {
    Func *f = e.f;
    if (e.frameSize) {
        if (e.frameSize <= 8) e.line(format("addq.l #%d,%%sp", e.frameSize));
        else e.line(format("lea %d(%%sp),%%sp", e.frameSize));
    }
    int cnt = 0;
    for (int r = 0; r < 15; r++) cnt += saved >> r & 1;
    if (cnt == 1) {
        for (int r = 0; r < 15; r++)
            if (saved >> r & 1) e.line(format("move.l (%%sp)+,%s", regName(r)));
    } else if (cnt > 1)
        e.line("movem.l (%sp)+," + movemList(saved));
    if (f->isPascal && f->argBytes) {
        e.line("move.l (%sp)+,%a0");
        if (f->argBytes <= 8) e.line(format("addq.l #%d,%%sp", f->argBytes));
        else e.line(format("lea %d(%%sp),%%sp", f->argBytes));
        e.line("jmp (%a0)");
    } else
        e.line("rts");
}

static const char *sfx(char s) { return s == 'b' ? ".b" : s == 'w' ? ".w" : ".l"; }

static void emitMI(Emit &e, MI &m, int nextBlockId, const std::string &retLabel, bool sharedRet, bool lastRet, uint32_t saved) {
    auto O = [&](const MOp &o) { return e.opd(o); };
    switch (m.op) {
    case M_MOVE: {
        const MOp &s = m.src, &d = m.dst;
        if (s.kind == MOp::IMM) {
            int64_t v = s.disp;
            if (d.kind == MOp::REG && d.reg < 8) {
                int64_t sv = m.size == 'b' ? (int8_t)v : m.size == 'w' ? (int16_t)v : (int32_t)v;
                if (sv >= -128 && sv <= 127) { e.line(format("moveq #%lld,%s", (long long)sv, regName(d.reg))); break; }
                e.line(format("move%s #%lld,%s", sfx(m.size), (long long)sv, regName(d.reg)));
                break;
            }
            if (d.kind == MOp::REG) { // address register
                int32_t sv = (int32_t)v;
                if (sv == 0) e.line(format("suba.l %s,%s", regName(d.reg), regName(d.reg)));
                else if (sv >= -32768 && sv <= 32767) e.line(format("move.w #%d,%s", sv, regName(d.reg)));
                else e.line(format("move.l #%d,%s", sv, regName(d.reg)));
                break;
            }
            if (d.kind == MOp::PREDEC && d.reg == RSP && m.size == 'l') {
                int32_t sv = (int32_t)v;
                if (sv == 0) e.line("clr.l -(%sp)");
                else if (sv >= -32768 && sv <= 32767) e.line(format("pea %d.w", sv));
                else e.line(format("move.l #%d,-(%%sp)", sv));
                break;
            }
            int64_t sv = m.size == 'b' ? (int8_t)v : m.size == 'w' ? (int16_t)v : (int32_t)v;
            e.line(format("move%s #%lld,%s", sfx(m.size), (long long)sv, O(d).c_str()));
            break;
        }
        if (s.kind == MOp::IMMSYM) {
            if (d.kind == MOp::REG && d.reg >= 8) { e.line(format("lea %s,%s", e.symOff(s.sym, s.disp).c_str(), regName(d.reg))); break; }
            if (d.kind == MOp::PREDEC && d.reg == RSP) { e.line(format("pea %s", e.symOff(s.sym, s.disp).c_str())); break; }
            e.line(format("move.l %s,%s", O(s).c_str(), O(d).c_str()));
            break;
        }
        // byte/word moves to an address register do not exist as byte ops; isel never emits them
        e.line(format("move%s %s,%s", sfx(m.size), O(s).c_str(), O(d).c_str()));
        break;
    }
    case M_LEA: e.line(format("lea %s,%s", O(m.src).c_str(), O(m.dst).c_str())); break;
    case M_PEA: e.line(format("pea %s", O(m.src).c_str())); break;
    case M_ADD: case M_SUB: {
        bool add = m.op == M_ADD;
        if (m.src.kind == MOp::IMM) {
            int64_t v = m.size == 'b' ? (int8_t)m.src.disp : m.size == 'w' ? (int16_t)m.src.disp : (int32_t)m.src.disp;
            if (v == 0) break;
            if (v < 0 && v >= -8) { add = !add; v = -v; }
            if (v >= 1 && v <= 8) { e.line(format("%s%s #%lld,%s", add ? "addq" : "subq", sfx(m.size), (long long)v, O(m.dst).c_str())); break; }
            if (m.dst.kind == MOp::REG && m.dst.reg >= 8) {
                int64_t d = add ? v : -v;
                if (d >= -32768 && d <= 32767) { e.line(format("lea %lld(%s),%s", (long long)d, regName(m.dst.reg), regName(m.dst.reg))); break; }
            }
            e.line(format("%s%s #%lld,%s", add ? "add" : "sub", sfx(m.size), (long long)v, O(m.dst).c_str()));
            break;
        }
        e.line(format("%s%s %s,%s", add ? "add" : "sub", sfx(m.size), O(m.src).c_str(), O(m.dst).c_str()));
        break;
    }
    case M_AND: case M_OR: case M_EOR: {
        const char *n = m.op == M_AND ? "and" : m.op == M_OR ? "or" : "eor";
        if (m.src.kind == MOp::IMM) {
            int64_t v = m.size == 'b' ? (uint8_t)m.src.disp : m.size == 'w' ? (uint16_t)m.src.disp : (int64_t)(uint32_t)m.src.disp;
            e.line(format("%s%s #0x%llX,%s", n, sfx(m.size), (unsigned long long)v, O(m.dst).c_str()));
            break;
        }
        e.line(format("%s%s %s,%s", n, sfx(m.size), O(m.src).c_str(), O(m.dst).c_str()));
        break;
    }
    case M_CMP: {
        if (m.src.kind == MOp::IMM) {
            int64_t v = m.size == 'b' ? (int8_t)m.src.disp : m.size == 'w' ? (int16_t)m.src.disp : (int32_t)m.src.disp;
            e.line(format("cmp%s #%lld,%s", sfx(m.size), (long long)v, O(m.dst).c_str()));
            break;
        }
        e.line(format("cmp%s %s,%s", sfx(m.size), O(m.src).c_str(), O(m.dst).c_str()));
        break;
    }
    case M_TST:
        if (m.dst.kind == MOp::REG && m.dst.reg >= 8 && opt.cpu < 68020) e.line(format("cmp.w #0,%s", regName(m.dst.reg)));
        else e.line(format("tst%s %s", sfx(m.size), O(m.dst).c_str()));
        break;
    case M_CLR: e.line(format("clr%s %s", sfx(m.size), O(m.dst).c_str())); break;
    case M_NEG: e.line(format("neg%s %s", sfx(m.size), O(m.dst).c_str())); break;
    case M_NOT: e.line(format("not%s %s", sfx(m.size), O(m.dst).c_str())); break;
    case M_EXT: e.line(format("ext%s %s", sfx(m.size), O(m.dst).c_str())); break;
    case M_EXTB: e.line(format("extb.l %s", O(m.dst).c_str())); break;
    case M_SWAP: e.line(format("swap %s", O(m.dst).c_str())); break;
    case M_MULS: case M_MULU: case M_DIVS: case M_DIVU: {
        const char *n = m.op == M_MULS ? "muls" : m.op == M_MULU ? "mulu" : m.op == M_DIVS ? "divs" : "divu";
        e.line(format("%s%s %s,%s", n, sfx(m.size), O(m.src).c_str(), O(m.dst).c_str()));
        break;
    }
    case M_LSL: case M_LSR: case M_ASR: {
        const char *n = m.op == M_LSL ? "lsl" : m.op == M_LSR ? "lsr" : "asr";
        if (m.op == M_LSL && m.src.kind == MOp::IMM && m.src.disp == 1) { e.line(format("add%s %s,%s", sfx(m.size), O(m.dst).c_str(), O(m.dst).c_str())); break; }
        e.line(format("%s%s %s,%s", n, sfx(m.size), O(m.src).c_str(), O(m.dst).c_str()));
        break;
    }
    case M_SCC: e.line(format("s%s %s", ccName(m.cc), O(m.dst).c_str())); break;
    case M_BCC: e.line(format("j%s %s", ccName(m.cc), e.blockLabel(m.target).c_str())); break;
    case M_BRA:
        if (m.target != nextBlockId) e.line(format("jra %s", e.blockLabel(m.target).c_str()));
        break;
    case M_LBRA: e.line(format("j%s %s", ccName(m.cc), e.localLabel(m.target).c_str())); break;
    case M_LABEL: *e.out += e.localLabel(m.target) + ":\n"; break;
    case M_DBRA: e.line(format("dbra %s,%s", O(m.dst).c_str(), e.localLabel(m.target).c_str())); break;
    case M_JSR:
        if (m.src.kind == MOp::ABS && !m.src.sym.empty()) e.line(format("jsr %s", e.symOff(m.src.sym, m.src.disp).c_str()));
        else e.line(format("jsr %s", O(m.src).c_str()));
        break;
    case M_CALLW: {
        std::string s = ".short ";
        for (size_t k = 0; k < m.words.size(); k++) s += format("%s0x%04X", k ? "," : "", m.words[k] & 0xFFFF);
        e.line(s);
        break;
    }
    case M_RET:
        if (sharedRet && !lastRet) e.line("jra " + retLabel);
        else {
            if (sharedRet) *e.out += retLabel + ":\n";
            emitEpilogue(e, saved);
        }
        break;
    case M_JTAB: {
        std::string tab = format(".LT%d_%d", e.fn, e.f->obj ? (int)(size_t)(&m - (MI *)nullptr) % 1000000 : 0);
        static int tabCounter = 0;
        tab = format(".LT%d", tabCounter++);
        const char *r = regName(m.dst.reg);
        e.line(format("add.w %s,%s", r, r));
        e.line(format("move.w %s(%%pc,%s.w),%s", tab.c_str(), r, r));
        e.line(format("jmp %s(%%pc,%s.w)", tab.c_str(), r));
        *e.out += tab + ":\n";
        std::vector<int> targets((size_t)m.tabCount, m.tabDefault);
        for (auto &c : m.cases) targets[(size_t)c.first] = c.second;
        for (int t : targets) e.line(format(".word %s-%s", e.blockLabel(t).c_str(), tab.c_str()));
        break;
    }
    case M_ASM: {
        std::string t;
        const std::string &src = m.text;
        for (size_t k = 0; k < src.size(); k++) {
            if (src[k] == '%' && k + 1 < src.size()) {
                if (src[k + 1] == '%') { t += '%'; k++; continue; }
                size_t j = k + 1;
                while (j < src.size() && isalpha((unsigned char)src[j]) && j + 1 < src.size() && isdigit((unsigned char)src[j + 1])) j++;
                if (j < src.size() && isdigit((unsigned char)src[j])) {
                    size_t idx = src[j] - '0';
                    if (idx < m.asmOps.size()) { t += O(m.asmOps[idx]); k = j; continue; }
                }
            }
            t += src[k];
        }
        // split into lines so each gets a leading tab
        size_t start = 0;
        while (start <= t.size()) {
            size_t nl = t.find('\n', start);
            std::string ln = t.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
            size_t a = ln.find_first_not_of(" \t");
            if (a != std::string::npos) {
                ln = ln.substr(a);
                bool isLabel = !ln.empty() && ln.back() == ':';
                if (isLabel) *e.out += ln + "\n";
                else e.line(ln);
            }
            if (nl == std::string::npos) break;
            start = nl + 1;
        }
        break;
    }
    }
    e.depth += m.spAdj;
}

void emitFileHeader(std::string &out) {
    out += "\t.text\n";
}

static std::string sectionFor(const char *kind, const std::string &name, bool perSymbol) {
    if (perSymbol) return format("\t.section %s.%s,\"%s\",@%s\n", kind, name.c_str(), !strcmp(kind, ".text") ? "ax" : !strcmp(kind, ".rodata") ? "a" : "aw",
                                 !strcmp(kind, ".bss") ? "nobits" : "progbits");
    if (!strcmp(kind, ".rodata")) return "\t.section .rodata,\"a\",@progbits\n";
    return format("\t%s\n", kind);
}

void emitFunction(Func *f, std::string &out) {
    Sel sel;
    S = &sel;
    sel.f = f;
    size_t nv = f->vregTy.size();
    sel.useCount.assign(nv, 0);
    sel.defCount.assign(nv, 0);
    sel.noSpill.assign(nv, 0);
    sel.forceCls.assign(nv, CLS_ANY);
    singleDefs.assign(nv, nullptr);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            auto use = [&](const Opd &o) { if (o.isReg()) sel.useCount[o.v]++; };
            if (i.op == IR_ASM) {
                for (auto &o : i.asmOps) {
                    if (o.isMem || o.isIn) use(o.val);
                    if (o.isOut && !o.isMem && o.val.isReg()) sel.defCount[o.val.v] += 2;
                    if (o.val.isReg()) sel.useCount[o.val.v] += 2;
                }
                continue;
            }
            use(i.a);
            use(i.b);
            if (i.call) {
                use(i.call->target);
                for (auto &a : i.call->args) use(a.val);
                use(i.call->retSlot);
                use(i.call->sret);
            }
            if (i.dst >= 0) {
                sel.defCount[i.dst]++;
                singleDefs[i.dst] = &i;
            }
        }
    for (size_t v = 0; v < nv; v++)
        if (sel.defCount[v] != 1) singleDefs[v] = nullptr;

    // ---- instruction selection
    sel.blocks.resize(f->blocks.size());
    for (size_t k = 0; k < f->blocks.size(); k++) {
        Block *b = f->blocks[k];
        MBlock &mb = sel.blocks[k];
        mb.id = b->id;
        mb.succs = b->succs;
        mb.loopDepth = b->loopDepth;
        sel.mb = &mb;
        sel.pending.clear();
        for (Ins &i : b->ins) {
            if (isDeferrable(i)) {
                sel.pending[i.dst] = {&i, sel.order++};
                continue;
            }
            if (i.op != IR_ASM && i.dst >= 0) flushUsing(i.dst);
            if (i.op == IR_ASM)
                for (auto &o : i.asmOps)
                    if (o.isOut && !o.isMem && o.val.isReg()) flushUsing(o.val.v);
            selectIns(i);
        }
        if (!sel.pending.empty()) {
            // deferred values that are used in other blocks: compute them before the terminator
            std::vector<MI> tail;
            while (!mb.mis.empty() && (mb.mis.back().op == M_BRA || mb.mis.back().op == M_BCC || mb.mis.back().op == M_RET || mb.mis.back().op == M_JTAB)) {
                tail.push_back(mb.mis.back());
                mb.mis.pop_back();
            }
            bool hadCmp = false;
            MI cmp;
            if (!tail.empty() && tail.back().op == M_BCC && !mb.mis.empty() && (mb.mis.back().op == M_CMP || mb.mis.back().op == M_TST)) {
                cmp = mb.mis.back();
                mb.mis.pop_back();
                hadCmp = true;
            }
            flushAll();
            if (hadCmp) mb.mis.push_back(cmp);
            for (size_t t = tail.size(); t-- > 0;) mb.mis.push_back(tail[t]);
        }
    }
    formAutoInc(sel);

    // ---- register allocation
    RA ra;
    ra.s = &sel;
    ra.f = f;
    determineClasses(sel, ra.cls);
    bool ok = false;
    for (int iter = 0; iter < 30 && !ok; iter++) ok = colorOnce(ra);
    if (!ok) fatal("%s: register allocation did not converge", f->name.c_str());
    peephole(sel, ra);

    // ---- frame layout
    uint32_t used = 0;
    std::vector<RegRef> refs;
    for (auto &b : sel.blocks)
        for (auto &m : b.mis) {
            regRefs(m, refs);
            for (auto &r : refs)
                if (r.def) used |= 1u << *r.reg;
            used |= m.defMask;
            auto mark = [&](const MOp &o) { if (o.kind == MOp::SLOT) f->slots[o.slot].used = true; };
            mark(m.src);
            mark(m.dst);
            for (auto &o : m.asmOps) mark(o);
        }
    uint32_t saved = 0;
    for (int r = 0; r < 15; r++)
        if ((used >> r & 1) && isCalleeSaved(r) && !(r == RA5 && opt.fixedA5)) saved |= 1u << r;
    int frame = 0;
    for (auto &sl : f->slots) {
        if (!sl.used || sl.argOffset >= 0) continue;
        int al = sl.align < 2 ? (sl.size > 1 ? 2 : 1) : 2;
        frame = (frame + al - 1) / al * al;
        sl.offset = frame;
        frame += sl.size;
    }
    frame = (frame + 1) & ~1;

    Emit e;
    e.f = f;
    e.ra = &ra;
    e.out = &out;
    e.frameSize = frame;
    e.fn = funcCounter++;
    int nsaved = 0;
    for (int r = 0; r < 15; r++) nsaved += saved >> r & 1;
    e.savedBytes = nsaved * 4;

    Obj *o = f->obj;
    std::string sec = o && !o->section.empty() ? format("\t.section %s,\"ax\",@progbits\n", o->section.c_str())
                                               : sectionFor(".text", f->name, opt.functionSections);
    out += sec;
    out += "\t.even\n";
    if (o && !o->isStatic) out += format("\t.globl %s\n", f->name.c_str());
    if (o && o->isWeak) out += format("\t.weak %s\n", f->name.c_str());
    out += format("\t.type %s,@function\n%s:\n", f->name.c_str(), f->name.c_str());
    if (nsaved == 1) {
        for (int r = 0; r < 15; r++)
            if (saved >> r & 1) e.line(format("move.l %s,-(%%sp)", regName(r)));
    } else if (nsaved > 1)
        e.line("movem.l " + movemList(saved) + ",-(%sp)");
    if (frame) {
        if (frame <= 8) e.line(format("subq.l #%d,%%sp", frame));
        else if (frame <= 32768) e.line(format("lea %d(%%sp),%%sp", -frame));
        else e.line(format("sub.l #%d,%%sp", frame));
    }
    int rets = 0;
    for (auto &b : sel.blocks)
        for (auto &m : b.mis)
            if (m.op == M_RET) rets++;
    bool sharedRet = rets > 1 && (frame || nsaved || (f->isPascal && f->argBytes));
    std::string retLabel = format(".Lret%d", e.fn);
    int seenRets = 0;
    for (size_t k = 0; k < sel.blocks.size(); k++) {
        MBlock &b = sel.blocks[k];
        out += e.blockLabel(b.id) + ":\n";
        e.depth = 0;
        int next = k + 1 < sel.blocks.size() ? sel.blocks[k + 1].id : -1;
        for (size_t idx = 0; idx < b.mis.size(); idx++) {
            MI &m = b.mis[idx];
            // only the final instruction of a block may fall through
            int nb = (idx + 1 == b.mis.size()) ? next : -1;
            // invert  bcc L1 ; bra L2  when L1 is the next block
            if (m.op == M_BCC && idx + 2 == b.mis.size() && b.mis[idx + 1].op == M_BRA && m.target == next) {
                m.cc = ccNegate(m.cc);
                m.target = b.mis[idx + 1].target;
                b.mis[idx + 1].target = next;
            }
            bool lastRet = false;
            if (m.op == M_RET) { seenRets++; lastRet = seenRets == rets; }
            emitMI(e, m, nb, retLabel, sharedRet, lastRet, saved);
        }
    }
    out += format("\t.size %s,.-%s\n", f->name.c_str(), f->name.c_str());
}

// ------------------------------------------------------------ data
void emitData(Program *prog, std::string &out) {
    for (const std::string &a : prog->topAsm) out += a + "\n";
    for (Obj *v : prog->globals) {
        if (v->isFunction || !v->isDefinition) continue;
        if (v->isStatic && !referencedSymbols.count(v->asmName)) continue;
        int size = tsize(v->ty);
        int align = v->align ? v->align : talign(v->ty);
        if (size >= 2 && align < 2 && !v->isString) align = 2;
        bool ro = v->isConstData && v->relocs.empty();
        if (v->isConstData && !v->relocs.empty()) ro = true; // relocated read-only data is still in the image
        const char *kind = v->isBss ? ".bss" : ro ? ".rodata" : ".data";
        if (!v->section.empty()) out += format("\t.section %s,\"aw\",@progbits\n", v->section.c_str());
        else out += sectionFor(kind, v->asmName[0] == '.' ? v->asmName.substr(1) : v->asmName, opt.dataSections);
        if (align >= 2) out += format("\t.balign %d\n", align);
        if (!v->isStatic) out += format("\t.globl %s\n", v->asmName.c_str());
        if (v->isWeak) out += format("\t.weak %s\n", v->asmName.c_str());
        out += format("\t.type %s,@object\n\t.size %s,%d\n", v->asmName.c_str(), v->asmName.c_str(), size);
        out += v->asmName + ":\n";
        if (v->isBss) {
            out += format("\t.space %d\n", size ? size : 1);
            continue;
        }
        std::map<int, const Reloc *> rel;
        for (auto &r : v->relocs) rel[r.offset] = &r;
        int pos = 0;
        std::string lineBuf;
        int inLine = 0;
        auto flush = [&]() {
            if (inLine) out += "\t.byte " + lineBuf + "\n";
            lineBuf.clear();
            inLine = 0;
        };
        while (pos < (int)v->data.size()) {
            auto it = rel.find(pos);
            if (it != rel.end()) {
                flush();
                const Reloc *r = it->second;
                referencedSymbols.insert(r->sym);
                if (r->addend) out += format("\t.long %s%+lld\n", r->sym.c_str(), (long long)r->addend);
                else out += format("\t.long %s\n", r->sym.c_str());
                pos += 4;
                continue;
            }
            lineBuf += format("%s%d", inLine ? "," : "", v->data[pos]);
            pos++;
            if (++inLine == 20) flush();
        }
        flush();
        if ((int)v->data.size() < size) out += format("\t.space %d\n", size - (int)v->data.size());
    }
}

} // namespace p68
