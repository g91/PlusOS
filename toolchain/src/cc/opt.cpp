// opt.cpp - IR utilities and machine-independent optimisation passes.
#include "cc.h"

namespace p68 {

std::map<std::string, Func *> inlineCandidates;

CC ccNegate(CC c) {
    switch (c) {
    case CC_EQ: return CC_NE;
    case CC_NE: return CC_EQ;
    case CC_LT: return CC_GE;
    case CC_LE: return CC_GT;
    case CC_GT: return CC_LE;
    case CC_GE: return CC_LT;
    case CC_ULT: return CC_UGE;
    case CC_ULE: return CC_UGT;
    case CC_UGT: return CC_ULE;
    case CC_UGE: return CC_ULT;
    }
    return c;
}
CC ccSwap(CC c) {
    switch (c) {
    case CC_LT: return CC_GT;
    case CC_LE: return CC_GE;
    case CC_GT: return CC_LT;
    case CC_GE: return CC_LE;
    case CC_ULT: return CC_UGT;
    case CC_ULE: return CC_UGE;
    case CC_UGT: return CC_ULT;
    case CC_UGE: return CC_ULE;
    default: return c;
    }
}

static int64_t norm(int64_t c, IRT t) {
    switch (t) {
    case I8: return (int8_t)c;
    case I16: return (int16_t)c;
    default: return (int32_t)c;
    }
}

// ------------------------------------------------------------ operand iteration
template <class Fn> static void forEachUse(Ins &i, Fn fn) {
    if (i.op == IR_ASM) {
        for (auto &o : i.asmOps)
            if (o.isMem || o.isIn) fn(o.val);
        return;
    }
    fn(i.a);
    fn(i.b);
    if (i.call) {
        fn(i.call->target);
        for (auto &a : i.call->args) fn(a.val);
        fn(i.call->retSlot);
        fn(i.call->sret);
    }
}
template <class Fn> static void forEachDef(Ins &i, Fn fn) {
    if (i.op == IR_ASM) {
        for (auto &o : i.asmOps)
            if (o.isOut && !o.isMem && o.val.isReg()) fn(o.val.v);
        return;
    }
    if (i.dst >= 0) fn(i.dst);
}

static bool isTerminator(IROp op) { return op == IR_JMP || op == IR_BR || op == IR_RET || op == IR_SWITCH || op == IR_UNREACH; }

static bool isPure(const Ins &i) {
    switch (i.op) {
    case IR_MOV: case IR_ADD: case IR_SUB: case IR_MUL: case IR_AND: case IR_OR: case IR_XOR: case IR_SHL:
    case IR_SHRS: case IR_SHRU: case IR_NEG: case IR_NOT: case IR_SEXT: case IR_ZEXT: case IR_TRUNC: case IR_SETCC:
    case IR_DIVS: case IR_DIVU: case IR_MODS: case IR_MODU:
        return true;
    case IR_LOAD: return !i.isVolatile;
    default: return false;
    }
}

// ------------------------------------------------------------ CFG
void computeCFG(Func *f) {
    for (Block *b : f->allBlocks) {
        b->preds.clear();
        b->succs.clear();
        b->reachable = false;
    }
    // make sure every block ends in a terminator (fall through to the next block in layout)
    for (size_t k = 0; k < f->blocks.size(); k++) {
        Block *b = f->blocks[k];
        if (b->ins.empty() || !isTerminator(b->ins.back().op)) {
            Ins j;
            if (k + 1 < f->blocks.size()) { j.op = IR_JMP; j.t1 = f->blocks[k + 1]->id; }
            else j.op = IR_RET;
            b->ins.push_back(j);
        }
    }
    auto succsOf = [&](Block *b, std::vector<int> &out) {
        Ins &t = b->ins.back();
        if (t.op == IR_JMP) out.push_back(t.t1);
        else if (t.op == IR_BR) { out.push_back(t.t1); if (t.t2 != t.t1) out.push_back(t.t2); }
        else if (t.op == IR_SWITCH) {
            out.push_back(t.t1);
            for (auto &c : t.cases)
                if (std::find(out.begin(), out.end(), c.second) == out.end()) out.push_back(c.second);
        }
    };
    std::vector<Block *> work;
    f->blocks[0]->reachable = true;
    work.push_back(f->blocks[0]);
    while (!work.empty()) {
        Block *b = work.back();
        work.pop_back();
        succsOf(b, b->succs);
        for (int s : b->succs) {
            Block *sb = f->allBlocks[s];
            sb->preds.push_back(b->id);
            if (!sb->reachable) { sb->reachable = true; work.push_back(sb); }
        }
    }
    std::vector<Block *> keep;
    for (Block *b : f->blocks)
        if (b->reachable) keep.push_back(b);
    f->blocks = keep;
}

static void retarget(Ins &t, int from, int to) {
    if (t.t1 == from) t.t1 = to;
    if (t.t2 == from) t.t2 = to;
    for (auto &c : t.cases)
        if (c.second == from) c.second = to;
}

static bool cleanupCFG(Func *f) {
    bool changed = false;
    computeCFG(f);
    // jump threading: blocks that contain only a JMP
    for (bool again = true; again;) {
        again = false;
        for (Block *b : f->blocks) {
            if (b == f->blocks[0]) continue;
            if (b->ins.size() == 1 && b->ins[0].op == IR_JMP && b->ins[0].t1 != b->id) {
                int target = b->ins[0].t1;
                for (int p : b->preds) retarget(f->allBlocks[p]->ins.back(), b->id, target);
                if (!b->preds.empty()) { again = true; changed = true; }
            }
        }
        // simplify degenerate branches
        for (Block *b : f->blocks) {
            Ins &t = b->ins.back();
            if (t.op == IR_BR && t.t1 == t.t2) {
                Ins j;
                j.op = IR_JMP;
                j.t1 = t.t1;
                b->ins.back() = j;
                changed = true;
            }
        }
        computeCFG(f);
    }
    // merge a block into its single predecessor
    for (bool again = true; again;) {
        again = false;
        for (Block *b : f->blocks) {
            Ins &t = b->ins.back();
            if (t.op != IR_JMP) continue;
            Block *s = f->allBlocks[t.t1];
            if (s == b || s == f->blocks[0] || s->preds.size() != 1) continue;
            b->ins.pop_back();
            b->ins.insert(b->ins.end(), s->ins.begin(), s->ins.end());
            s->ins.clear();
            Ins u;
            u.op = IR_UNREACH;
            s->ins.push_back(u);
            again = true;
            changed = true;
            computeCFG(f);
            break;
        }
    }
    // loop depth estimate from backward edges in layout order
    std::map<int, int> pos;
    for (size_t i = 0; i < f->blocks.size(); i++) { pos[f->blocks[i]->id] = (int)i; f->blocks[i]->loopDepth = 0; }
    for (size_t i = 0; i < f->blocks.size(); i++)
        for (int s : f->blocks[i]->succs) {
            int sp = pos[s];
            if (sp <= (int)i)
                for (int k = sp; k <= (int)i; k++) f->blocks[k]->loopDepth++;
        }
    return changed;
}

// ------------------------------------------------------------ liveness
struct Bits {
    std::vector<uint64_t> w;
    void init(size_t n) { w.assign((n + 63) / 64, 0); }
    bool get(int i) const { return (w[i >> 6] >> (i & 63)) & 1; }
    void set(int i) { w[i >> 6] |= 1ull << (i & 63); }
    void clr(int i) { w[i >> 6] &= ~(1ull << (i & 63)); }
    bool orWith(const Bits &o) {
        bool ch = false;
        for (size_t i = 0; i < w.size(); i++) {
            uint64_t n = w[i] | o.w[i];
            if (n != w[i]) { w[i] = n; ch = true; }
        }
        return ch;
    }
};

static void liveness(Func *f, std::vector<Bits> &liveOut) {
    size_t nv = f->vregTy.size();
    size_t nb = f->allBlocks.size();
    std::vector<Bits> use(nb), def(nb), liveIn(nb);
    liveOut.assign(nb, Bits());
    for (Block *b : f->blocks) {
        use[b->id].init(nv);
        def[b->id].init(nv);
        liveIn[b->id].init(nv);
        liveOut[b->id].init(nv);
        for (Ins &i : b->ins) {
            forEachUse(i, [&](Opd &o) {
                if (o.isReg() && !def[b->id].get(o.v)) use[b->id].set(o.v);
            });
            forEachDef(i, [&](int d) { def[b->id].set(d); });
        }
    }
    for (bool changed = true; changed;) {
        changed = false;
        for (size_t k = f->blocks.size(); k-- > 0;) {
            Block *b = f->blocks[k];
            Bits &out = liveOut[b->id];
            for (int s : b->succs) out.orWith(liveIn[s]);
            Bits in = out;
            for (size_t i = 0; i < in.w.size(); i++) in.w[i] = use[b->id].w[i] | (out.w[i] & ~def[b->id].w[i]);
            if (liveIn[b->id].orWith(in)) changed = true;
        }
    }
}

static bool dce(Func *f) {
    bool changed = false;
    std::vector<Bits> liveOut;
    liveness(f, liveOut);
    for (Block *b : f->blocks) {
        Bits live = liveOut[b->id];
        for (size_t k = b->ins.size(); k-- > 0;) {
            Ins &i = b->ins[k];
            bool dead = false;
            if (i.dst >= 0 && i.op != IR_ASM && !live.get(i.dst) && f->vregFixed[i.dst] < 0) {
                if (isPure(i)) dead = true;
                else if (i.op == IR_CALL) { i.dst = -1; changed = true; }
            }
            if (i.op == IR_MOV && i.a.isReg() && i.a.v == i.dst) dead = true;
            if (dead) {
                i.op = IR_NOP;
                i.dst = -1;
                i.a = Opd();
                i.b = Opd();
                changed = true;
                continue;
            }
            forEachDef(i, [&](int d) { live.clr(d); });
            forEachUse(i, [&](Opd &o) { if (o.isReg()) live.set(o.v); });
        }
        size_t n = 0;
        for (size_t k = 0; k < b->ins.size(); k++)
            if (b->ins[k].op != IR_NOP) {
                if (n != k) b->ins[n] = b->ins[k];
                n++;
            }
        b->ins.resize(n);
    }
    return changed;
}

// ------------------------------------------------------------ constant folding helpers
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

static bool evalCC(CC cc, IRT ty, int64_t a, int64_t b) {
    int64_t x = norm(a, ty), y = norm(b, ty);
    uint32_t mask = ty == I8 ? 0xFF : ty == I16 ? 0xFFFF : 0xFFFFFFFFu;
    uint32_t ux = (uint32_t)x & mask, uy = (uint32_t)y & mask;
    switch (cc) {
    case CC_EQ: return x == y;
    case CC_NE: return x != y;
    case CC_LT: return x < y;
    case CC_LE: return x <= y;
    case CC_GT: return x > y;
    case CC_GE: return x >= y;
    case CC_ULT: return ux < uy;
    case CC_ULE: return ux <= uy;
    case CC_UGT: return ux > uy;
    case CC_UGE: return ux >= uy;
    }
    return false;
}

static int log2Exact(int64_t v) {
    if (v <= 0 || (v & (v - 1))) return -1;
    int n = 0;
    while ((v >> n) != 1) n++;
    return n;
}

// Simplify one instruction in place. Returns true if it changed.
static bool simplify(Func *f, Ins &i) {
    auto toMov = [&](Opd v) {
        i.op = IR_MOV;
        i.a = v;
        i.b = Opd();
    };
    switch (i.op) {
    case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIVS: case IR_DIVU: case IR_MODS: case IR_MODU: case IR_AND:
    case IR_OR: case IR_XOR: case IR_SHL: case IR_SHRS: case IR_SHRU: {
        if (i.a.isConst() && i.b.isConst()) {
            int64_t r;
            if (foldBin(i.op, i.ty, i.a.c, i.b.c, &r)) { toMov(Opd::imm(r)); return true; }
            return false;
        }
        bool comm = i.op == IR_ADD || i.op == IR_MUL || i.op == IR_AND || i.op == IR_OR || i.op == IR_XOR;
        if (comm && !i.b.isConst() && (i.a.isConst() || ((i.a.kind == Opd::SYM || i.a.kind == Opd::SLOT) && i.b.isReg()))) {
            std::swap(i.a, i.b);
            return true;
        }
        if ((i.op == IR_ADD || i.op == IR_SUB) && i.ty == I32 && (i.a.kind == Opd::SYM || i.a.kind == Opd::SLOT) && i.b.isConst()) {
            Opd v = i.a;
            v.c += i.op == IR_ADD ? i.b.c : -i.b.c;
            toMov(v);
            return true;
        }
        if (i.b.isConst()) {
            int64_t c = norm(i.b.c, i.ty);
            bool ident = (c == 0 && (i.op == IR_ADD || i.op == IR_SUB || i.op == IR_OR || i.op == IR_XOR || i.op == IR_SHL || i.op == IR_SHRS || i.op == IR_SHRU)) ||
                         (c == 1 && (i.op == IR_MUL || i.op == IR_DIVS || i.op == IR_DIVU)) || (c == -1 && i.op == IR_AND);
            if (ident) { toMov(i.a); return true; }
            if ((c == 0 && (i.op == IR_MUL || i.op == IR_AND)) || (c == 1 && (i.op == IR_MODS || i.op == IR_MODU))) { toMov(Opd::imm(0)); return true; }
            if (i.op == IR_SUB && c != (i.ty == I8 ? -128 : i.ty == I16 ? -32768 : INT32_MIN)) {
                i.op = IR_ADD;
                i.b = Opd::imm(-c);
                return true;
            }
            int k = log2Exact(i.ty == I32 ? (int64_t)(uint32_t)c : c & (i.ty == I8 ? 0xFF : 0xFFFF));
            if (k > 0) {
                if (i.op == IR_MUL) { i.op = IR_SHL; i.b = Opd::imm(k); return true; }
                if (i.op == IR_DIVU) { i.op = IR_SHRU; i.b = Opd::imm(k); return true; }
                if (i.op == IR_MODU) { i.op = IR_AND; i.b = Opd::imm(norm((1LL << k) - 1, i.ty)); return true; }
            }
            if (i.op == IR_MUL && c == -1) { i.op = IR_NEG; i.b = Opd(); return true; }
        }
        if (i.a.isReg() && i.b.isReg() && i.a.v == i.b.v && f->vregFixed[i.a.v] < 0) {
            if (i.op == IR_SUB || i.op == IR_XOR) { toMov(Opd::imm(0)); return true; }
            if (i.op == IR_AND || i.op == IR_OR) { toMov(i.a); return true; }
        }
        return false;
    }
    case IR_NEG: case IR_NOT:
        if (i.a.isConst()) { toMov(Opd::imm(norm(i.op == IR_NEG ? -i.a.c : ~i.a.c, i.ty))); return true; }
        return false;
    case IR_SEXT: case IR_ZEXT: case IR_TRUNC:
        if (i.a.isConst()) {
            int64_t c = norm(i.a.c, i.srcTy);
            if (i.op == IR_ZEXT) c &= (i.srcTy == I8 ? 0xFF : i.srcTy == I16 ? 0xFFFF : 0xFFFFFFFFLL);
            toMov(Opd::imm(norm(c, i.ty)));
            return true;
        }
        return false;
    case IR_SETCC:
        if (i.a.isConst() && i.b.isConst()) {
            bool r = evalCC(i.cc, i.ty, i.a.c, i.b.c);
            i.ty = f->vregTy[i.dst];
            toMov(Opd::imm(r));
            return true;
        }
        if (i.a.isConst() && !i.b.isConst()) { std::swap(i.a, i.b); i.cc = ccSwap(i.cc); return true; }
        return false;
    case IR_BR:
        if (i.a.isConst() && i.b.isConst()) {
            bool r = evalCC(i.cc, i.ty, i.a.c, i.b.c);
            int t = r ? i.t1 : i.t2;
            i = Ins();
            i.op = IR_JMP;
            i.t1 = t;
            return true;
        }
        if (i.a.isConst() && !i.b.isConst()) { std::swap(i.a, i.b); i.cc = ccSwap(i.cc); return true; }
        return false;
    case IR_SWITCH:
        if (i.a.isConst()) {
            int t = i.t1;
            for (auto &c : i.cases)
                if ((int32_t)c.first == (int32_t)i.a.c) t = c.second;
            i = Ins();
            i.op = IR_JMP;
            i.t1 = t;
            return true;
        }
        return false;
    case IR_LOAD: case IR_STORE:
        if ((i.a.kind == Opd::SYM || i.a.kind == Opd::SLOT || i.a.kind == Opd::CONST) && i.off) {
            i.a.c += i.off;
            i.off = 0;
            return true;
        }
        if (i.op == IR_STORE && i.b.isConst() && i.b.c != norm(i.b.c, i.ty)) { i.b.c = norm(i.b.c, i.ty); return true; }
        return false;
    default: return false;
    }
}

// ------------------------------------------------------------ propagation
struct DefInfo {
    std::vector<int> count;
    std::vector<Ins *> single; // defining instruction when count == 1
};
static void countDefs(Func *f, DefInfo &d) {
    d.count.assign(f->vregTy.size(), 0);
    d.single.assign(f->vregTy.size(), nullptr);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins)
            forEachDef(i, [&](int v) {
                d.count[v]++;
                d.single[v] = i.op == IR_ASM ? nullptr : &i;
            });
    for (size_t v = 0; v < d.count.size(); v++)
        if (d.count[v] != 1) d.single[v] = nullptr;
}

