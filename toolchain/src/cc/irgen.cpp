// irgen.cpp - lower the typed AST of one function to the three-address IR.
//
// Value conventions:
//   * scalars of 1/2/4 bytes (integers, pointers, float bit patterns) are Opd values
//   * 8-byte scalars (long long, double) and aggregates are represented by the ADDRESS
//     of the memory holding them (an Opd that evaluates to a pointer)
#include "parse_int.h"

namespace p68 {

uint32_t floatBits(float f);
uint64_t doubleBits(double d);

static Func *F;
static Block *cur;
static Obj *curObj;
static std::map<std::string, Block *> labelBlocks;
static std::map<int, Block *> idBlocks;
static int curLine;
static int pascalResultSlot = -1;
static int vaSlot = -1;

static IRT irt(const Type *t) {
    int s = tsize(t);
    return s == 1 ? I8 : s == 2 ? I16 : I32;
}
static bool isWide(const Type *t) { return t->kind == TY_LLONG || t->kind == TY_DOUBLE; }
static bool isAgg(const Type *t) { return t->kind == TY_STRUCT || t->kind == TY_UNION || t->kind == TY_ARRAY; }
static bool isSigned(const Type *t) { return !t->isUnsigned && t->kind != TY_PTR && t->kind != TY_BOOL; }

static int64_t norm(int64_t c, IRT t) {
    switch (t) {
    case I8: return (int8_t)c;
    case I16: return (int16_t)c;
    default: return (int32_t)c;
    }
}

static Ins &emit(IROp op) {
    cur->ins.emplace_back();
    Ins &i = cur->ins.back();
    i.op = op;
    i.line = curLine;
    return i;
}
static bool terminated() {
    if (cur->ins.empty()) return false;
    IROp op = cur->ins.back().op;
    return op == IR_JMP || op == IR_BR || op == IR_RET || op == IR_SWITCH || op == IR_UNREACH;
}
static Block *newBlock() { return F->newBlock(); }
static void jmp(Block *b) {
    if (terminated()) return;
    Ins &i = emit(IR_JMP);
    i.t1 = b->id;
}
static void setBlock(Block *b) {
    jmp(b);
    F->blocks.push_back(b);
    cur = b;
}
static void freshBlock() { // start an (unreachable) block after a terminator
    Block *b = newBlock();
    F->blocks.push_back(b);
    cur = b;
}
static int tmp(IRT t, bool ptr = false) { return F->newVreg(t, ptr); }

static bool foldBin(IROp op, IRT ty, int64_t a, int64_t b, int64_t *r) {
    uint32_t mask = ty == I8 ? 0xFF : ty == I16 ? 0xFFFF : 0xFFFFFFFFu;
    uint32_t ua = (uint32_t)a & mask, ub = (uint32_t)b & mask;
    int64_t sa = norm(a, ty), sb = norm(b, ty);
    int bits = ty * 8;
    switch (op) {
    case IR_ADD: *r = sa + sb; break;
    case IR_SUB: *r = sa - sb; break;
    case IR_MUL: *r = sa * sb; break;
    case IR_DIVS: if (sb == 0) return false; *r = sb == -1 ? -sa : sa / sb; break;
    case IR_DIVU: if (ub == 0) return false; *r = ua / ub; break;
    case IR_MODS: if (sb == 0) return false; *r = sb == -1 ? 0 : sa % sb; break;
    case IR_MODU: if (ub == 0) return false; *r = ua % ub; break;
    case IR_AND: *r = sa & sb; break;
    case IR_OR: *r = sa | sb; break;
    case IR_XOR: *r = sa ^ sb; break;
    case IR_SHL: if (sb < 0 || sb >= bits) return false; *r = (int64_t)((uint64_t)ua << sb); break;
    case IR_SHRS: if (sb < 0 || sb >= bits) return false; *r = sa >> sb; break;
    case IR_SHRU: if (sb < 0 || sb >= bits) return false; *r = ua >> sb; break;
    default: return false;
    }
    *r = norm(*r, ty);
    return true;
}

static Opd mov(Opd v, IRT ty, bool ptr = false) {
    int d = tmp(ty, ptr);
    Ins &i = emit(IR_MOV);
    i.ty = ty;
    i.dst = d;
    i.a = v;
    return Opd::reg(d);
}

static Opd bin(IROp op, IRT ty, Opd a, Opd b, bool ptr = false) {
    if (a.isConst() && b.isConst()) {
        int64_t r;
        if (foldBin(op, ty, a.c, b.c, &r)) return Opd::imm(r);
    }
    // address constants: sym + const
    if ((op == IR_ADD || op == IR_SUB) && ty == I32 && (a.kind == Opd::SYM || a.kind == Opd::SLOT) && b.isConst()) {
        a.c += op == IR_ADD ? b.c : -b.c;
        return a;
    }
    if (op == IR_ADD && ty == I32 && (b.kind == Opd::SYM || b.kind == Opd::SLOT) && a.isConst()) {
        b.c += a.c;
        return b;
    }
    if (b.isConst()) {
        int64_t c = norm(b.c, ty);
        if (c == 0 && (op == IR_ADD || op == IR_SUB || op == IR_OR || op == IR_XOR || op == IR_SHL || op == IR_SHRS || op == IR_SHRU)) return a;
        if (c == 1 && (op == IR_MUL || op == IR_DIVS || op == IR_DIVU)) return a;
        if (c == 0 && (op == IR_MUL || op == IR_AND)) return Opd::imm(0);
        if (c == -1 && op == IR_AND) return a;
    }
    if (a.isConst()) {
        int64_t c = norm(a.c, ty);
        if (c == 0 && (op == IR_ADD || op == IR_OR || op == IR_XOR)) return b;
        if (c == 1 && op == IR_MUL) return b;
        if (c == 0 && (op == IR_MUL || op == IR_AND)) return Opd::imm(0);
        // canonical: constant on the right for commutative operators
        if (op == IR_ADD || op == IR_MUL || op == IR_AND || op == IR_OR || op == IR_XOR) std::swap(a, b);
    }
    int d = tmp(ty, ptr);
    Ins &i = emit(op);
    i.ty = ty;
    i.dst = d;
    i.a = a;
    i.b = b;
    return Opd::reg(d);
}

static Opd un(IROp op, IRT ty, Opd a) {
    if (a.isConst()) return Opd::imm(norm(op == IR_NEG ? -a.c : ~a.c, ty));
    int d = tmp(ty);
    Ins &i = emit(op);
    i.ty = ty;
    i.dst = d;
    i.a = a;
    return Opd::reg(d);
}

// convert an integer value between widths
static Opd ext(Opd v, IRT from, IRT to, bool sign) {
    if (from == to) return v;
    if (v.isConst()) {
        int64_t c = norm(v.c, from);
        if (to > from && !sign) c &= (from == I8 ? 0xFF : 0xFFFF);
        return Opd::imm(norm(c, to));
    }
    if (v.kind == Opd::SYM || v.kind == Opd::SLOT) v = mov(v, I32, true);
    int d = tmp(to);
    Ins &i = emit(to < from ? IR_TRUNC : sign ? IR_SEXT : IR_ZEXT);
    i.ty = to;
    i.srcTy = from;
    i.dst = d;
    i.a = v;
    return Opd::reg(d);
}

static Opd addrAdd(Opd addr, int64_t off) {
    if (off == 0) return addr;
    if (addr.kind == Opd::SYM || addr.kind == Opd::SLOT || addr.kind == Opd::CONST) {
        addr.c += off;
        return addr;
    }
    return bin(IR_ADD, I32, addr, Opd::imm(off), true);
}

static Opd loadMem(Opd addr, int64_t off, IRT ty, bool vol = false, bool ptr = false) {
    if (addr.kind == Opd::SYM || addr.kind == Opd::SLOT || addr.kind == Opd::CONST) { addr.c += off; off = 0; }
    int d = tmp(ty, ptr);
    Ins &i = emit(IR_LOAD);
    i.ty = ty;
    i.dst = d;
    i.a = addr;
    i.off = off;
    i.isVolatile = vol;
    return Opd::reg(d);
}
static void storeMem(Opd addr, int64_t off, IRT ty, Opd val, bool vol = false) {
    if (addr.kind == Opd::SYM || addr.kind == Opd::SLOT || addr.kind == Opd::CONST) { addr.c += off; off = 0; }
    if (val.isConst()) val.c = norm(val.c, ty);
    Ins &i = emit(IR_STORE);
    i.ty = ty;
    i.a = addr;
    i.b = val;
    i.off = off;
    i.isVolatile = vol;
}
// align: 1 = the blocks may start at odd addresses (byte copy), 2 = word aligned
static void copyBlock(Opd dst, Opd src, int size, int align = 2) {
    if (size == 0 || dst == src) return;
    Ins &i = emit(IR_COPYBLK);
    i.a = dst;
    i.b = src;
    i.off = size;
    i.ty = align >= 2 ? I16 : I8;
}
static Opd newWideSlot() { return Opd::slot(F->newSlot(8, 2)); }
static Opd wideConst(uint64_t bits) {
    Opd s = newWideSlot();
    storeMem(s, 0, I32, Opd::imm((int32_t)(bits >> 32)));
    storeMem(s, 4, I32, Opd::imm((int32_t)bits));
    return s;
}

static void br(CC cc, IRT ty, Opd a, Opd b, Block *t, Block *f) {
    if (a.isConst() && b.isConst()) {
        int64_t x = norm(a.c, ty), y = norm(b.c, ty);
        uint32_t mask = ty == I8 ? 0xFF : ty == I16 ? 0xFFFF : 0xFFFFFFFFu;
        uint32_t ux = (uint32_t)x & mask, uy = (uint32_t)y & mask;
        bool r = false;
        switch (cc) {
        case CC_EQ: r = x == y; break;
        case CC_NE: r = x != y; break;
        case CC_LT: r = x < y; break;
        case CC_LE: r = x <= y; break;
        case CC_GT: r = x > y; break;
        case CC_GE: r = x >= y; break;
        case CC_ULT: r = ux < uy; break;
        case CC_ULE: r = ux <= uy; break;
        case CC_UGT: r = ux > uy; break;
        case CC_UGE: r = ux >= uy; break;
        }
        jmp(r ? t : f);
        return;
    }
    if (a.isConst()) { std::swap(a, b); cc = ccSwap(cc); }
    Ins &i = emit(IR_BR);
    i.ty = ty;
    i.cc = cc;
    i.a = a;
    i.b = b;
    i.t1 = t->id;
    i.t2 = f->id;
}

// ------------------------------------------------------------ helper calls
struct HArg {
    Opd v;
    bool wide;
};
static Opd callHelper(const char *name, std::vector<HArg> args, int retSize) {
    CallInfo *ci = new CallInfo;
    ci->target = Opd::symbol(name);
    for (auto &a : args) {
        ArgInfo ai;
        ai.val = a.v;
        ai.size = a.wide ? 8 : 4;
        ai.valSize = a.wide ? 8 : 4;
        ai.byMem = a.wide;
        ci->args.push_back(ai);
    }
    ci->retSize = retSize;
    Opd res;
    int dst = -1;
    if (retSize == 8) {
        res = newWideSlot();
        ci->retSlot = res;
    } else if (retSize) {
        dst = tmp(I32);
        res = Opd::reg(dst);
    }
    Ins &i = emit(IR_CALL);
    i.call = ci;
    i.dst = dst;
    i.ty = I32;
    F->hasCalls = true;
    return res;
}

// ------------------------------------------------------------ lvalues
struct LV {
    enum Kind { REG, MEM, BITFIELD } kind = MEM;
    int vreg = -1;
    Opd addr;
    int64_t off = 0;
    Type *ty = nullptr;
    Member *bf = nullptr;
    bool vol = false;
    bool unaligned = false;   // may sit at an odd address (member of a packed struct)
};

static Opd genExpr(Node *n);
static void genStmt(Node *n);
static void genCond(Node *n, Block *t, Block *f);
static Opd genCast(Opd v, Type *from, Type *to);
static LV genLval(Node *n);

static LV genLval(Node *n) {
    LV lv;
    lv.ty = n->ty;
    lv.vol = n->ty->isVolatile;
    switch (n->kind) {
    case ND_VAR: {
        Obj *v = n->var;
        if (v->isLocal) {
            if (v->vreg >= 0) { lv.kind = LV::REG; lv.vreg = v->vreg; return lv; }
            if (v->slot < 0) errorAt(n->tok, "internal: local '%s' has no storage", v->name.c_str());
            lv.addr = Opd::slot(v->slot);
            return lv;
        }
        lv.addr = Opd::symbol(v->asmName);
        referencedSymbols.insert(v->asmName);
        return lv;
    }
    case ND_DEREF:
        lv.addr = genExpr(n->lhs);
        return lv;
    case ND_MEMBER: {
        LV base;
        Node *l = n->lhs;
        if (l->kind == ND_VAR || l->kind == ND_DEREF || l->kind == ND_MEMBER) base = genLval(l);
        else { base.addr = genExpr(l); base.ty = l->ty; }
        if (base.kind != LV::MEM) errorAt(n->tok, "internal: member of a register value");
        lv.addr = base.addr;
        lv.off = base.off + n->member->offset;
        lv.vol = lv.vol || base.vol;
        lv.unaligned = base.unaligned || talign(l->ty) < 2;
        if (n->member->isBitfield) { lv.kind = LV::BITFIELD; lv.bf = n->member; }
        return lv;
    }
    case ND_COMMA:
        genExpr(n->lhs);
        return genLval(n->rhs);
    default:
        if (isAgg(n->ty) || isWide(n->ty)) {
            lv.addr = genExpr(n);
            return lv;
        }
        errorAt(n->tok, "expression is not an lvalue");
    }
}

static Opd loadLV(LV &lv) {
    Type *ty = lv.ty;
    if (lv.kind == LV::REG) return Opd::reg(lv.vreg);
    if (lv.kind == LV::BITFIELD) {
        Member *m = lv.bf;
        IRT ut = irt(m->ty);
        int ubits = ut * 8;
        Opd u = loadMem(lv.addr, lv.off, ut, lv.vol);
        bool sign = isSigned(m->ty);
        // shift the field to the top, then down (arithmetic or logical)
        Opd v = u;
        if (m->bitOffset) v = bin(IR_SHL, ut, v, Opd::imm(m->bitOffset));
        if (ubits - m->bitWidth) v = bin(sign ? IR_SHRS : IR_SHRU, ut, v, Opd::imm(ubits - m->bitWidth));
        return v;
    }
    if (isAgg(ty) || isWide(ty)) {
        if (lv.unaligned && talign(ty) >= 2) { // realign into a temporary
            Opd t = Opd::slot(F->newSlot(tsize(ty), 2));
            copyBlock(t, addrAdd(lv.addr, lv.off), tsize(ty), 1);
            return t;
        }
        return addrAdd(lv.addr, lv.off);
    }
    if (lv.unaligned && tsize(ty) > 1) {
        Opd t = Opd::slot(F->newSlot(tsize(ty), 2));
        copyBlock(t, addrAdd(lv.addr, lv.off), tsize(ty), 1);
        return loadMem(t, 0, irt(ty), false, ty->kind == TY_PTR);
    }
    return loadMem(lv.addr, lv.off, irt(ty), lv.vol, ty->kind == TY_PTR);
}

static void storeLV(LV &lv, Opd val) {
    Type *ty = lv.ty;
    if (lv.kind == LV::REG) {
        if (val.isReg() && val.v == lv.vreg) return;
        Ins &i = emit(IR_MOV);
        i.ty = F->vregTy[lv.vreg];
        i.dst = lv.vreg;
        i.a = val.isConst() ? Opd::imm(norm(val.c, i.ty)) : val;
        return;
    }
    if (lv.kind == LV::BITFIELD) {
        Member *m = lv.bf;
        IRT ut = irt(m->ty);
        int ubits = ut * 8;
        int shift = ubits - m->bitOffset - m->bitWidth;
        int64_t mask = (m->bitWidth >= 32 ? 0xFFFFFFFFLL : ((1LL << m->bitWidth) - 1)) << shift;
        Opd u = loadMem(lv.addr, lv.off, ut, lv.vol);
        Opd cleared = bin(IR_AND, ut, u, Opd::imm(norm(~mask, ut)));
        Opd v = val;
        if (shift) v = bin(IR_SHL, ut, v, Opd::imm(shift));
        v = bin(IR_AND, ut, v, Opd::imm(norm(mask, ut)));
        storeMem(lv.addr, lv.off, ut, bin(IR_OR, ut, cleared, v), lv.vol);
        return;
    }
    if (isAgg(ty) || isWide(ty)) {
        copyBlock(addrAdd(lv.addr, lv.off), val, tsize(ty), lv.unaligned ? 1 : talign(ty));
        return;
    }
    if (lv.unaligned && tsize(ty) > 1) {
        Opd t = Opd::slot(F->newSlot(tsize(ty), 2));
        storeMem(t, 0, irt(ty), val);
        copyBlock(addrAdd(lv.addr, lv.off), t, tsize(ty), 1);
        return;
    }
    storeMem(lv.addr, lv.off, irt(ty), val, lv.vol);
}

// ------------------------------------------------------------ arithmetic
static const char *fsuffix(const Type *t) { return t->kind == TY_FLOAT ? "sf" : "df"; }

static Opd wideHalf(Opd addr, int which) { return loadMem(addr, which * 4, I32); }

static Opd arith(NodeKind op, Type *ty, Opd a, Opd b, Token *tk) {
    if (isFlonum(ty)) {
        bool w = ty->kind == TY_DOUBLE;
        const char *nm = op == ND_ADD ? "add" : op == ND_SUB ? "sub" : op == ND_MUL ? "mul" : op == ND_DIV ? "div" : nullptr;
        if (!nm) errorAt(tk, "invalid floating-point operation");
        return callHelper(format("__%s%s3", nm, fsuffix(ty)).c_str(), {{a, w}, {b, w}}, w ? 8 : 4);
    }
    if (tsize(ty) == 8) {
        bool u = ty->isUnsigned;
        switch (op) {
        case ND_AND: case ND_OR: case ND_XOR: {
            IROp o = op == ND_AND ? IR_AND : op == ND_OR ? IR_OR : IR_XOR;
            Opd r = newWideSlot();
            storeMem(r, 0, I32, bin(o, I32, wideHalf(a, 0), wideHalf(b, 0)));
            storeMem(r, 4, I32, bin(o, I32, wideHalf(a, 1), wideHalf(b, 1)));
            return r;
        }
        case ND_ADD: return callHelper("__adddi3", {{a, true}, {b, true}}, 8);
        case ND_SUB: return callHelper("__subdi3", {{a, true}, {b, true}}, 8);
        case ND_MUL: return callHelper("__muldi3", {{a, true}, {b, true}}, 8);
        case ND_DIV: return callHelper(u ? "__udivdi3" : "__divdi3", {{a, true}, {b, true}}, 8);
        case ND_MOD: return callHelper(u ? "__umoddi3" : "__moddi3", {{a, true}, {b, true}}, 8);
        case ND_SHL: return callHelper("__ashldi3", {{a, true}, {b, false}}, 8);
        case ND_SHR: return callHelper(u ? "__lshrdi3" : "__ashrdi3", {{a, true}, {b, false}}, 8);
        default: errorAt(tk, "invalid 64-bit operation");
        }
    }
    IRT t = irt(ty);
    bool u = !isSigned(ty);
    IROp o;
    switch (op) {
    case ND_ADD: o = IR_ADD; break;
    case ND_SUB: o = IR_SUB; break;
    case ND_MUL: o = IR_MUL; break;
    case ND_DIV: o = u ? IR_DIVU : IR_DIVS; break;
    case ND_MOD: o = u ? IR_MODU : IR_MODS; break;
    case ND_AND: o = IR_AND; break;
    case ND_OR: o = IR_OR; break;
    case ND_XOR: o = IR_XOR; break;
    case ND_SHL: o = IR_SHL; break;
    case ND_SHR: o = u ? IR_SHRU : IR_SHRS; break;
    default: errorAt(tk, "internal: bad arithmetic operator");
    }
    return bin(o, t, a, b, ty->kind == TY_PTR);
}

// value of integer expression `n` truncated to width w (only the low w bytes matter)
static Opd genNarrow(Node *n, IRT w) {
    Type *ty = n->ty;
    if (!(isInteger(ty) || ty->kind == TY_PTR) || tsize(ty) == 8 || (int)w >= tsize(ty)) {
        Opd v = genExpr(n);
        if (tsize(ty) == 8) return loadMem(v, 8 - w, w);
        return ext(v, irt(ty), w, false);
    }
    switch (n->kind) {
    case ND_NUM: return Opd::imm(norm(n->ival, w));
    case ND_CAST: {
        Type *from = n->lhs->ty;
        if ((isInteger(from) || from->kind == TY_PTR) && from->kind != TY_BOOL && ty->kind != TY_BOOL && tsize(from) >= (int)w && tsize(from) <= 4)
            return genNarrow(n->lhs, w);
        break;
    }
    case ND_ADD: case ND_SUB: case ND_MUL: case ND_AND: case ND_OR: case ND_XOR: {
        Opd a = genNarrow(n->lhs, w);
        Opd b = genNarrow(n->rhs, w);
        IROp o = n->kind == ND_ADD ? IR_ADD : n->kind == ND_SUB ? IR_SUB : n->kind == ND_MUL ? IR_MUL : n->kind == ND_AND ? IR_AND : n->kind == ND_OR ? IR_OR : IR_XOR;
        return bin(o, w, a, b);
    }
    case ND_NEG: return un(IR_NEG, w, genNarrow(n->lhs, w));
    case ND_BITNOT: return un(IR_NOT, w, genNarrow(n->lhs, w));
    case ND_SHL:
        if (n->rhs->kind == ND_NUM && n->rhs->ival >= 0 && n->rhs->ival < w * 8)
            return bin(IR_SHL, w, genNarrow(n->lhs, w), Opd::imm(n->rhs->ival));
        break;
    case ND_COMMA:
        genExpr(n->lhs);
        return genNarrow(n->rhs, w);
    case ND_VAR: case ND_DEREF: case ND_MEMBER: {
        if (n->ty->isVolatile) break;
        LV lv = genLval(n);
        if (lv.kind == LV::MEM && !lv.vol && (!lv.unaligned || w == I8)) return loadMem(lv.addr, lv.off + (tsize(ty) - w), w);
        Opd v = loadLV(lv);
        return ext(v, irt(ty), w, false);
    }
    default: break;
    }
    Opd v = genExpr(n);
    return ext(v, irt(ty), w, false);
}

static Opd floatNonZero(Opd v, Type *ty) { // returns an I32 that is nonzero iff the float is nonzero
    if (ty->kind == TY_FLOAT) return bin(IR_SHL, I32, v, Opd::imm(1));
    Opd hi = bin(IR_SHL, I32, wideHalf(v, 0), Opd::imm(1));
    return bin(IR_OR, I32, hi, wideHalf(v, 1));
}

static Opd genCast(Opd v, Type *from, Type *to) {
    if (to->kind == TY_VOID) return Opd();
    if (isAgg(to)) return v;
    if (to->kind == TY_BOOL) {
        if (from->kind == TY_BOOL) return v;
        Opd x = v;
        IRT t = I32;
        if (isFlonum(from)) x = floatNonZero(v, from);
        else if (tsize(from) == 8) x = bin(IR_OR, I32, wideHalf(v, 0), wideHalf(v, 1));
        else t = irt(from);
        if (x.isConst()) return Opd::imm(norm(x.c, t) != 0);
        int d = tmp(I8);
        Ins &i = emit(IR_SETCC);
        i.ty = t;
        i.cc = CC_NE;
        i.dst = d;
        i.a = x;
        i.b = Opd::imm(0);
        return Opd::reg(d);
    }
    bool ff = isFlonum(from), tf = isFlonum(to);
    if (ff && tf) {
        if (from->kind == to->kind) return v;
        if (to->kind == TY_DOUBLE) return callHelper("__extendsfdf2", {{v, false}}, 8);
        return callHelper("__truncdfsf2", {{v, true}}, 4);
    }
    if (ff) { // float -> integer
        bool w = from->kind == TY_DOUBLE;
        const char *fs = fsuffix(from);
        if (tsize(to) == 8)
            return callHelper(format("__fix%s%sdi", to->isUnsigned ? "uns" : "", fs).c_str(), {{v, w}}, 8);
        bool u = to->isUnsigned && tsize(to) == 4;
        Opd r = callHelper(format("__fix%s%ssi", u ? "uns" : "", fs).c_str(), {{v, w}}, 4);
        return ext(r, I32, irt(to), false);
    }
    if (tf) { // integer -> float
        bool w = to->kind == TY_DOUBLE;
        const char *fs = fsuffix(to);
        if (tsize(from) == 8)
            return callHelper(format("__float%sdi%s", from->isUnsigned ? "un" : "", fs).c_str(), {{v, true}}, w ? 8 : 4);
        bool u = !isSigned(from);
        Opd x = ext(v, irt(from), I32, !u);
        bool useUns = u && tsize(from) == 4;
        return callHelper(format("__float%ssi%s", useUns ? "un" : "", fs).c_str(), {{x, false}}, w ? 8 : 4);
    }
    int fs = tsize(from), ts = tsize(to);
    if (fs == 8 && ts == 8) return v;
    if (ts == 8) {
        Opd x = ext(v, irt(from), I32, isSigned(from));
        Opd r = newWideSlot();
        Opd hi = isSigned(from) ? bin(IR_SHRS, I32, x, Opd::imm(31)) : Opd::imm(0);
        storeMem(r, 0, I32, hi);
        storeMem(r, 4, I32, x);
        return r;
    }
    if (fs == 8) return loadMem(v, 8 - ts, irt(to));
    return ext(v, irt(from), irt(to), isSigned(from));
}

// ------------------------------------------------------------ comparisons / conditions
static Node *stripExt(Node *n, Type **narrow) {
    if (n->kind != ND_CAST) return nullptr;
    Type *to = n->ty, *from = n->lhs->ty;
    if (!isInteger(to) || !isInteger(from) || tsize(to) > 4 || tsize(from) >= tsize(to)) return nullptr;
    *narrow = from;
    return n->lhs;
}
static bool fitsIn(int64_t v, const Type *t) {
    int bits = tsize(t) * 8;
    if (t->kind == TY_BOOL) return v == 0 || v == 1;
    if (isSigned(t)) return v >= -(1LL << (bits - 1)) && v < (1LL << (bits - 1));
    return v >= 0 && v < (1LL << bits);
}

// Produce operands and condition for a comparison node (EQ/NE/LT/LE) on <=4-byte integers.
static void cmpOperands(Node *n, Opd *a, Opd *b, IRT *ty, CC *cc) {
    Node *l = n->lhs, *r = n->rhs;
    Type *cty = l->ty; // operands were converted to a common type
    bool u = !isSigned(cty);
    NodeKind k = n->kind;
    bool ordered = k == ND_LT || k == ND_LE;
    Type *lt = nullptr, *rt = nullptr;
    Node *ls = stripExt(l, &lt), *rs = stripExt(r, &rt);
    Type *nt = nullptr;
    Node *ln = l, *rn = r;
    if (ls && rs && tsize(lt) == tsize(rt) && isSigned(lt) == isSigned(rt)) {
        nt = lt; ln = ls; rn = rs;
    } else if (ls && r->kind == ND_NUM && fitsIn((int32_t)r->ival, lt)) {
        nt = lt; ln = ls;
    } else if (rs && l->kind == ND_NUM && fitsIn((int32_t)l->ival, rt)) {
        nt = rt; rn = rs;
    }
    if (nt && ordered && u && isSigned(nt)) nt = nullptr; // unsigned compare of sign-extended values
    if (nt) {
        *ty = irt(nt);
        u = !isSigned(nt);
        *a = ln == l ? Opd::imm(norm(l->ival, *ty)) : genExpr(ln);
        *b = rn == r ? Opd::imm(norm(r->ival, *ty)) : genExpr(rn);
    } else {
        *ty = irt(cty);
        *a = genExpr(l);
        *b = genExpr(r);
    }
    switch (k) {
    case ND_EQ: *cc = CC_EQ; break;
    case ND_NE: *cc = CC_NE; break;
    case ND_LT: *cc = u ? CC_ULT : CC_LT; break;
    default: *cc = u ? CC_ULE : CC_LE; break;
    }
}

static bool isCmp(NodeKind k) { return k == ND_EQ || k == ND_NE || k == ND_LT || k == ND_LE; }

// helper-based comparison (float/double/64-bit): returns value to compare against `*rhs` with `*cc`
static Opd wideCmp(Node *n, CC *cc, int64_t *rhs) {
    Type *ty = n->lhs->ty;
    Opd a = genExpr(n->lhs), b = genExpr(n->rhs);
    if (isFlonum(ty)) {
        bool w = ty->kind == TY_DOUBLE;
        const char *nm = n->kind == ND_EQ ? "eq" : n->kind == ND_NE ? "ne" : n->kind == ND_LT ? "lt" : "le";
        *cc = n->kind == ND_EQ ? CC_EQ : n->kind == ND_NE ? CC_NE : n->kind == ND_LT ? CC_LT : CC_LE;
        *rhs = 0;
        return callHelper(format("__%s%s2", nm, fsuffix(ty)).c_str(), {{a, w}, {b, w}}, 4);
    }
    // 64-bit integers: __cmpdi2 / __ucmpdi2 return 0 (a<b), 1 (a==b), 2 (a>b)
    *cc = n->kind == ND_EQ ? CC_EQ : n->kind == ND_NE ? CC_NE : n->kind == ND_LT ? CC_LT : CC_LE;
    *rhs = 1;
    return callHelper(ty->isUnsigned ? "__ucmpdi2" : "__cmpdi2", {{a, true}, {b, true}}, 4);
}

static void genCond(Node *n, Block *t, Block *f) {
    curLine = n->tok ? n->tok->line : curLine;
    switch (n->kind) {
    case ND_NUM:
        jmp(n->ival ? t : f);
        return;
    case ND_NOT:
        genCond(n->lhs, f, t);
        return;
    case ND_LOGAND: {
        Block *mid = newBlock();
        genCond(n->lhs, mid, f);
        setBlock(mid);
        genCond(n->rhs, t, f);
        return;
    }
    case ND_LOGOR: {
        Block *mid = newBlock();
        genCond(n->lhs, t, mid);
        setBlock(mid);
        genCond(n->rhs, t, f);
        return;
    }
    case ND_COMMA:
        genExpr(n->lhs);
        genCond(n->rhs, t, f);
        return;
    case ND_CAST: {
        Type *from = n->lhs->ty, *to = n->ty;
        bool scalarInt = (isInteger(from) || from->kind == TY_PTR) && (isInteger(to) || to->kind == TY_PTR);
        if (scalarInt && (to->kind == TY_BOOL || tsize(to) >= tsize(from))) {
            genCond(n->lhs, t, f);
            return;
        }
        break;
    }
    default: break;
    }
    if (isCmp(n->kind)) {
        Type *oty = n->lhs->ty;
        if (isFlonum(oty) || tsize(oty) == 8) {
            CC cc;
            int64_t rhs;
            Opd v = wideCmp(n, &cc, &rhs);
            br(cc, I32, v, Opd::imm(rhs), t, f);
            return;
        }
        Opd a, b;
        IRT ty;
        CC cc;
        cmpOperands(n, &a, &b, &ty, &cc);
        br(cc, ty, a, b, t, f);
        return;
    }
    Opd v = genExpr(n);
    Type *ty = n->ty;
    if (isFlonum(ty)) { br(CC_NE, I32, floatNonZero(v, ty), Opd::imm(0), t, f); return; }
    if (tsize(ty) == 8) { br(CC_NE, I32, bin(IR_OR, I32, wideHalf(v, 0), wideHalf(v, 1)), Opd::imm(0), t, f); return; }
    br(CC_NE, irt(ty), v, Opd::imm(0), t, f);
}

static Opd genBoolValue(Node *n) { // 0/1 as I32 through control flow
    int d = tmp(I32);
    Block *t = newBlock(), *f = newBlock(), *end = newBlock();
    genCond(n, t, f);
    setBlock(t);
    { Ins &i = emit(IR_MOV); i.ty = I32; i.dst = d; i.a = Opd::imm(1); }
    jmp(end);
    setBlock(f);
    { Ins &i = emit(IR_MOV); i.ty = I32; i.dst = d; i.a = Opd::imm(0); }
    setBlock(end);
    return Opd::reg(d);
}

// ------------------------------------------------------------ calls
static int pascalArgSize(Type *t) {
    int s = tsize(t);
    if (isAgg(t)) return s <= 4 ? (s + 1) & ~1 : 4;
    return s == 1 ? 2 : s;
}

static Opd genCall(Node *n) {
    Type *fty = n->funcTy;
    Obj *callee = n->callee;
    CallInfo *ci = new CallInfo;
    ci->cc = fty->cc;
    ci->variadic = fty->variadic;
    bool regp = callee && callee->regParams.present;
    if (callee && !callee->inlineWords.empty()) ci->inlineWords = callee->inlineWords;
    if (callee) {
        ci->noreturn = callee->noreturn;
        if (ci->inlineWords.empty()) {
            ci->target = Opd::symbol(callee->asmName);
            referencedSymbols.insert(callee->asmName);
        }
    } else {
        Opd t = genExpr(n->lhs);
        if (t.isConst()) t = mov(t, I32, true);
        ci->target = t;
    }
    Type *rt = fty->base;
    // evaluate arguments left to right
    for (size_t i = 0; i < n->body.size(); i++) {
        Node *a = n->body[i];
        Type *at = a->ty;
        ArgInfo ai;
        Opd v = genExpr(a);
        if (regp) {
            if (i >= callee->regParams.argRegs.size()) errorAt(a->tok, "too many arguments for a register-parameter call");
            ai.reg = callee->regParams.argRegs[i];
            if (isAgg(at) || isWide(at)) {
                if (tsize(at) > 4) errorAt(a->tok, "argument does not fit in a register");
                v = loadMem(v, 0, irt(at));
            }
            ai.val = ext(v, irt(at), I32, isSigned(at));
            ai.size = ai.valSize = 4;
        } else if (ci->cc == CC_PASCAL) {
            if (isAgg(at)) {
                int s = tsize(at);
                if (s <= 4) { ai.val = v; ai.byMem = true; ai.valSize = s; ai.size = (s + 1) & ~1; }
                else { ai.val = v; ai.size = ai.valSize = 4; }
            } else if (isWide(at)) {
                ai.val = v; ai.byMem = true; ai.size = ai.valSize = 8;
            } else {
                ai.val = v;
                ai.valSize = tsize(at);
                ai.size = pascalArgSize(at);
                if (v.isConst()) ai.val = Opd::imm(norm(v.c, irt(at)));
            }
        } else {
            if (isAgg(at)) {
                ai.val = v; ai.byMem = true; ai.valSize = tsize(at); ai.size = (tsize(at) + 3) & ~3;
            } else if (isWide(at)) {
                ai.val = v; ai.byMem = true; ai.size = ai.valSize = 8;
            } else {
                ai.val = ext(v, irt(at), I32, isSigned(at));
                ai.size = ai.valSize = 4;
            }
        }
        ci->args.push_back(ai);
    }
    Opd result;
    int dst = -1;
    IRT dty = I32;
    if (rt->kind == TY_VOID) {
        ci->retSize = 0;
    } else if (regp) {
        if (isAgg(rt) || isWide(rt)) errorAt(n->tok, "register-parameter function returns an aggregate");
        ci->retReg = callee->regParams.retReg >= 0 ? callee->regParams.retReg : 0;
        ci->retSize = tsize(rt);
        dty = irt(rt);
        dst = tmp(dty, rt->kind == TY_PTR);
        result = Opd::reg(dst);
    } else if (ci->cc == CC_PASCAL) {
        if (isWide(rt) || (isAgg(rt) && tsize(rt) > 4)) {
            // large results: caller passes the address of the result as a hidden first parameter
            Opd slot = Opd::slot(F->newSlot(tsize(rt), 2));
            ArgInfo ai;
            ai.val = slot;
            ai.size = ai.valSize = 4;
            ci->args.insert(ci->args.begin(), ai);
            result = slot;
            ci->retSize = 0;
        } else if (isAgg(rt)) {
            ci->pascalResultOnStack = true;
            ci->pascalResultSize = (tsize(rt) + 1) & ~1;
            ci->retSize = ci->pascalResultSize;
            dty = ci->retSize == 2 ? I16 : I32;
            dst = tmp(dty);
            Opd slot = Opd::slot(F->newSlot(4, 2));
            result = slot; // stored below
        } else {
            ci->pascalResultOnStack = true;
            ci->pascalResultSize = tsize(rt) <= 2 ? 2 : 4;
            ci->retSize = tsize(rt);
            dty = irt(rt);
            dst = tmp(dty, rt->kind == TY_PTR);
            result = Opd::reg(dst);
        }
    } else if (isWide(rt)) {
        ci->retSize = 8;
        result = newWideSlot();
        ci->retSlot = result;
    } else if (isAgg(rt)) {
        result = Opd::slot(F->newSlot(tsize(rt), 2));
        ci->sret = result;
        ci->retSize = 0;
    } else {
        ci->retSize = tsize(rt);
        dty = irt(rt);
        dst = tmp(dty, rt->kind == TY_PTR);
        result = Opd::reg(dst);
    }
    ci->regParams = regp;
    {
        Ins &i = emit(IR_CALL);
        i.call = ci;
        i.dst = dst;
        i.ty = dty;
    }
    F->hasCalls = true;
    if (ci->cc == CC_PASCAL && isAgg(rt) && tsize(rt) <= 4 && dst >= 0) storeMem(result, 0, dty, Opd::reg(dst));
    if (ci->noreturn) {
        emit(IR_UNREACH);
        freshBlock();
    }
    return result;
}

// ------------------------------------------------------------ inline assembly
int physRegFromName(const std::string &n);

static void genAsm(Node *n) {
    Ins ins;
    ins.op = IR_ASM;
    ins.text = n->label;
    ins.line = curLine;
    struct Post { LV lv; int vreg; Type *ty; };
    std::vector<Post> posts;
    for (size_t k = 0; k < n->body.size(); k++) {
        Node *e = n->body[k];
        std::string c = n->asmConstraints[k];
        AsmOpd op;
        bool isOut = (int)k < n->asmNumOut;
        bool inout = c.find('+') != std::string::npos;
        std::string clean;
        for (char ch : c)
            if (!strchr("=+&%*?!", ch)) clean += ch;
        op.constraint = clean;
        op.isOut = isOut;
        op.isIn = !isOut || inout;
        op.size = tsize(e->ty) > 4 ? 4 : tsize(e->ty);
        bool wantsReg = clean.find_first_of("darDAR") != std::string::npos;
        bool allowsMem = clean.find_first_of("mog<>") != std::string::npos;
        bool allowsImm = clean.find_first_of("ins") != std::string::npos;
        bool lval = e->kind == ND_VAR || e->kind == ND_DEREF || e->kind == ND_MEMBER;
        if (lval) {
            LV lv = genLval(e);
            if (lv.kind == LV::REG && (wantsReg || !allowsMem)) {
                op.val = Opd::reg(lv.vreg);
            } else if (lv.kind == LV::MEM && allowsMem && (!wantsReg || isOut)) {
                op.isMem = true;
                op.val = lv.addr;
                op.off = lv.off;
            } else {
                // go through a temporary register
                int t = tmp(irt(e->ty), e->ty->kind == TY_PTR);
                if (op.isIn) {
                    Opd v = loadLV(lv);
                    Ins &m = emit(IR_MOV);
                    m.ty = irt(e->ty);
                    m.dst = t;
                    m.a = v;
                }
                op.val = Opd::reg(t);
                if (isOut) posts.push_back({lv, t, e->ty});
            }
        } else {
            Opd v = genExpr(e);
            if (v.isConst() && allowsImm) op.val = v;
            else if (v.isReg() && wantsReg) {
                // copy so the template may clobber it safely
                op.val = mov(v, irt(e->ty), e->ty->kind == TY_PTR);
            } else
                op.val = mov(v, irt(e->ty), e->ty->kind == TY_PTR);
        }
        ins.asmOps.push_back(op);
    }
    for (auto &cl : n->asmClobbers) {
        int r = physRegFromName(cl);
        if (r >= 0) ins.clobbers.push_back(r);
    }
    cur->ins.push_back(ins);
    for (auto &p : posts) storeLV(p.lv, Opd::reg(p.vreg));
}

// ------------------------------------------------------------ expressions
static Opd genExpr(Node *n) {
    if (n->tok) curLine = n->tok->line;
    Type *ty = n->ty;
    switch (n->kind) {
    case ND_NULL_EXPR: return Opd();
    case ND_NUM:
        if (tsize(ty) == 8) return wideConst((uint64_t)n->ival);
        return Opd::imm(norm(n->ival, irt(ty)));
    case ND_FNUM:
        if (ty->kind == TY_FLOAT) return Opd::imm((int32_t)floatBits((float)n->fval));
        return wideConst(doubleBits(n->fval));
    case ND_VAR: case ND_DEREF: case ND_MEMBER: {
        if (ty->kind == TY_FUNC) {
            LV lv = genLval(n);
            return addrAdd(lv.addr, lv.off);
        }
        LV lv = genLval(n);
        return loadLV(lv);
    }
    case ND_ADDR: {
        Node *l = n->lhs;
        if (l->kind == ND_VAR && l->var->isFunction) {
            referencedSymbols.insert(l->var->asmName);
            return Opd::symbol(l->var->asmName);
        }
        LV lv = genLval(l);
        if (lv.kind != LV::MEM) errorAt(n->tok, "cannot take the address of a register variable");
        return addrAdd(lv.addr, lv.off);
    }
    case ND_ASSIGN: {
        if (isAgg(ty) || isWide(ty)) {
            Opd src = genExpr(n->rhs);
            LV lv = genLval(n->lhs);
            storeLV(lv, src);
            return addrAdd(lv.addr, lv.off);
        }
        LV lv = genLval(n->lhs);
        Opd v = genExpr(n->rhs);
        if (lv.kind == LV::REG && v.isReg() && F->vregFixed[lv.vreg] < 0) {
            // retarget the instruction that just computed v (single-use temporary)
            if (!cur->ins.empty() && cur->ins.back().dst == v.v && cur->ins.back().op != IR_CALL &&
                F->vregTy[v.v] == F->vregTy[lv.vreg] && v.v != lv.vreg && cur->ins.back().op != IR_ASM) {
                bool isTemp = true;
                for (Obj *o : curObj->locals)
                    if (o->vreg == v.v) { isTemp = false; break; }
                if (isTemp) {
                    cur->ins.back().dst = lv.vreg;
                    return Opd::reg(lv.vreg);
                }
            }
        }
        storeLV(lv, v);
        if (lv.kind == LV::REG) return Opd::reg(lv.vreg);
        return v;
    }
    case ND_ASSIGNOP: {
        LV lv = genLval(n->lhs);
        Type *lt = n->lhs->ty;
        Type *ot = n->opTy;
        bool narrowOk = isInteger(lt) && tsize(lt) < 4 && isInteger(ot) && tsize(ot) == 4 && lt->kind != TY_BOOL &&
                        (n->op == ND_ADD || n->op == ND_SUB || n->op == ND_AND || n->op == ND_OR || n->op == ND_XOR || n->op == ND_MUL);
        if (narrowOk) {
            IRT w = irt(lt);
            Opd old = loadLV(lv);
            Opd r = genNarrow(n->rhs, w);
            IROp o = n->op == ND_ADD ? IR_ADD : n->op == ND_SUB ? IR_SUB : n->op == ND_AND ? IR_AND : n->op == ND_OR ? IR_OR : n->op == ND_XOR ? IR_XOR : IR_MUL;
            Opd res = bin(o, w, old, r);
            storeLV(lv, res);
            return res;
        }
        Opd old = loadLV(lv);
        Opd a = genCast(old, lt, ot);
        Opd r = genExpr(n->rhs);
        Opd res = arith(n->op, ot, a, r, n->tok);
        Opd val = genCast(res, ot, lt);
        storeLV(lv, val);
        return val;
    }
    case ND_POSTINC: {
        LV lv = genLval(n->lhs);
        Type *lt = n->lhs->ty;
        Opd old = loadLV(lv);
        if (isFlonum(lt) || tsize(lt) == 8) {
            Opd saved;
            if (tsize(lt) == 8) { saved = newWideSlot(); copyBlock(saved, old, 8); }
            else saved = mov(old, I32);
            Node one(ND_NUM);
            one.ty = tyInt;
            one.ival = n->ival;
            Opd delta = genCast(Opd::imm(n->ival), tyInt, lt);
            Opd res = arith(ND_ADD, lt, saved, delta, n->tok);
            storeLV(lv, res);
            return saved;
        }
        IRT w = irt(lt);
        Opd saved = old;
        if (lv.kind == LV::REG) saved = mov(old, w, lt->kind == TY_PTR);
        Opd res = bin(IR_ADD, w, old, Opd::imm(n->ival), lt->kind == TY_PTR);
        storeLV(lv, res);
        return saved;
    }
    case ND_NEG: {
        Opd v = genExpr(n->lhs);
        if (ty->kind == TY_FLOAT) return bin(IR_XOR, I32, v, Opd::imm((int32_t)0x80000000));
        if (ty->kind == TY_DOUBLE) {
            Opd r = newWideSlot();
            storeMem(r, 0, I32, bin(IR_XOR, I32, wideHalf(v, 0), Opd::imm((int32_t)0x80000000)));
            storeMem(r, 4, I32, wideHalf(v, 1));
            return r;
        }
        if (tsize(ty) == 8) return callHelper("__negdi2", {{v, true}}, 8);
        return un(IR_NEG, irt(ty), v);
    }
    case ND_BITNOT: {
        Opd v = genExpr(n->lhs);
        if (tsize(ty) == 8) {
            Opd r = newWideSlot();
            storeMem(r, 0, I32, un(IR_NOT, I32, wideHalf(v, 0)));
            storeMem(r, 4, I32, un(IR_NOT, I32, wideHalf(v, 1)));
            return r;
        }
        return un(IR_NOT, irt(ty), v);
    }
    case ND_NOT: case ND_LOGAND: case ND_LOGOR:
        return genBoolValue(n);
    case ND_EQ: case ND_NE: case ND_LT: case ND_LE: {
        Type *oty = n->lhs->ty;
        Opd a, b;
        IRT cty = I32;
        CC cc;
        if (isFlonum(oty) || tsize(oty) == 8) {
            int64_t rhs;
            a = wideCmp(n, &cc, &rhs);
            b = Opd::imm(rhs);
        } else
            cmpOperands(n, &a, &b, &cty, &cc);
        if (a.isConst() && b.isConst()) return genBoolValue(n);
        if (a.isConst()) { std::swap(a, b); cc = ccSwap(cc); }
        int d = tmp(I32);
        Ins &i = emit(IR_SETCC);
        i.ty = cty;
        i.cc = cc;
        i.dst = d;
        i.a = a;
        i.b = b;
        return Opd::reg(d);
    }
    case ND_ADD: case ND_SUB: case ND_MUL: case ND_DIV: case ND_MOD: case ND_AND: case ND_OR: case ND_XOR:
    case ND_SHL: case ND_SHR: {
        Opd a = genExpr(n->lhs);
        Opd b = genExpr(n->rhs);
        return arith(n->kind, ty, a, b, n->tok);
    }
    case ND_COND: {
        Block *t = newBlock(), *f = newBlock(), *end = newBlock();
        if (ty->kind == TY_VOID) {
            genCond(n->cond, t, f);
            setBlock(t);
            genExpr(n->then);
            jmp(end);
            setBlock(f);
            genExpr(n->els);
            setBlock(end);
            return Opd();
        }
        bool mem = isAgg(ty) || isWide(ty);
        IRT w = mem ? I32 : irt(ty);
        int d = tmp(w, mem || ty->kind == TY_PTR);
        genCond(n->cond, t, f);
        setBlock(t);
        { Opd v = genExpr(n->then); Ins &i = emit(IR_MOV); i.ty = w; i.dst = d; i.a = v; }
        jmp(end);
        setBlock(f);
        { Opd v = genExpr(n->els); Ins &i = emit(IR_MOV); i.ty = w; i.dst = d; i.a = v; }
        setBlock(end);
        return Opd::reg(d);
    }
    case ND_COMMA:
        genExpr(n->lhs);
        return genExpr(n->rhs);
    case ND_CALL: return genCall(n);
    case ND_CAST: {
        Type *from = n->lhs->ty;
        if (ty->kind != TY_VOID && ty->kind != TY_BOOL && (isInteger(ty) || ty->kind == TY_PTR) && (isInteger(from) || from->kind == TY_PTR) &&
            tsize(ty) < tsize(from) && tsize(from) <= 4)
            return genNarrow(n->lhs, irt(ty));
        Opd v = genExpr(n->lhs);
        return genCast(v, from, ty);
    }
    case ND_STMT_EXPR: {
        Opd last;
        for (size_t i = 0; i < n->body.size(); i++) {
            Node *s = n->body[i];
            if (i + 1 == n->body.size() && s->kind == ND_EXPR_STMT) last = genExpr(s->lhs);
            else genStmt(s);
        }
        return last;
    }
    case ND_MEMZERO: {
        Obj *v = n->var;
        if (v->slot >= 0) {
            Ins &i = emit(IR_ZEROBLK);
            i.a = Opd::slot(v->slot);
            i.off = tsize(v->ty);
            i.ty = I16;
        } else if (v->vreg >= 0) {
            Ins &i = emit(IR_MOV);
            i.ty = F->vregTy[v->vreg];
            i.dst = v->vreg;
            i.a = Opd::imm(0);
        }
        return Opd();
    }
    case ND_VA_START: {
        LV lv = genLval(n->lhs);
        storeLV(lv, Opd::slot(vaSlot));
        return Opd();
    }
    case ND_ALLOCA: {
        Opd sz = genExpr(n->lhs);
        int d = tmp(I32, true);
        Ins &i = emit(IR_ALLOCA);
        i.dst = d;
        i.a = sz;
        F->hasAlloca = true;
        return Opd::reg(d);
    }
    case ND_UNREACHABLE:
        emit(IR_UNREACH);
        freshBlock();
        return Opd();
    default:
        errorAt(n->tok, "internal: unexpected node %d in expression", (int)n->kind);
    }
}

// ------------------------------------------------------------ statements
static Block *blockForId(int id) {
    auto it = idBlocks.find(id);
    if (it != idBlocks.end()) return it->second;
    return idBlocks[id] = newBlock();
}
static Block *blockForLabel(const std::string &name) {
    auto it = labelBlocks.find(name);
    if (it != labelBlocks.end()) return it->second;
    return labelBlocks[name] = newBlock();
}

static void genReturn(Node *n) {
    Type *rt = curObj->ty->base;
    if (n->lhs && (rt->kind == TY_VOID)) {
        genExpr(n->lhs);
        emit(IR_RET);
        freshBlock();
        return;
    }
    if (!n->lhs) {
        emit(IR_RET);
        freshBlock();
        return;
    }
    Opd v = genExpr(n->lhs);
    if (F->isPascal) {
        if (pascalResultSlot < 0) errorAt(n->tok, "internal: pascal result slot missing");
        if (isAgg(rt) || isWide(rt)) {
            if (F->sretVreg >= 0) copyBlock(Opd::reg(F->sretVreg), v, tsize(rt), talign(rt));
            else copyBlock(Opd::slot(pascalResultSlot), v, tsize(rt), talign(rt));
        } else
            storeMem(Opd::slot(pascalResultSlot), 0, irt(rt), v);
        emit(IR_RET);
        freshBlock();
        return;
    }
    if (isWide(rt)) {
        Ins &i = emit(IR_RET);
        i.a = v;
        i.off = 8;
    } else if (isAgg(rt)) {
        copyBlock(Opd::reg(F->sretVreg), v, tsize(rt), talign(rt));
        Ins &i = emit(IR_RET);
        i.a = Opd::reg(F->sretVreg);
        i.ty = I32;
        i.off = 4;
    } else {
        Opd x = ext(v, irt(rt), I32, isSigned(rt));
        Ins &i = emit(IR_RET);
        i.a = x;
        i.ty = I32;
        i.off = 4;
    }
    freshBlock();
}

static void genStmt(Node *n) {
    if (n->tok) curLine = n->tok->line;
    switch (n->kind) {
    case ND_BLOCK:
        for (Node *s : n->body) genStmt(s);
        return;
    case ND_EXPR_STMT:
        genExpr(n->lhs);
        return;
    case ND_RETURN:
        genReturn(n);
        return;
    case ND_IF: {
        Block *t = newBlock(), *end = newBlock();
        Block *f = n->els ? newBlock() : end;
        genCond(n->cond, t, f);
        setBlock(t);
        genStmt(n->then);
        if (n->els) {
            jmp(end);
            setBlock(f);
            genStmt(n->els);
        }
        setBlock(end);
        return;
    }
    case ND_FOR: {
        if (n->init) genStmt(n->init);
        Block *body = newBlock(), *cont = blockForId(n->contId), *exit = blockForId(n->brkId);
        Block *cond = n->cond ? newBlock() : body;
        // rotated loop: test at the bottom
        if (n->cond) {
            // evaluate the condition once up front when it is cheap to duplicate? keep simple: jump to test
            jmp(cond);
        }
        setBlock(body);
        genStmt(n->then);
        setBlock(cont);
        if (n->inc) genExpr(n->inc);
        if (n->cond) {
            setBlock(cond);
            genCond(n->cond, body, exit);
        } else
            jmp(body);
        setBlock(exit);
        return;
    }
    case ND_DO: {
        Block *body = newBlock(), *cont = blockForId(n->contId), *exit = blockForId(n->brkId);
        setBlock(body);
        genStmt(n->then);
        setBlock(cont);
        genCond(n->cond, body, exit);
        setBlock(exit);
        return;
    }
    case ND_SWITCH: {
        Opd v = genExpr(n->cond);
        Block *exit = blockForId(n->brkId);
        Block *def = n->defaultCase ? blockForId(n->defaultCase->labelId) : exit;
        Ins sw;
        sw.op = IR_SWITCH;
        sw.ty = I32;
        sw.line = curLine;
        if (v.isConst()) {
            // constant selector: direct jump
            Block *target = def;
            for (Node *c : n->cases)
                if (v.c >= c->ival && v.c <= c->ival2) target = blockForId(c->labelId);
            jmp(target);
        } else {
            for (Node *c : n->cases) {
                Block *cb = blockForId(c->labelId);
                if (c->ival2 - c->ival > 64) {
                    // wide range: explicit range test
                    Block *next = newBlock(), *mid = newBlock();
                    br(CC_GE, I32, v, Opd::imm(c->ival), mid, next);
                    setBlock(mid);
                    br(CC_LE, I32, v, Opd::imm(c->ival2), cb, next);
                    setBlock(next);
                } else {
                    for (int64_t x = c->ival; x <= c->ival2; x++) sw.cases.push_back({x, cb->id});
                }
            }
            if (sw.cases.empty()) jmp(def);
            else {
                sw.a = v;
                sw.t1 = def->id;
                cur->ins.push_back(sw);
            }
        }
        freshBlock();
        genStmt(n->then);
        setBlock(exit);
        return;
    }
    case ND_CASE:
        setBlock(blockForId(n->labelId));
        genStmt(n->lhs);
        return;
    case ND_GOTO: {
        Block *b = n->label.empty() ? blockForId(n->labelId) : blockForLabel(n->label);
        jmp(b);
        freshBlock();
        return;
    }
    case ND_LABEL:
        setBlock(blockForLabel(n->label));
        genStmt(n->lhs);
        return;
    case ND_ASM:
        genAsm(n);
        return;
    default:
        errorAt(n->tok, "internal: unexpected statement node %d", (int)n->kind);
    }
}

// ------------------------------------------------------------ function
static bool needsSlot(Obj *v) {
    Type *t = v->ty;
    if (isAgg(t) || isWide(t)) return true;
    if (v->addrTaken) return true;
    if (t->isVolatile) return true;
    return false;
}

Func *genIR(Obj *fn) {
    F = new Func;
    F->obj = fn;
    F->name = fn->asmName;
    curObj = fn;
    labelBlocks.clear();
    idBlocks.clear();
    pascalResultSlot = -1;
    vaSlot = -1;
    curLine = fn->tok ? fn->tok->line : 0;
    Type *fty = fn->ty;
    F->isPascal = fty->cc == CC_PASCAL;
    Block *entry = newBlock();
    F->blocks.push_back(entry);
    cur = entry;

    if (fn->regParams.present) errorAt(fn->tok, "functions with '#pragma parameter' cannot be defined in C");
    Type *rt = fty->base;
    bool sret = !F->isPascal && isAgg(rt);
    bool pascalBigRet = F->isPascal && (isWide(rt) || (isAgg(rt) && tsize(rt) > 4));

    // incoming argument layout
    std::vector<int> offs(fn->params.size()), sizes(fn->params.size());
    int total = 0;
    if (F->isPascal) {
        // pushed left to right: the last parameter is nearest the return address
        int off = 0;
        for (size_t i = fn->params.size(); i-- > 0;) {
            int s = pascalArgSize(fn->params[i]->ty);
            sizes[i] = s;
            offs[i] = off;
            off += s;
        }
        if (pascalBigRet) off += 4; // hidden result pointer pushed first
        total = off;
    } else {
        int off = 0;
        for (size_t i = 0; i < fn->params.size(); i++) {
            Type *pt = fn->params[i]->ty;
            int s = isWide(pt) ? 8 : isAgg(pt) ? (tsize(pt) + 3) & ~3 : 4;
            sizes[i] = s;
            offs[i] = off;
            off += s;
        }
        total = off;
    }
    F->argBytes = total;
    if (sret) {
        F->sretVreg = tmp(I32, true);
        Ins &i = emit(IR_GETREG);
        i.dst = F->sretVreg;
        i.reg = 9; // a1
    }
    for (size_t i = 0; i < fn->params.size(); i++) {
        Obj *p = fn->params[i];
        Type *pt = p->ty;
        int vs = tsize(pt);
        int valOff = offs[i];
        bool byRef = false;
        if (F->isPascal) {
            if (isAgg(pt) && vs > 4) byRef = true;
            // byte parameters occupy the high byte of their word: no adjustment
        } else if (!isAgg(pt) && !isWide(pt) && vs < 4)
            valOff += 4 - vs;
        int slot = F->newSlot(byRef ? 4 : vs, 2);
        F->slots[slot].argOffset = valOff;
        if (byRef) {
            // the caller passed the address of the aggregate: copy it into a local
            p->slot = F->newSlot(vs, 2);
            Opd addr = loadMem(Opd::slot(slot), 0, I32, false, true);
            copyBlock(Opd::slot(p->slot), addr, vs, talign(pt));
        } else if (needsSlot(p)) {
            p->slot = slot;
        } else {
            p->vreg = tmp(irt(pt), pt->kind == TY_PTR);
            Ins &ld = emit(IR_LOAD);
            ld.ty = irt(pt);
            ld.dst = p->vreg;
            ld.a = Opd::slot(slot);
        }
    }
    if (fty->variadic) {
        vaSlot = F->newSlot(0, 2);
        F->slots[vaSlot].argOffset = total;
    }
    if (F->isPascal && rt->kind != TY_VOID) {
        if (pascalBigRet) {
            int ps = F->newSlot(4, 2);
            F->slots[ps].argOffset = total - 4;
            F->sretVreg = tmp(I32, true);
            Ins &ld = emit(IR_LOAD);
            ld.ty = I32;
            ld.dst = F->sretVreg;
            ld.a = Opd::slot(ps);
            pascalResultSlot = ps;
        } else {
            int rs = isAgg(rt) ? (tsize(rt) + 1) & ~1 : (tsize(rt) <= 2 ? 2 : 4);
            pascalResultSlot = F->newSlot(rs, 2);
            F->slots[pascalResultSlot].argOffset = total;
            F->pascalRetSize = rs;
        }
    }
    // locals
    for (Obj *v : fn->locals) {
        if (v->isParam) continue;
        v->vreg = -1;
        v->slot = -1;
        if (v->regAsm >= 0) {
            v->vreg = tmp(irt(v->ty), v->ty->kind == TY_PTR);
            F->vregFixed[v->vreg] = v->regAsm;
        } else if (needsSlot(v)) {
            int al = v->align ? v->align : talign(v->ty);
            v->slot = F->newSlot(tsize(v->ty), al < 2 && tsize(v->ty) > 1 ? 2 : al);
        } else
            v->vreg = tmp(irt(v->ty), v->ty->kind == TY_PTR);
    }
    genStmt(fn->body);
    if (!terminated()) {
        // falling off the end: main returns 0
        Ins &i = emit(IR_RET);
        if (fn->name == "main" && rt->kind != TY_VOID) {
            i.a = Opd::imm(0);
            i.ty = I32;
            i.off = 4;
        }
    }
    // place any label blocks that were referenced but never defined
    for (auto &kv : labelBlocks) {
        bool placed = std::find(F->blocks.begin(), F->blocks.end(), kv.second) != F->blocks.end();
        if (!placed) errorAt(fn->tok, "use of undeclared label '%s'", kv.first.c_str());
    }
    for (auto &kv : idBlocks) {
        bool placed = std::find(F->blocks.begin(), F->blocks.end(), kv.second) != F->blocks.end();
        if (!placed) {
            F->blocks.push_back(kv.second);
            Ins i;
            i.op = IR_UNREACH;
            kv.second->ins.push_back(i);
        }
    }
    return F;
}

} // namespace p68
