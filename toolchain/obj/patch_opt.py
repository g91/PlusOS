import io
p = 'src/cc/opt.cpp'
s = open(p, encoding='utf-8').read()

def rep(a, b, cnt=1):
    global s
    assert a in s, a[:70]
    s = s.replace(a, b, cnt)

newpasses = r'''
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
'''

rep("// ------------------------------------------------------------ inlining\n",
    newpasses + "\n// ------------------------------------------------------------ inlining\n")
rep("""        ch |= propagate(f);
        ch |= addrFold(f);""",
    """        ch |= propagate(f);
        ch |= foldExtensions(f);
        ch |= retargetDefs(f);
        ch |= sinkIncrements(f);
        ch |= addrFold(f);""")

open(p, 'w', encoding='utf-8', newline='\n').write(s)
print("patched")