static bool propagate(Func *f) {
    bool changed = false;
    DefInfo di;
    countDefs(f, di);
    size_t nv = f->vregTy.size();
    // global replacement for single-definition vregs
    std::vector<Opd> repl(nv);
    for (size_t v = 0; v < nv; v++) {
        Ins *d = di.single[v];
        if (!d || d->op != IR_MOV || f->vregFixed[v] >= 0) continue;
        if (d->a.isConst() || d->a.kind == Opd::SYM || d->a.kind == Opd::SLOT) repl[v] = d->a;
        else if (d->a.isReg() && di.count[d->a.v] == 1 && f->vregFixed[d->a.v] < 0 && f->vregTy[d->a.v] == f->vregTy[v] && d->a.v != (int)v)
            repl[v] = d->a;
    }
    auto resolve = [&](Opd o) {
        for (int guard = 0; guard < 32 && o.isReg() && !repl[o.v].isNone(); guard++) o = repl[o.v];
        return o;
    };
    for (Block *b : f->blocks) {
        // local copies valid inside the block
        std::map<int, Opd> local;
        auto kill = [&](int d) {
            local.erase(d);
            for (auto it = local.begin(); it != local.end();) {
                if (it->second.isReg() && it->second.v == d) it = local.erase(it);
                else ++it;
            }
        };
        for (Ins &i : b->ins) {
            bool isAsm = i.op == IR_ASM;
            forEachUse(i, [&](Opd &o) {
                if (!o.isReg()) return;
                Opd r = resolve(o);
                if (r.isReg()) {
                    auto it = local.find(r.v);
                    if (it != local.end()) r = it->second;
                }
                if (r == o) return;
                // inline asm register operands and register call arguments need registers
                if (isAsm && !r.isReg()) return;
                if (isAsm) return;
                o = r;
                changed = true;
            });
            // operands that must stay registers: restore constants where needed
            if (i.op == IR_CALL && i.call && i.call->target.isConst()) { /* absolute call: fine */ }
            if (simplify(f, i)) changed = true;
            forEachDef(i, [&](int d) { kill(d); });
            if (i.op == IR_MOV && i.dst >= 0 && f->vregFixed[i.dst] < 0) {
                Opd s = i.a;
                bool ok = s.isConst() || s.kind == Opd::SYM || s.kind == Opd::SLOT ||
                          (s.isReg() && f->vregFixed[s.v] < 0 && f->vregTy[s.v] == f->vregTy[i.dst] && s.v != i.dst);
                if (ok) local[i.dst] = s;
            }
        }
    }
    return changed;
}

// fold  t = add p, c ; load/store [t + off]   into  [p + c + off]
static bool addrFold(Func *f) {
    bool changed = false;
    DefInfo di;
    countDefs(f, di);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            if (i.op != IR_LOAD && i.op != IR_STORE) continue;
            for (int guard = 0; guard < 8 && i.a.isReg(); guard++) {
                Ins *d = di.single[i.a.v];
                if (!d || d->op != IR_ADD || !d->b.isConst() || d->ty != I32) break;
                Opd base = d->a;
                if (base.isReg() && (di.count[base.v] != 1 || f->vregFixed[base.v] >= 0)) break;
                if (!base.isReg() && base.kind != Opd::SYM && base.kind != Opd::SLOT) break;
                int64_t noff = i.off + d->b.c;
                if (base.isReg() && (noff < -32768 || noff > 32767)) break;
                i.a = base;
                i.off = noff;
                if (!base.isReg()) { i.a.c += i.off; i.off = 0; }
                changed = true;
            }
        }
    return changed;
}

// ------------------------------------------------------------ local value numbering
static bool localCSE(Func *f) {
    bool changed = false;
    struct Key {
        IROp op; IRT ty, srcTy; CC cc; Opd a, b; int64_t off;
        bool operator==(const Key &o) const {
            return op == o.op && ty == o.ty && srcTy == o.srcTy && cc == o.cc && a == o.a && b == o.b && off == o.off;
        }
    };
    struct Entry { Key k; Opd val; };
    for (Block *b : f->blocks) {
        std::vector<Entry> table;
        auto killReg = [&](int d) {
            for (size_t k = 0; k < table.size();) {
                Entry &e = table[k];
                if ((e.k.a.isReg() && e.k.a.v == d) || (e.k.b.isReg() && e.k.b.v == d) || (e.val.isReg() && e.val.v == d)) {
                    table[k] = table.back();
                    table.pop_back();
                } else
                    k++;
            }
        };
        auto killMem = [&]() {
            for (size_t k = 0; k < table.size();) {
                if (table[k].k.op == IR_LOAD) { table[k] = table.back(); table.pop_back(); }
                else k++;
            }
        };
        auto killSlotLoads = [&](const Opd &addr, bool exactOnly) {
            // a store through an unknown pointer cannot alias a non-escaping... we do not track
            // escapes, so any store kills all loads except when both are distinct direct slots/symbols
            for (size_t k = 0; k < table.size();) {
                Entry &e = table[k];
                bool kill = false;
                if (e.k.op == IR_LOAD) {
                    bool directA = addr.kind == Opd::SLOT || addr.kind == Opd::SYM;
                    bool directB = e.k.a.kind == Opd::SLOT || e.k.a.kind == Opd::SYM;
                    if (directA && directB) {
                        if (addr.kind != e.k.a.kind) kill = false;
                        else if (addr.kind == Opd::SLOT && addr.v != e.k.a.v) kill = false;
                        else if (addr.kind == Opd::SYM && addr.sym != e.k.a.sym) kill = false;
                        else kill = true; // same object: be conservative about overlap
                    } else
                        kill = true;
                }
                (void)exactOnly;
                if (kill) { table[k] = table.back(); table.pop_back(); }
                else k++;
            }
        };
        for (Ins &i : b->ins) {
            if (i.op == IR_ASM || i.op == IR_CALL || i.op == IR_COPYBLK || i.op == IR_ZEROBLK || i.op == IR_ALLOCA || i.op == IR_SETREG) {
                killMem();
                forEachDef(i, [&](int d) { killReg(d); });
                continue;
            }
            if (i.op == IR_STORE) {
                Opd addr = i.a;
                killSlotLoads(addr, false);
                if (!i.isVolatile && (i.b.isReg() || i.b.isConst()) && !(i.b.isReg() && f->vregFixed[i.b.v] >= 0)) {
                    Key k{IR_LOAD, i.ty, I32, CC_EQ, i.a, Opd(), i.off};
                    table.push_back({k, i.b});
                }
                continue;
            }
            if (i.dst < 0 || !isPure(i) || i.op == IR_MOV || f->vregFixed[i.dst] >= 0) {
                forEachDef(i, [&](int d) { killReg(d); });
                continue;
            }
            bool usesFixed = false;
            forEachUse(i, [&](Opd &o) { if (o.isReg() && f->vregFixed[o.v] >= 0) usesFixed = true; });
            Key k{i.op, i.ty, (i.op == IR_SEXT || i.op == IR_ZEXT || i.op == IR_TRUNC) ? i.srcTy : I32, i.op == IR_SETCC ? i.cc : CC_EQ, i.a, i.b, i.off};
            Opd found;
            if (!usesFixed)
                for (Entry &e : table)
                    if (e.k == k) { found = e.val; break; }
            int dst = i.dst;
            if (!found.isNone() && !(found.isReg() && found.v == dst)) {
                // widths must agree: a forwarded store value has the store's width
                IRT have = found.isReg() ? f->vregTy[found.v] : f->vregTy[dst];
                if (have == f->vregTy[dst]) {
                    i.op = IR_MOV;
                    i.ty = f->vregTy[dst];
                    i.a = found;
                    i.b = Opd();
                    i.off = 0;
                    changed = true;
                    killReg(dst);
                    continue;
                }
            }
            killReg(dst);
            bool selfRef = (i.a.isReg() && i.a.v == dst) || (i.b.isReg() && i.b.v == dst);
            if (!usesFixed && !selfRef) table.push_back({k, Opd::reg(dst)});
        }
    }
    return changed;
}

// ------------------------------------------------------------ slot promotion (mem2reg for simple slots)
static bool promoteSlots(Func *f) {
    size_t ns = f->slots.size();
    std::vector<int> state(ns, 0); // 0 unseen, 1 promotable with type, -1 not promotable
    std::vector<IRT> ty(ns, I32);
    auto bad = [&](const Opd &o) { if (o.kind == Opd::SLOT) state[o.v] = -1; };
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            if ((i.op == IR_LOAD || i.op == IR_STORE) && i.a.kind == Opd::SLOT && !i.isVolatile) {
                int s = i.a.v;
                if (i.a.c != 0 || i.off != 0 || f->slots[s].argOffset >= 0 || f->slots[s].size != (int)i.ty) state[s] = -1;
                else if (state[s] == 0) { state[s] = 1; ty[s] = i.ty; }
                else if (state[s] == 1 && ty[s] != i.ty) state[s] = -1;
                if (i.op == IR_STORE) bad(i.b);
                continue;
            }
            forEachUse(i, [&](Opd &o) { bad(o); });
        }
    bool changed = false;
    std::vector<int> vr(ns, -1);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            if ((i.op == IR_LOAD || i.op == IR_STORE) && i.a.kind == Opd::SLOT && state[i.a.v] == 1) {
                int s = i.a.v;
                if (vr[s] < 0) vr[s] = f->newVreg(ty[s]);
                if (i.op == IR_LOAD) {
                    i.op = IR_MOV;
                    i.a = Opd::reg(vr[s]);
                } else {
                    i.op = IR_MOV;
                    i.dst = vr[s];
                    i.a = i.b;
                    i.b = Opd();
                }
                i.off = 0;
                changed = true;
            }
        }
    return changed;
}

// ------------------------------------------------------------ multiply by constant
static bool expandMul(Func *f) {
    if (opt.cpu >= 68020) return false; // muls.l is available
    bool changed = false;
    for (Block *b : f->blocks) {
        for (size_t k = 0; k < b->ins.size(); k++) {
            Ins i = b->ins[k];
            if (i.op != IR_MUL || !i.b.isConst() || !i.a.isReg()) continue;
            int64_t c = norm(i.b.c, i.ty);
            bool neg = c < 0;
            uint64_t u = (uint64_t)(neg ? -c : c);
            if (u == 0 || u > 0x10000) continue;
            int pop = 0;
            for (int bit = 0; bit < 32; bit++)
                if (u >> bit & 1) pop++;
            std::vector<Ins> seq;
            auto shl = [&](Opd x, int n) -> Opd {
                if (n == 0) return x;
                Ins s;
                s.op = IR_SHL;
                s.ty = i.ty;
                s.dst = f->newVreg(i.ty);
                s.a = x;
                s.b = Opd::imm(n);
                seq.push_back(s);
                return Opd::reg(s.dst);
            };
            auto addsub = [&](IROp op, Opd x, Opd y) -> Opd {
                Ins s;
                s.op = op;
                s.ty = i.ty;
                s.dst = f->newVreg(i.ty);
                s.a = x;
                s.b = y;
                seq.push_back(s);
                return Opd::reg(s.dst);
            };
            Opd res;
            int k1 = log2Exact((int64_t)u + 1);
            if (pop <= 3 && (i.ty == I32 || opt.optSize == false || pop <= 2)) {
                // sum of shifted copies
                for (int bit = 31; bit >= 0; bit--)
                    if (u >> bit & 1) {
                        Opd t = shl(i.a, bit);
                        res = res.isNone() ? t : addsub(IR_ADD, res, t);
                    }
            } else if (k1 > 0) {
                res = addsub(IR_SUB, shl(i.a, k1), i.a); // x * (2^k - 1)
            } else
                continue;
            if (i.ty != I32 && pop > 2) continue; // 16-bit multiplies are single instructions
            if (neg) {
                Ins s;
                s.op = IR_NEG;
                s.ty = i.ty;
                s.dst = f->newVreg(i.ty);
                s.a = res;
                seq.push_back(s);
                res = Opd::reg(s.dst);
            }
            Ins m;
            m.op = IR_MOV;
            m.ty = i.ty;
            m.dst = i.dst;
            m.a = res;
            seq.push_back(m);
            b->ins.erase(b->ins.begin() + k);
            b->ins.insert(b->ins.begin() + k, seq.begin(), seq.end());
            k += seq.size() - 1;
            changed = true;
        }
    }
    return changed;
}

// signed division / modulo by a power of two
static bool expandDiv(Func *f) {
    bool changed = false;
    for (Block *b : f->blocks) {
        for (size_t k = 0; k < b->ins.size(); k++) {
            Ins i = b->ins[k];
            if ((i.op != IR_DIVS && i.op != IR_MODS) || !i.b.isConst() || !i.a.isReg()) continue;
            int64_t c = norm(i.b.c, i.ty);
            int n = log2Exact(c);
            if (n <= 0) continue;
            int bits = i.ty * 8;
            std::vector<Ins> seq;
            auto op2 = [&](IROp op, Opd x, Opd y) -> Opd {
                Ins s;
                s.op = op;
                s.ty = i.ty;
                s.dst = f->newVreg(i.ty);
                s.a = x;
                s.b = y;
                seq.push_back(s);
                return Opd::reg(s.dst);
            };
            // bias = (x >> (bits-1)) >>> (bits - n) ;  q = (x + bias) >> n
            Opd sign = n == 1 ? i.a : op2(IR_SHRS, i.a, Opd::imm(bits - 1));
            Opd bias = op2(IR_SHRU, sign, Opd::imm(bits - n));
            Opd sum = op2(IR_ADD, i.a, bias);
            Opd res;
            if (i.op == IR_DIVS) res = op2(IR_SHRS, sum, Opd::imm(n));
            else {
                Opd masked = op2(IR_AND, sum, Opd::imm(norm(~((1LL << n) - 1), i.ty)));
                res = op2(IR_SUB, i.a, masked);
            }
            Ins m;
            m.op = IR_MOV;
            m.ty = i.ty;
            m.dst = i.dst;
            m.a = res;
            seq.push_back(m);
            b->ins.erase(b->ins.begin() + k);
            b->ins.insert(b->ins.begin() + k, seq.begin(), seq.end());
            k += seq.size() - 1;
            changed = true;
        }
    }
    return changed;
}


// ------------------------------------------------------------ extra peephole passes
static void countUses(Func *f, std::vector<int> &uses) {
    uses.assign(f->vregTy.size(), 0);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            if (i.op == IR_ASM) {
                for (auto &o : i.asmOps)
                    if (o.val.isReg()) uses[o.val.v] += 2;
                continue;
            }
            forEachUse(i, [&](Opd &o) { if (o.isReg()) uses[o.v]++; });
        }
}
static bool insUses(Ins &i, int v) {
    bool r = false;
    if (i.op == IR_ASM) {
        for (auto &o : i.asmOps)
            if (o.val.isReg() && o.val.v == v) r = true;
        return r;
    }
    forEachUse(i, [&](Opd &o) { if (o.isReg() && o.v == v) r = true; });
    return r;
}
static bool insDefs(Ins &i, int v) {
    bool r = false;
    forEachDef(i, [&](int d) { if (d == v) r = true; });
    return r;
}

//   t = OP a, b ; ... ; x = MOV t      ==>      x = OP a, b   (t used once)
static bool retargetDefs(Func *f) {
    bool changed = false;
    std::vector<int> uses;
    DefInfo di;
    countUses(f, uses);
    countDefs(f, di);
    for (Block *b : f->blocks) {
        for (size_t j = 1; j < b->ins.size(); j++) {
            Ins &mv = b->ins[j];
            if (mv.op != IR_MOV || !mv.a.isReg() || mv.dst < 0) continue;
            int t = mv.a.v, x = mv.dst;
            if (t == x || f->vregFixed[t] >= 0 || f->vregFixed[x] >= 0) continue;
            if (uses[t] != 1 || di.count[t] != 1 || f->vregTy[t] != f->vregTy[x]) continue;
            size_t i = j;
            bool ok = false;
            for (int steps = 0; i-- > 0 && steps < 50; steps++) {
                Ins &d = b->ins[i];
                if (d.op != IR_ASM && d.dst == t && d.op != IR_CALL) { ok = true; break; }
                if (insUses(d, x) || insDefs(d, x) || d.op == IR_ASM || d.op == IR_CALL) break;
            }
            if (!ok) continue;
            b->ins[i].dst = x;
            if (f->vregPtr[t]) f->vregPtr[x] = true;
            b->ins.erase(b->ins.begin() + j);
            j--;
            changed = true;
        }
    }
    return changed;
}

//   s = MOV p ; ... ; p = ADD p, c ; use of s as an address / in a compare
static bool sinkIncrements(Func *f) {
    bool changed = false;
    std::vector<int> uses;
    DefInfo di;
    countUses(f, uses);
    countDefs(f, di);
    for (Block *b : f->blocks) {
        for (size_t i = 0; i < b->ins.size(); i++) {
            Ins &mv = b->ins[i];
            if (mv.op != IR_MOV || !mv.a.isReg() || mv.dst < 0) continue;
            int s = mv.dst, p = mv.a.v;
            if (s == p || uses[s] != 1 || di.count[s] != 1 || f->vregFixed[s] >= 0 || f->vregFixed[p] >= 0) continue;
            if (f->vregTy[s] != f->vregTy[p]) continue;
            size_t j = i + 1;
            bool found = false;
            for (; j < b->ins.size() && j < i + 12; j++) {
                Ins &a = b->ins[j];
                if (a.op == IR_ADD && a.dst == p && a.a.isReg() && a.a.v == p && a.b.isConst()) { found = true; break; }
                if (insDefs(a, p) || insUses(a, s) || a.op == IR_ASM || a.op == IR_CALL) break;
            }
            if (!found) continue;
            int64_t c = b->ins[j].b.c;
            size_t k = j + 1;
            bool ok = false;
            for (; k < b->ins.size() && k < j + 12; k++) {
                Ins &u = b->ins[k];
                if (insUses(u, s)) { ok = true; break; }
                if (insUses(u, p) || insDefs(u, p) || u.op == IR_ASM || u.op == IR_CALL) break;
            }
            if (!ok) continue;
            Ins &u = b->ins[k];
            if ((u.op == IR_BR || u.op == IR_SETCC) && u.a.isReg() && u.a.v == s && u.b.isConst() && (u.cc == CC_EQ || u.cc == CC_NE)) {
                u.a = Opd::reg(p);
                u.b = Opd::imm(norm(u.b.c + c, u.ty));
                b->ins.erase(b->ins.begin() + i);
                changed = true;
                i--;
                continue;
            }
            bool addrUse = (u.op == IR_LOAD && u.a.isReg() && u.a.v == s) ||
                           (u.op == IR_STORE && u.a.isReg() && u.a.v == s && !(u.b.isReg() && (u.b.v == s || u.b.v == p)));
            if (!addrUse || insUses(u, p) || insDefs(u, p)) continue;
            u.a = Opd::reg(p);
            Ins add = b->ins[j];
            b->ins.insert(b->ins.begin() + k + 1, add);
            b->ins.erase(b->ins.begin() + j);
            b->ins.erase(b->ins.begin() + i);
            changed = true;
            i--;
        }
    }
    return changed;
}

// fold redundant integer extensions / truncations
static bool foldExtensions(Func *f) {
    bool changed = false;
    DefInfo di;
    countDefs(f, di);
    for (Block *b : f->blocks)
        for (Ins &i : b->ins) {
            if ((i.op != IR_TRUNC && i.op != IR_SEXT && i.op != IR_ZEXT) || !i.a.isReg()) continue;
            Ins *d = di.single[i.a.v];
            if (!d || f->vregFixed[i.a.v] >= 0) continue;
            if (!d->a.isReg() || di.count[d->a.v] != 1 || f->vregFixed[d->a.v] >= 0) continue;
            if (i.op == IR_TRUNC && (d->op == IR_SEXT || d->op == IR_ZEXT)) {
                if (d->srcTy == i.ty) { i.op = IR_MOV; i.a = d->a; changed = true; }
                else if (d->srcTy > i.ty) { i.srcTy = d->srcTy; i.a = d->a; changed = true; }
                else { i.op = d->op; i.srcTy = d->srcTy; i.a = d->a; changed = true; }
            } else if (i.op == IR_TRUNC && d->op == IR_TRUNC) {
                i.srcTy = d->srcTy; i.a = d->a; changed = true;
            } else if ((i.op == IR_SEXT || i.op == IR_ZEXT) && d->op == i.op && d->ty == i.srcTy) {
                i.srcTy = d->srcTy; i.a = d->a; changed = true;
            } else if (i.op == IR_SEXT && d->op == IR_ZEXT && d->ty == i.srcTy) {
                i.op = IR_ZEXT; i.srcTy = d->srcTy; i.a = d->a; changed = true;
            }
        }
    return changed;
}

// ------------------------------------------------------------ inlining
static int funcSize(Func *f) {
    int n = 0;
    for (Block *b : f->blocks) n += (int)b->ins.size();
    return n;
}

static bool canBeInlined(Func *g) {
    Obj *o = g->obj;
    if (!o || o->noInline || g->isPascal || g->hasAlloca || g->sretVreg >= 0) return false;
    if (o->ty->variadic) return false;
    for (Block *b : g->blocks)
        for (Ins &i : b->ins) {
            if (i.op == IR_CALL && i.call && i.call->target.kind == Opd::SYM && i.call->target.sym == g->name) return false; // recursive
            if (i.op == IR_RET && i.off == 8) return false;
            if (i.op == IR_GETREG || i.op == IR_SETREG) return false;
        }
    return true;
}

static bool inlineCalls(Func *f) {
    bool changed = false;
    int budget = 400;
    for (size_t bi = 0; bi < f->blocks.size(); bi++) {
        Block *b = f->blocks[bi];
        for (size_t k = 0; k < b->ins.size(); k++) {
            Ins &call = b->ins[k];
            if (call.op != IR_CALL || !call.call || call.call->target.kind != Opd::SYM) continue;
            CallInfo *ci = call.call;
            if (ci->cc != CC_C || !ci->inlineWords.empty() || ci->regParams || !ci->sret.isNone() || ci->retSize == 8) continue;
            auto it = inlineCandidates.find(ci->target.sym);
            if (it == inlineCandidates.end()) continue;
            Func *g = it->second;
            if (g == f) continue;
            bool allScalar = true;
            for (auto &a : ci->args)
                if (a.byMem || a.size != 4) allScalar = false;
            if (!allScalar) continue;
            int size = funcSize(g);
            int limit = g->obj->alwaysInline ? 100000 : g->obj->isInline ? (opt.optSize ? 24 : 60) : (opt.optSize ? 10 : 20);
            if (opt.optLevel < 2 && !g->obj->alwaysInline) limit = g->obj->isInline ? 12 : 0;
            if (size > limit || (size > budget && !g->obj->alwaysInline)) continue;
            budget -= size;

            // ---- perform the inlining
            Ins callCopy = call;
            std::vector<int> vmap(g->vregTy.size());
            for (size_t v = 0; v < g->vregTy.size(); v++) {
                vmap[v] = f->newVreg(g->vregTy[v], g->vregPtr[v]);
                f->vregFixed[vmap[v]] = g->vregFixed[v];
            }
            std::vector<int> smap(g->slots.size());
            for (size_t s = 0; s < g->slots.size(); s++) {
                smap[s] = f->newSlot(g->slots[s].size, g->slots[s].align);
            }
            std::map<int, Block *> bmap;
            for (Block *gb : g->blocks) bmap[gb->id] = f->newBlock();
            Block *after = f->newBlock();
            after->ins.assign(b->ins.begin() + k + 1, b->ins.end());
            b->ins.resize(k);
            // store the arguments into the callee's (renamed) parameter slots
            for (size_t s = 0; s < g->slots.size(); s++) {
                int ao = g->slots[s].argOffset;
                if (ao < 0) continue;
                size_t idx = (size_t)ao / 4;
                if (idx >= callCopy.call->args.size()) continue;
                int sz = g->slots[s].size;
                if (sz != 1 && sz != 2 && sz != 4) continue;
                Opd v = callCopy.call->args[idx].val;
                IRT t = (IRT)sz;
                if (t != I32 && v.isReg()) {
                    Ins tr;
                    tr.op = IR_TRUNC;
                    tr.ty = t;
                    tr.srcTy = I32;
                    tr.dst = f->newVreg(t);
                    tr.a = v;
                    b->ins.push_back(tr);
                    v = Opd::reg(tr.dst);
                } else if (t != I32 && v.isConst())
                    v = Opd::imm(norm(v.c, t));
                Ins st;
                st.op = IR_STORE;
                st.ty = t;
                st.a = Opd::slot(smap[s]);
                st.b = v;
                b->ins.push_back(st);
            }
            {
                Ins j;
                j.op = IR_JMP;
                j.t1 = bmap[g->blocks[0]->id]->id;
                b->ins.push_back(j);
            }
            int retTmp = -1;
            if (callCopy.dst >= 0) retTmp = f->newVreg(I32);
            auto mapOpd = [&](Opd &o) {
                if (o.kind == Opd::VREG) o.v = vmap[o.v];
                else if (o.kind == Opd::SLOT) o.v = smap[o.v];
            };
            std::vector<Block *> newBlocks;
            for (Block *gb : g->blocks) {
                Block *nb = bmap[gb->id];
                for (Ins i : gb->ins) {
                    if (i.call) i.call = new CallInfo(*i.call);
                    if (i.op == IR_ASM) {
                        for (auto &o : i.asmOps) mapOpd(o.val);
                    } else {
                        mapOpd(i.a);
                        mapOpd(i.b);
                        if (i.call) {
                            mapOpd(i.call->target);
                            for (auto &a : i.call->args) mapOpd(a.val);
                            mapOpd(i.call->retSlot);
                            mapOpd(i.call->sret);
                        }
                    }
                    if (i.dst >= 0) i.dst = vmap[i.dst];
                    if (i.t1 >= 0 && i.op != IR_RET) i.t1 = bmap.count(i.t1) ? bmap[i.t1]->id : i.t1;
                    if (i.t2 >= 0) i.t2 = bmap.count(i.t2) ? bmap[i.t2]->id : i.t2;
                    for (auto &c : i.cases) c.second = bmap[c.second]->id;
                    if (i.op == IR_RET) {
                        if (retTmp >= 0 && !i.a.isNone()) {
                            Ins m;
                            m.op = IR_MOV;
                            m.ty = I32;
                            m.dst = retTmp;
                            m.a = i.a;
                            nb->ins.push_back(m);
                        }
                        Ins j;
                        j.op = IR_JMP;
                        j.t1 = after->id;
                        nb->ins.push_back(j);
                        continue;
                    }
                    nb->ins.push_back(i);
                }
                newBlocks.push_back(nb);
            }
            if (callCopy.dst >= 0) {
                Ins m;
                IRT dt = f->vregTy[callCopy.dst];
                if (dt == I32) {
                    m.op = IR_MOV;
                    m.ty = I32;
                } else {
                    m.op = IR_TRUNC;
                    m.ty = dt;
                    m.srcTy = I32;
                }
                m.dst = callCopy.dst;
                m.a = Opd::reg(retTmp);
                after->ins.insert(after->ins.begin(), m);
            }
            f->blocks.insert(f->blocks.begin() + bi + 1, newBlocks.begin(), newBlocks.end());
            f->blocks.insert(f->blocks.begin() + bi + 1 + newBlocks.size(), after);
            if (g->hasCalls) f->hasCalls = true;
            changed = true;
            break; // continue with the next block (the inlined body is scanned too)
        }
    }
    return changed;
}

// ------------------------------------------------------------ driver
void optimize(Func *f) {
    computeCFG(f);
    if (opt.optLevel == 0) {
        cleanupCFG(f);
        return;
    }
    if (inlineCalls(f)) computeCFG(f);
    cleanupCFG(f);
    promoteSlots(f);
    for (int iter = 0; iter < 6; iter++) {
        bool ch = false;
        ch |= propagate(f);
        ch |= foldExtensions(f);
        ch |= retargetDefs(f);
        ch |= sinkIncrements(f);
        ch |= addrFold(f);
        ch |= localCSE(f);
        ch |= dce(f);
        ch |= cleanupCFG(f);
        if (iter == 1) {
            ch |= expandMul(f);
            ch |= expandDiv(f);
        }
        if (!ch && iter >= 2) break;
    }
    dce(f);
    cleanupCFG(f);
    // remember small functions for inlining into later ones
    if (f->obj && canBeInlined(f)) {
        int size = funcSize(f);
        if (size <= 80 || f->obj->alwaysInline) inlineCandidates[f->name] = f;
    }
}

// ------------------------------------------------------------ debugging
static std::string opdStr(const Opd &o) {
    switch (o.kind) {
    case Opd::NONE: return "-";
    case Opd::VREG: return format("v%d", o.v);
    case Opd::CONST: return format("#%lld", (long long)o.c);
    case Opd::SYM: return format("&%s%+lld", o.sym.c_str(), (long long)o.c);
    case Opd::SLOT: return format("&slot%d%+lld", o.v, (long long)o.c);
    }
    return "?";
}

std::string dumpIR(Func *f) {
    static const char *names[] = {"nop", "mov", "add", "sub", "mul", "divs", "divu", "mods", "modu", "and", "or", "xor",
                                  "shl", "shrs", "shru", "neg", "not", "sext", "zext", "trunc", "load", "store", "setcc",
                                  "br", "jmp", "switch", "call", "ret", "copyblk", "zeroblk", "asm", "alloca", "getreg",
                                  "setreg", "unreach"};
    static const char *ccs[] = {"eq", "ne", "lt", "le", "gt", "ge", "ult", "ule", "ugt", "uge"};
    std::string s = format("function %s\n", f->name.c_str());
    for (Block *b : f->blocks) {
        s += format("B%d:  (depth %d)\n", b->id, b->loopDepth);
        for (Ins &i : b->ins) {
            s += format("    %-7s i%d ", names[i.op], (int)i.ty * 8);
            if (i.dst >= 0) s += format("v%d = ", i.dst);
            s += opdStr(i.a) + ", " + opdStr(i.b);
            if (i.op == IR_LOAD || i.op == IR_STORE || i.op == IR_COPYBLK || i.op == IR_ZEROBLK) s += format(" off=%lld", (long long)i.off);
            if (i.op == IR_BR || i.op == IR_SETCC) s += format(" %s", ccs[i.cc]);
            if (i.op == IR_BR) s += format(" ? B%d : B%d", i.t1, i.t2);
            if (i.op == IR_JMP) s += format(" B%d", i.t1);
            if (i.call) {
                s += " " + opdStr(i.call->target) + "(";
                for (auto &a : i.call->args) s += opdStr(a.val) + " ";
                s += ")";
            }
            s += "\n";
        }
    }
    return s;
}

} // namespace p68
