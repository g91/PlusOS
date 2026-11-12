// expr.cpp - expression and statement parsing with type checking and constant folding.
#include "parse_int.h"

namespace p68 {

static std::vector<Node *> *curSwitchCases = nullptr;
static Node *curSwitch = nullptr;
static int curBrk = -1, curCont = -1;

// ------------------------------------------------------------ node constructors
Node *newNum(int64_t v, Token *t, Type *ty) {
    Node *n = new Node(ND_NUM, t);
    n->ty = ty ? ty : tyInt;
    n->ival = truncVal(v, n->ty);
    return n;
}
Node *newFNum(double v, Token *t, Type *ty) {
    Node *n = new Node(ND_FNUM, t);
    n->ty = ty;
    n->fval = ty->kind == TY_FLOAT ? (double)(float)v : v;
    return n;
}
Node *newVarNode(Obj *v, Token *t) {
    Node *n = new Node(ND_VAR, t);
    n->var = v;
    n->ty = v->ty;
    v->referenced = true;
    return n;
}

static void markAddrTaken(Node *n) {
    for (; n; n = n->lhs) {
        if (n->kind == ND_VAR) { n->var->addrTaken = true; return; }
        if (n->kind != ND_MEMBER) return;
    }
}

Node *newAddr(Node *n, Token *t) {
    if (n->kind == ND_MEMBER && n->member->isBitfield) errorAt(t, "cannot take the address of a bit-field");
    if (n->kind == ND_DEREF && n->ty->kind != TY_FUNC && n->ty->kind != TY_ARRAY) {
        // &*p == p
        Node *p = n->lhs;
        if (sameType(p->ty->base, n->ty)) return p;
    }
    markAddrTaken(n);
    Node *a = new Node(ND_ADDR, t);
    a->lhs = n;
    a->ty = pointerTo(n->ty);
    return a;
}

Node *rval(Node *n) {
    if (n->ty->kind == TY_ARRAY) {
        markAddrTaken(n);
        Node *a = new Node(ND_ADDR, n->tok);
        a->lhs = n;
        a->ty = pointerTo(n->ty->base);
        return a;
    }
    if (n->ty->kind == TY_FUNC) {
        Node *a = new Node(ND_ADDR, n->tok);
        a->lhs = n;
        a->ty = pointerTo(n->ty);
        return a;
    }
    return n;
}

Node *newDeref(Node *n, Token *t) {
    n = rval(n);
    if (n->ty->kind != TY_PTR) errorAt(t, "invalid pointer dereference (type %s)", typeName(n->ty).c_str());
    if (n->kind == ND_ADDR && sameType(n->ty->base, n->lhs->ty)) return n->lhs; // *&x == x
    Node *d = new Node(ND_DEREF, t);
    d->lhs = n;
    d->ty = n->ty->base;
    return d;
}

Node *newMember(Node *n, Member *m, Token *t) {
    Node *r = new Node(ND_MEMBER, t);
    r->lhs = n;
    r->member = m;
    r->ty = m->ty;
    if (n->ty->isConst || n->ty->isVolatile) {
        if ((n->ty->isConst && !r->ty->isConst) || (n->ty->isVolatile && !r->ty->isVolatile)) {
            r->ty = copyType(m->ty);
            r->ty->isConst = r->ty->isConst || n->ty->isConst;
            r->ty->isVolatile = r->ty->isVolatile || n->ty->isVolatile;
        }
    }
    return r;
}

Node *newCast(Node *n, Type *ty) {
    n = rval(n);
    if (ty->kind == TY_VOID) {
        Node *c = new Node(ND_CAST, n->tok);
        c->lhs = n;
        c->ty = ty;
        return c;
    }
    if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION)) {
        if (!sameType(ty, n->ty)) errorAt(n->tok, "cannot convert %s to %s", typeName(n->ty).c_str(), typeName(ty).c_str());
        return n;
    }
    if (n->ty->kind == TY_STRUCT || n->ty->kind == TY_UNION || n->ty->kind == TY_VOID)
        errorAt(n->tok, "cannot convert %s to %s", typeName(n->ty).c_str(), typeName(ty).c_str());
    if (n->kind == ND_NUM) {
        if (isFlonum(ty)) return newFNum(n->ty->isUnsigned ? (double)(uint64_t)n->ival : (double)n->ival, n->tok, ty);
        return newNum(n->ival, n->tok, ty);
    }
    if (n->kind == ND_FNUM) {
        if (isFlonum(ty)) return newFNum(n->fval, n->tok, ty);
        if (ty->kind != TY_PTR) {
            double d = n->fval;
            int64_t v = ty->isUnsigned && d >= 9223372036854775808.0 ? (int64_t)(uint64_t)d : (int64_t)d;
            if (ty->kind == TY_BOOL) v = d != 0;
            return newNum(v, n->tok, ty);
        }
    }
    if (isFlonum(ty) && n->ty->kind == TY_PTR) errorAt(n->tok, "cannot convert a pointer to a floating type");
    if (ty->kind == TY_PTR && isFlonum(n->ty)) errorAt(n->tok, "cannot convert a floating value to a pointer");
    Node *c = new Node(ND_CAST, n->tok);
    c->lhs = n;
    c->ty = ty;
    return c;
}

static Type *promoted(Type *t) {
    if (isInteger(t) && tsize(t) < 4) return tyInt;
    if (t->kind == TY_ENUM) return tyInt;
    return unqual(t);
}

static Type *commonType(Type *a, Type *b) {
    if (a->kind == TY_PTR) return a;
    if (b->kind == TY_PTR) return b;
    if (a->kind == TY_FUNC) return pointerTo(a);
    if (b->kind == TY_FUNC) return pointerTo(b);
    if (a->kind == TY_DOUBLE || b->kind == TY_DOUBLE) return tyDouble;
    if (a->kind == TY_FLOAT || b->kind == TY_FLOAT) return tyFloat;
    a = promoted(a);
    b = promoted(b);
    if (tsize(a) != tsize(b)) return tsize(a) > tsize(b) ? a : b;
    bool u = a->isUnsigned || b->isUnsigned;
    if (tsize(a) == 8) return u ? tyULLong : tyLLong;
    bool isLong = a->kind == TY_LONG || b->kind == TY_LONG;
    if (isLong) return u ? tyULong : tyLong;
    return u ? tyUInt : tyInt;
}

static bool isNullConst(Node *n) { return n->kind == ND_NUM && n->ival == 0 && isInteger(n->ty); }

static Node *foldInt(NodeKind k, Type *ty, Node *l, Node *r, Token *t) {
    int64_t a = l->ival, b = r->ival, v = 0;
    bool u = ty->isUnsigned, wide = tsize(ty) == 8;
    uint64_t ua = wide ? (uint64_t)a : (uint32_t)a, ub = wide ? (uint64_t)b : (uint32_t)b;
    switch (k) {
    case ND_ADD: v = a + b; break;
    case ND_SUB: v = a - b; break;
    case ND_MUL: v = (int64_t)((uint64_t)a * (uint64_t)b); break;
    case ND_DIV:
        if (b == 0) return nullptr;
        v = u ? (int64_t)(ua / ub) : (b == -1 ? -a : a / b);
        break;
    case ND_MOD:
        if (b == 0) return nullptr;
        v = u ? (int64_t)(ua % ub) : (b == -1 ? 0 : a % b);
        break;
    case ND_AND: v = a & b; break;
    case ND_OR: v = a | b; break;
    case ND_XOR: v = a ^ b; break;
    case ND_SHL: v = (int64_t)((uint64_t)a << (b & 63)); break;
    case ND_SHR: v = u ? (int64_t)(ua >> (b & 63)) : (a >> (b & 63)); break;
    default: return nullptr;
    }
    return newNum(v, t, ty);
}

Node *newBinary(NodeKind k, Node *l, Node *r, Token *t) {
    l = rval(l);
    r = rval(r);
    Node *n = new Node(k, t);
    switch (k) {
    case ND_MUL: case ND_DIV: case ND_MOD: case ND_AND: case ND_OR: case ND_XOR: case ND_ADD: case ND_SUB: {
        if (!isArith(l->ty) || !isArith(r->ty)) errorAt(t, "invalid operands to binary operator (%s and %s)", typeName(l->ty).c_str(), typeName(r->ty).c_str());
        Type *ty = commonType(l->ty, r->ty);
        if (isFlonum(ty) && (k == ND_MOD || k == ND_AND || k == ND_OR || k == ND_XOR)) errorAt(t, "invalid operands to binary operator");
        l = newCast(l, ty);
        r = newCast(r, ty);
        if (l->kind == ND_NUM && r->kind == ND_NUM) {
            Node *f = foldInt(k, ty, l, r, t);
            if (f) return f;
        }
        if (l->kind == ND_FNUM && r->kind == ND_FNUM) {
            double a = l->fval, b = r->fval;
            return newFNum(k == ND_ADD ? a + b : k == ND_SUB ? a - b : k == ND_MUL ? a * b : a / b, t, ty);
        }
        n->ty = ty;
        break;
    }
    case ND_SHL: case ND_SHR: {
        if (!isInteger(l->ty) || !isInteger(r->ty)) errorAt(t, "invalid operands to shift");
        Type *ty = promoted(l->ty);
        l = newCast(l, ty);
        r = newCast(r, tyInt);
        if (l->kind == ND_NUM && r->kind == ND_NUM) {
            Node *f = foldInt(k, ty, l, r, t);
            if (f) return f;
        }
        n->ty = ty;
        break;
    }
    case ND_EQ: case ND_NE: case ND_LT: case ND_LE: {
        if (!isScalar(l->ty) || !isScalar(r->ty)) errorAt(t, "invalid operands to comparison");
        Type *ty = commonType(l->ty, r->ty);
        if (ty->kind == TY_PTR) ty = tyULong;
        l = newCast(l, ty);
        r = newCast(r, ty);
        if (l->kind == ND_NUM && r->kind == ND_NUM) {
            int64_t a = l->ival, b = r->ival;
            uint64_t ua = (uint64_t)a, ub = (uint64_t)b;
            bool u = ty->isUnsigned;
            int v = k == ND_EQ ? a == b : k == ND_NE ? a != b : k == ND_LT ? (u ? ua < ub : a < b) : (u ? ua <= ub : a <= b);
            return newNum(v, t, tyInt);
        }
        if (l->kind == ND_FNUM && r->kind == ND_FNUM) {
            double a = l->fval, b = r->fval;
            return newNum(k == ND_EQ ? a == b : k == ND_NE ? a != b : k == ND_LT ? a < b : a <= b, t, tyInt);
        }
        n->ty = tyInt;
        break;
    }
    case ND_LOGAND: case ND_LOGOR: {
        if (!isScalar(l->ty) || !isScalar(r->ty)) errorAt(t, "invalid operands to logical operator");
        if (l->kind == ND_NUM) {
            bool lv = l->ival != 0;
            if (k == ND_LOGAND && !lv) return newNum(0, t, tyInt);
            if (k == ND_LOGOR && lv) return newNum(1, t, tyInt);
            if (r->kind == ND_NUM) return newNum(r->ival != 0, t, tyInt);
        }
        n->ty = tyInt;
        break;
    }
    case ND_COMMA: n->ty = r->ty; break;
    default: errorAt(t, "internal: bad binary operator");
    }
    n->lhs = l;
    n->rhs = r;
    return n;
}

static int ptrScale(Type *ptr) {
    Type *b = ptr->base;
    if (b->kind == TY_VOID || b->kind == TY_FUNC) return 1;
    if ((b->kind == TY_STRUCT || b->kind == TY_UNION) && !sdef(b)->complete) errorAt(tok, "arithmetic on a pointer to an incomplete type");
    int s = tsize(b);
    return s ? s : 1;
}

Node *newAddSub(NodeKind k, Node *l, Node *r, Token *t) {
    l = rval(l);
    r = rval(r);
    if (isArith(l->ty) && isArith(r->ty)) return newBinary(k, l, r, t);
    if (k == ND_ADD && l->ty->kind != TY_PTR && r->ty->kind == TY_PTR) std::swap(l, r);
    if (l->ty->kind == TY_PTR && isInteger(r->ty)) {
        int scale = ptrScale(l->ty);
        Node *idx = newCast(r, tyLong);
        if (scale != 1) idx = newBinary(ND_MUL, idx, newNum(scale, t, tyLong), t);
        if (idx->kind == ND_NUM && idx->ival == 0) return l;
        Node *n = new Node(k, t);
        n->lhs = l;
        n->rhs = idx;
        n->ty = l->ty;
        return n;
    }
    if (k == ND_SUB && l->ty->kind == TY_PTR && r->ty->kind == TY_PTR) {
        int scale = ptrScale(l->ty);
        Node *n = new Node(ND_SUB, t);
        n->lhs = newCast(l, tyLong);
        n->rhs = newCast(r, tyLong);
        n->ty = tyLong;
        if (scale == 1) return n;
        return newBinary(ND_DIV, n, newNum(scale, t, tyLong), t);
    }
    errorAt(t, "invalid operands to %s (%s and %s)", k == ND_ADD ? "+" : "-", typeName(l->ty).c_str(), typeName(r->ty).c_str());
}

static bool isLvalue(Node *n) {
    return n->kind == ND_VAR || n->kind == ND_DEREF || n->kind == ND_MEMBER;
}

Node *newAssign(Node *lhs, Node *rhs, Token *t) {
    if (!isLvalue(lhs)) errorAt(t, "expression is not assignable");
    if (lhs->ty->kind == TY_ARRAY) errorAt(t, "array is not assignable");
    Node *n = new Node(ND_ASSIGN, t);
    n->lhs = lhs;
    if (lhs->ty->kind == TY_STRUCT || lhs->ty->kind == TY_UNION) {
        if (!sameType(lhs->ty, rhs->ty)) errorAt(t, "incompatible types in assignment (%s = %s)", typeName(lhs->ty).c_str(), typeName(rhs->ty).c_str());
        n->rhs = rhs;
    } else
        n->rhs = newCast(rhs, unqual(lhs->ty));
    n->ty = unqual(lhs->ty);
    return n;
}

static Node *newAssignOp(NodeKind op, Node *lhs, Node *rhs, Token *t) {
    if (!isLvalue(lhs)) errorAt(t, "expression is not assignable");
    rhs = rval(rhs);
    Node *n = new Node(ND_ASSIGNOP, t);
    n->op = op;
    n->lhs = lhs;
    n->ty = unqual(lhs->ty);
    if (lhs->ty->kind == TY_PTR) {
        if ((op != ND_ADD && op != ND_SUB) || !isInteger(rhs->ty)) errorAt(t, "invalid compound assignment to a pointer");
        int scale = ptrScale(lhs->ty);
        Node *idx = newCast(rhs, tyLong);
        if (scale != 1) idx = newBinary(ND_MUL, idx, newNum(scale, t, tyLong), t);
        n->rhs = idx;
        n->opTy = tyULong;
        return n;
    }
    if (!isArith(lhs->ty) || !isArith(rhs->ty)) errorAt(t, "invalid operands to compound assignment");
    if (op == ND_SHL || op == ND_SHR) {
        n->opTy = promoted(lhs->ty);
        n->rhs = newCast(rhs, tyInt);
    } else {
        n->opTy = commonType(lhs->ty, rhs->ty);
        n->rhs = newCast(rhs, n->opTy);
    }
    return n;
}

// ------------------------------------------------------------ expressions
static Node *cast();
static Node *unary();
static Node *postfix();
static Node *primary();

int64_t constExpr() {
    Token *t = tok;
    Node *n = conditional();
    return evalConstExpr(n, t);
}

Node *expr() {
    Node *n = assign();
    while (equal(",")) {
        Token *t = tok;
        tok = tok->next;
        Node *r = assign();
        Node *c = new Node(ND_COMMA, t);
        c->lhs = n;
        c->rhs = rval(r);
        c->ty = c->rhs->ty;
        n = c;
    }
    return n;
}

Node *assign() {
    Node *n = conditional();
    Token *t = tok;
    if (tok->kind != Token::PUNCT) return n;
    const std::string &s = tok->text;
    if (s == "=") { tok = tok->next; return newAssign(n, assign(), t); }
    NodeKind op;
    if (s == "+=") op = ND_ADD;
    else if (s == "-=") op = ND_SUB;
    else if (s == "*=") op = ND_MUL;
    else if (s == "/=") op = ND_DIV;
    else if (s == "%=") op = ND_MOD;
    else if (s == "&=") op = ND_AND;
    else if (s == "|=") op = ND_OR;
    else if (s == "^=") op = ND_XOR;
    else if (s == "<<=") op = ND_SHL;
    else if (s == ">>=") op = ND_SHR;
    else return n;
    tok = tok->next;
    return newAssignOp(op, n, assign(), t);
}

static int binPrec(const Token *t, NodeKind *k, bool *swap) {
    *swap = false;
    if (t->kind != Token::PUNCT) return -1;
    const std::string &s = t->text;
    if (s == "*") { *k = ND_MUL; return 10; }
    if (s == "/") { *k = ND_DIV; return 10; }
    if (s == "%") { *k = ND_MOD; return 10; }
    if (s == "+") { *k = ND_ADD; return 9; }
    if (s == "-") { *k = ND_SUB; return 9; }
    if (s == "<<") { *k = ND_SHL; return 8; }
    if (s == ">>") { *k = ND_SHR; return 8; }
    if (s == "<") { *k = ND_LT; return 7; }
    if (s == "<=") { *k = ND_LE; return 7; }
    if (s == ">") { *k = ND_LT; *swap = true; return 7; }
    if (s == ">=") { *k = ND_LE; *swap = true; return 7; }
    if (s == "==") { *k = ND_EQ; return 6; }
    if (s == "!=") { *k = ND_NE; return 6; }
    if (s == "&") { *k = ND_AND; return 5; }
    if (s == "^") { *k = ND_XOR; return 4; }
    if (s == "|") { *k = ND_OR; return 3; }
    if (s == "&&") { *k = ND_LOGAND; return 2; }
    if (s == "||") { *k = ND_LOGOR; return 1; }
    return -1;
}

static Node *binary(int minPrec) {
    Node *l = cast();
    for (;;) {
        NodeKind k;
        bool swap;
        int prec = binPrec(tok, &k, &swap);
        if (prec < minPrec || prec < 0) return l;
        Token *t = tok;
        tok = tok->next;
        Node *r = binary(prec + 1);
        if (k == ND_ADD || k == ND_SUB) l = newAddSub(k, l, r, t);
        else if (swap) l = newBinary(k, r, l, t);
        else l = newBinary(k, l, r, t);
    }
}

Node *conditional() {
    Node *c = binary(1);
    if (!equal("?")) return c;
    Token *t = tok;
    tok = tok->next;
    c = rval(c);
    if (!isScalar(c->ty)) errorAt(t, "condition is not a scalar");
    Node *a;
    if (equal(":")) a = c; // GNU a ?: b (a evaluated twice; only safe without side effects)
    else a = rval(expr());
    expect(":");
    Node *b = rval(conditional());
    Type *ty;
    if (a->ty->kind == TY_VOID || b->ty->kind == TY_VOID) ty = tyVoid;
    else if (a->ty->kind == TY_STRUCT || a->ty->kind == TY_UNION) ty = a->ty;
    else if (a->ty->kind == TY_PTR && isNullConst(b)) ty = a->ty;
    else if (b->ty->kind == TY_PTR && isNullConst(a)) ty = b->ty;
    else if (a->ty->kind == TY_PTR && b->ty->kind == TY_PTR) ty = a->ty->base->kind == TY_VOID ? b->ty : a->ty;
    else ty = commonType(a->ty, b->ty);
    if (ty->kind != TY_VOID) {
        a = newCast(a, ty);
        b = newCast(b, ty);
    }
    if (c->kind == ND_NUM) return c->ival ? a : b;
    Node *n = new Node(ND_COND, t);
    n->cond = c;
    n->then = a;
    n->els = b;
    n->ty = ty;
    return n;
}

static Node *compoundLiteral(Type *ty, Token *t) {
    if (!curFn) errorAt(t, "compound literals at file scope are not supported");
    Obj *var = newLocal("", ty, t);
    Node *init = localInitializer(var, t);
    Node *v = newVarNode(var, t);
    if (!init) return v;
    Node *c = new Node(ND_COMMA, t);
    c->lhs = init;
    c->rhs = v;
    c->ty = v->ty;
    return c;
}

static Node *cast() {
    if (equal("(") && isTypeName(tok->next)) {
        Token *t = tok;
        tok = tok->next;
        Type *ty = typeName();
        expect(")");
        if (equal("{")) { // compound literal
            Node *n = compoundLiteral(ty, t);
            // allow postfix operators on it
            Node *save = n;
            (void)save;
            while (equal("[") || equal(".") || equal("->")) {
                if (consume("[")) {
                    Node *idx = expr();
                    expect("]");
                    n = newDeref(newAddSub(ND_ADD, n, idx, t), t);
                } else
                    break;
            }
            return n;
        }
        Node *e = cast();
        return newCast(e, ty);
    }
    return unary();
}

static Node *logicalNot(Node *e, Token *t) {
    e = rval(e);
    if (!isScalar(e->ty)) errorAt(t, "invalid operand to '!'");
    if (e->kind == ND_NUM) return newNum(!e->ival, t, tyInt);
    if (e->kind == ND_FNUM) return newNum(e->fval == 0, t, tyInt);
    Node *n = new Node(ND_NOT, t);
    n->lhs = e;
    n->ty = tyInt;
    return n;
}

static Node *unary() {
    Token *t = tok;
    if (consume("+")) {
        Node *e = rval(cast());
        if (!isArith(e->ty)) errorAt(t, "invalid operand to unary '+'");
        return newCast(e, promoted(e->ty));
    }
    if (consume("-")) {
        Node *e = rval(cast());
        if (!isArith(e->ty)) errorAt(t, "invalid operand to unary '-'");
        Type *ty = promoted(e->ty);
        e = newCast(e, ty);
        if (e->kind == ND_NUM) return newNum(-e->ival, t, ty);
        if (e->kind == ND_FNUM) return newFNum(-e->fval, t, ty);
        Node *n = new Node(ND_NEG, t);
        n->lhs = e;
        n->ty = ty;
        return n;
    }
    if (consume("!")) return logicalNot(cast(), t);
    if (consume("~")) {
        Node *e = rval(cast());
        if (!isInteger(e->ty)) errorAt(t, "invalid operand to '~'");
        Type *ty = promoted(e->ty);
        e = newCast(e, ty);
        if (e->kind == ND_NUM) return newNum(~e->ival, t, ty);
        Node *n = new Node(ND_BITNOT, t);
        n->lhs = e;
        n->ty = ty;
        return n;
    }
    if (consume("&")) {
        Node *e = cast();
        if (e->kind == ND_COMMA && isLvalue(e->rhs)) { // &(compound literal)
            Node *c = new Node(ND_COMMA, t);
            c->lhs = e->lhs;
            c->rhs = newAddr(e->rhs, t);
            c->ty = c->rhs->ty;
            return c;
        }
        if (e->kind == ND_ADDR && e->lhs->ty->kind == TY_FUNC) return e; // &func already decayed
        if (!isLvalue(e)) errorAt(t, "cannot take the address of an rvalue");
        return newAddr(e, t);
    }
    if (consume("*")) {
        Node *e = rval(cast());
        if (e->ty->kind == TY_PTR && e->ty->base->kind == TY_FUNC) {
            Node *d = new Node(ND_DEREF, t);
            d->lhs = e;
            d->ty = e->ty->base;
            return d;
        }
        return newDeref(e, t);
    }
    if (consume("++")) return newAssignOp(ND_ADD, unary(), newNum(1, t), t);
    if (consume("--")) return newAssignOp(ND_SUB, unary(), newNum(1, t), t);
    if (equal("sizeof")) {
        tok = tok->next;
        Type *ty;
        if (equal("(") && isTypeName(tok->next)) {
            tok = tok->next;
            ty = typeName();
            expect(")");
        } else
            ty = unary()->ty;
        if (ty->kind == TY_ARRAY && ty->arrayLen < 0) errorAt(t, "sizeof applied to an incomplete array");
        if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !sdef(ty)->complete) errorAt(t, "sizeof applied to an incomplete type");
        return newNum(tsize(ty), t, tyULong);
    }
    if (equal("_Alignof")) {
        tok = tok->next;
        Type *ty;
        if (equal("(") && isTypeName(tok->next)) {
            tok = tok->next;
            ty = typeName();
            expect(")");
        } else
            ty = unary()->ty;
        return newNum(talign(ty), t, tyULong);
    }
    return postfix();
}

static Node *structRef(Node *n, Token *nameTok) {
    Type *ty = n->ty;
    if (ty->kind != TY_STRUCT && ty->kind != TY_UNION) errorAt(nameTok, "member reference base is not a struct or union");
    Type *d = sdef(ty);
    if (!d->complete) errorAt(nameTok, "member access into incomplete type %s", typeName(ty).c_str());
    for (auto &m : d->members)
        if (!m.name.empty() && m.name == nameTok->text) return newMember(n, &m, nameTok);
    for (auto &m : d->members) {
        if (m.name.empty() && (m.ty->kind == TY_STRUCT || m.ty->kind == TY_UNION)) {
            // search inside the anonymous member
            std::function<bool(Type *)> has = [&](Type *t) -> bool {
                for (auto &x : sdef(t)->members) {
                    if (x.name == nameTok->text) return true;
                    if (x.name.empty() && (x.ty->kind == TY_STRUCT || x.ty->kind == TY_UNION) && has(x.ty)) return true;
                }
                return false;
            };
            if (has(m.ty)) return structRef(newMember(n, &m, nameTok), nameTok);
        }
    }
    errorAt(nameTok, "no member named '%s' in %s", nameTok->text.c_str(), typeName(ty).c_str());
}

static Node *postInc(Node *n, int delta, Token *t) {
    if (!isLvalue(n)) errorAt(t, "expression is not assignable");
    Node *p = new Node(ND_POSTINC, t);
    p->lhs = n;
    p->ty = unqual(n->ty);
    if (n->ty->kind == TY_PTR) p->ival = delta * ptrScale(n->ty);
    else if (isArith(n->ty)) p->ival = delta;
    else errorAt(t, "invalid operand to increment");
    return p;
}

static Node *funcall(Node *fn, Token *t) {
    // tok is after '('
    Node *callee = rval(fn);
    Type *fty = nullptr;
    if (callee->ty->kind == TY_PTR && callee->ty->base->kind == TY_FUNC) fty = callee->ty->base;
    else errorAt(t, "called object is not a function");
    Node *n = new Node(ND_CALL, t);
    n->funcTy = fty;
    n->lhs = callee;
    if (callee->kind == ND_ADDR && callee->lhs->kind == ND_VAR && callee->lhs->var->isFunction) {
        n->callee = callee->lhs->var;
        if (!n->callee->unavailable.empty())
            errorAt(t, "'%s' is unavailable: %s", n->callee->name.c_str(), n->callee->unavailable.c_str());
    }
    size_t i = 0;
    while (!consume(")")) {
        if (i > 0) expect(",");
        Node *a = rval(assign());
        if (i < fty->params.size()) {
            Type *pt = fty->params[i];
            if (pt->kind == TY_STRUCT || pt->kind == TY_UNION) {
                if (!sameType(pt, a->ty)) errorAt(a->tok, "incompatible type for argument %d", (int)i + 1);
            } else
                a = newCast(a, unqual(pt));
        } else {
            if (!fty->variadic && !fty->oldStyle) errorAt(a->tok, "too many arguments in call");
            if (a->ty->kind == TY_FLOAT) a = newCast(a, tyDouble);
            else if (isInteger(a->ty) && tsize(a->ty) < 4) a = newCast(a, tyInt);
        }
        n->body.push_back(a);
        i++;
    }
    if (i < fty->params.size()) errorAt(t, "too few arguments in call");
    n->ty = fty->base;
    if ((n->ty->kind == TY_STRUCT || n->ty->kind == TY_UNION) && !sdef(n->ty)->complete) errorAt(t, "call returns an incomplete type");
    return n;
}

static Node *postfix() {
    Node *n = primary();
    for (;;) {
        Token *t = tok;
        if (consume("(")) { n = funcall(n, t); continue; }
        if (consume("[")) {
            Node *idx = expr();
            expect("]");
            n = newDeref(newAddSub(ND_ADD, n, idx, t), t);
            continue;
        }
        if (consume(".")) {
            n = structRef(n, tok);
            tok = tok->next;
            continue;
        }
        if (consume("->")) {
            n = structRef(newDeref(n, t), tok);
            tok = tok->next;
            continue;
        }
        if (consume("++")) { n = postInc(n, 1, t); continue; }
        if (consume("--")) { n = postInc(n, -1, t); continue; }
        return n;
    }
}

static Node *builtinCall(const std::string &name, Token *t) {
    // tok is after '('
    if (name == "__builtin_va_start") {
        Node *ap = assign();
        if (consume(",")) assign();
        expect(")");
        Node *n = new Node(ND_VA_START, t);
        n->lhs = ap;
        n->ty = tyVoid;
        if (curFn) curFn->usesVarargs = true;
        return n;
    }
    if (name == "__builtin_va_arg") {
        Node *ap = assign();
        expect(",");
        Type *ty = typeName();
        expect(")");
        int sz = tsize(ty);
        int rsz = (sz + 3) & ~3;
        int adj = sz < 4 ? 4 - sz : 0;
        // (ap = ap + rsz, *(T *)(ap - rsz + adj))
        Node *inc = newAssign(ap, newAddSub(ND_ADD, ap, newNum(rsz, t), t), t);
        Node *addr = newAddSub(ND_SUB, ap, newNum(rsz - adj, t), t);
        Node *val = newDeref(newCast(addr, pointerTo(ty)), t);
        Node *c = new Node(ND_COMMA, t);
        c->lhs = inc;
        c->rhs = val;
        c->ty = val->ty;
        return c;
    }
    if (name == "__builtin_alloca") {
        Node *sz = newCast(assign(), tyULong);
        expect(")");
        Node *n = new Node(ND_ALLOCA, t);
        n->lhs = sz;
        n->ty = pointerTo(tyVoid);
        if (curFn) curFn->usesAlloca = 1;
        return n;
    }
    if (name == "__builtin_unreachable" || name == "__builtin_trap") {
        expect(")");
        Node *n = new Node(ND_UNREACHABLE, t);
        n->ty = tyVoid;
        return n;
    }
    return nullptr;
}

static Node *primary() {
    Token *t = tok;
    // C++ scope resolution: Namespace::name / EnumType::enumerator / ::name. Namespaces and
    // scoped enums are treated as transparent here (their members live in the enclosing scope),
    // so a qualifier that names a known scope is simply skipped.
    if (opt.cxx) {
        while (equal("::")) tok = tok->next;        // leading ::name (global scope)
        while (tok->kind == Token::IDENT && tok->next->is("::")) tok = tok->next->next;
    }
    if (equal("(") && tok->next->is("{")) { // GNU statement expression
        tok = tok->next;
        Node *n = new Node(ND_STMT_EXPR, t);
        Node *blk = compoundStmt();
        n->body = blk->body;
        expect(")");
        n->ty = tyVoid;
        if (!n->body.empty() && n->body.back()->kind == ND_EXPR_STMT) {
            n->body.back()->lhs = rval(n->body.back()->lhs);
            n->ty = n->body.back()->lhs->ty;
        }
        return n;
    }
    if (consume("(")) {
        Node *n = expr();
        expect(")");
        return n;
    }
    if (tok->kind == Token::NUM) {
        tok = tok->next;
        if (isFlonum(t->ty)) return newFNum(t->fval, t, t->ty);
        Node *n = new Node(ND_NUM, t);
        n->ty = t->ty;
        n->ival = t->ival;
        return n;
    }
    if (tok->kind == Token::STR) {
        std::string s;
        while (tok->kind == Token::STR) { s += tok->sval; tok = tok->next; }
        return newVarNode(newStringLiteral(s, t), t);
    }
    if (equal("_Generic")) errorAt(t, "_Generic is not supported");
    if (tok->kind == Token::IDENT) {
        if (tok->next->is("(")) {
            const std::string &name = tok->text;
            if (name.compare(0, 10, "__builtin_") == 0) {
                Token *save = tok;
                tok = tok->next->next;
                Node *b = builtinCall(name, t);
                if (b) return b;
                tok = save;
                // __builtin_memcpy etc: call the library function of the same name
                std::string plain = name.substr(10);
                VarScope *vs = findVar(plain);
                if (!vs || !vs->var) {
                    Type *fty = funcType(tyInt);
                    fty->oldStyle = true;
                    Obj *fn = newGlobal(plain, fty, t);
                    fn->isFunction = true;
                    vs = pushVar(plain);
                    vs->var = fn;
                }
                tok = tok->next;
                return newVarNode(vs->var, t);
            }
        }
        VarScope *vs = findVar(tok->text);
        if (!vs || (!vs->var && !vs->isEnum)) {
            if (tok->next->is("(")) {
                warnAt(tok, "implicit declaration of function '%s'", tok->text.c_str());
                Type *fty = funcType(tyInt);
                fty->oldStyle = true;
                Obj *fn = newGlobal(tok->text, fty, tok);
                fn->isFunction = true;
                // declare at file scope: findVar walks outward, so push into the outermost scope
                VarScope *nv = pushVar(tok->text);
                nv->var = fn;
                tok = tok->next;
                return newVarNode(fn, t);
            }
            if (opt.cxx && (tok->text == "true" || tok->text == "false")) {
                tok = tok->next;
                return newNum(t->text == "true", t, tyBool);
            }
            if (opt.cxx && tok->text == "nullptr") {
                tok = tok->next;
                return newNum(0, t, tyLong);
            }
            errorAt(tok, "use of undeclared identifier '%s'", tok->text.c_str());
        }
        tok = tok->next;
        if (vs->isEnum) return newNum(vs->enumVal, t, vs->enumTy);
        return newVarNode(vs->var, t);
    }
    errorAt(tok, "expected an expression");
}

// ------------------------------------------------------------ statements
static Node *condExpr() {
    Node *c = rval(expr());
    if (!isScalar(c->ty)) errorAt(c->tok, "condition is not a scalar");
    return c;
}

static Node *asmStmt() {
    Token *t = tok;
    tok = tok->next;
    while (equal("volatile") || equal("inline") || equal("goto") || equal("__volatile__")) tok = tok->next;
    expect("(");
    Node *n = new Node(ND_ASM, t);
    if (tok->kind != Token::STR) errorAt(tok, "expected assembly template string");
    while (tok->kind == Token::STR) { n->label += tok->sval; tok = tok->next; }
    for (int section = 0; section < 3 && consume(":"); section++) {
        while (equal(":") == false && !equal(")")) {
            if (section == 2) {
                if (tok->kind != Token::STR) errorAt(tok, "expected clobber string");
                n->asmClobbers.push_back(tok->sval);
                tok = tok->next;
            } else {
                if (consume("[")) { tok = tok->next; expect("]"); }
                if (tok->kind != Token::STR) errorAt(tok, "expected constraint string");
                std::string c = tok->sval;
                tok = tok->next;
                expect("(");
                Node *e = expr();
                expect(")");
                if (section == 0) {
                    if (!isLvalue(e)) errorAt(e->tok, "asm output is not an lvalue");
                    n->asmNumOut++;
                } else if (c.find('m') == std::string::npos || !isLvalue(e))
                    e = rval(e);
                n->asmConstraints.push_back(c);
                n->body.push_back(e);
            }
            if (!consume(",")) break;
        }
    }
    expect(")");
    expect(";");
    return n;
}

Node *stmt() {
    Token *t = tok;
    if (consume(";")) return new Node(ND_BLOCK, t);
    if (equal("{")) return compoundStmt();
    if (consume("return")) {
        Node *n = new Node(ND_RETURN, t);
        if (consume(";")) return n;
        Node *e = rval(expr());
        expect(";");
        Type *rt = curFn->ty->base;
        if (rt->kind == TY_VOID) {
            n->lhs = e; // evaluated for side effects
            n->ival = 1;
        } else if (rt->kind == TY_STRUCT || rt->kind == TY_UNION) {
            if (!sameType(rt, e->ty)) errorAt(t, "incompatible return type");
            n->lhs = e;
        } else
            n->lhs = newCast(e, unqual(rt));
        return n;
    }
    if (consume("if")) {
        Node *n = new Node(ND_IF, t);
        expect("(");
        n->cond = condExpr();
        expect(")");
        n->then = stmt();
        if (consume("else")) n->els = stmt();
        return n;
    }
    if (consume("switch")) {
        Node *n = new Node(ND_SWITCH, t);
        expect("(");
        Node *c = rval(expr());
        if (!isInteger(c->ty)) errorAt(t, "switch quantity is not an integer");
        n->cond = newCast(c, tsize(c->ty) == 8 ? c->ty : promoted(c->ty));
        if (tsize(n->cond->ty) == 8) n->cond = newCast(n->cond, tyLong);
        expect(")");
        Node *saveSw = curSwitch;
        int saveBrk = curBrk;
        curSwitch = n;
        curBrk = n->brkId = newLabelId();
        n->then = stmt();
        curSwitch = saveSw;
        curBrk = saveBrk;
        return n;
    }
    if (consume("case")) {
        if (!curSwitch) errorAt(t, "stray case");
        Node *n = new Node(ND_CASE, t);
        n->ival = truncVal(constExpr(), curSwitch->cond->ty);
        n->ival2 = n->ival;
        if (consume("...")) n->ival2 = truncVal(constExpr(), curSwitch->cond->ty);
        expect(":");
        n->labelId = newLabelId();
        curSwitch->cases.push_back(n);
        if (equal("}")) n->lhs = new Node(ND_BLOCK, t);
        else n->lhs = stmt();
        return n;
    }
    if (consume("default")) {
        if (!curSwitch) errorAt(t, "stray default");
        expect(":");
        Node *n = new Node(ND_CASE, t);
        n->isDefault = true;
        n->labelId = newLabelId();
        curSwitch->defaultCase = n;
        if (equal("}")) n->lhs = new Node(ND_BLOCK, t);
        else n->lhs = stmt();
        return n;
    }
    if (consume("for")) {
        Node *n = new Node(ND_FOR, t);
        expect("(");
        enterScope();
        int saveBrk = curBrk, saveCont = curCont;
        curBrk = n->brkId = newLabelId();
        curCont = n->contId = newLabelId();
        if (isTypeName(tok)) {
            Attrs a;
            Type *base = declspec(&a);
            n->init = declaration(base, &a);
        } else if (!consume(";")) {
            Node *es = new Node(ND_EXPR_STMT, tok);
            es->lhs = expr();
            n->init = es;
            expect(";");
        }
        if (!equal(";")) n->cond = condExpr();
        expect(";");
        if (!equal(")")) n->inc = expr();
        expect(")");
        n->then = stmt();
        leaveScope();
        curBrk = saveBrk;
        curCont = saveCont;
        return n;
    }
    if (consume("while")) {
        Node *n = new Node(ND_FOR, t);
        expect("(");
        n->cond = condExpr();
        expect(")");
        int saveBrk = curBrk, saveCont = curCont;
        curBrk = n->brkId = newLabelId();
        curCont = n->contId = newLabelId();
        n->then = stmt();
        curBrk = saveBrk;
        curCont = saveCont;
        return n;
    }
    if (consume("do")) {
        Node *n = new Node(ND_DO, t);
        int saveBrk = curBrk, saveCont = curCont;
        curBrk = n->brkId = newLabelId();
        curCont = n->contId = newLabelId();
        n->then = stmt();
        curBrk = saveBrk;
        curCont = saveCont;
        if (!consume("while")) errorAt(tok, "expected 'while'");
        expect("(");
        n->cond = condExpr();
        expect(")");
        expect(";");
        return n;
    }
    if (consume("break")) {
        if (curBrk < 0) errorAt(t, "stray break");
        Node *n = new Node(ND_GOTO, t);
        n->labelId = curBrk;
        expect(";");
        return n;
    }
    if (consume("continue")) {
        if (curCont < 0) errorAt(t, "stray continue");
        Node *n = new Node(ND_GOTO, t);
        n->labelId = curCont;
        expect(";");
        return n;
    }
    if (consume("goto")) {
        Node *n = new Node(ND_GOTO, t);
        if (tok->kind != Token::IDENT) errorAt(tok, "expected label name");
        n->label = tok->text;
        tok = tok->next;
        expect(";");
        return n;
    }
    if ((equal("__asm__") || equal("asm")) ) return asmStmt();
    if (tok->kind == Token::IDENT && tok->next->is(":") && !tok->next->next->is(":")) {
        Node *n = new Node(ND_LABEL, t);
        n->label = tok->text;
        tok = tok->next->next;
        parseAttributes(nullptr);
        if (equal("}")) n->lhs = new Node(ND_BLOCK, t);
        else n->lhs = stmt();
        return n;
    }
    Node *n = new Node(ND_EXPR_STMT, t);
    n->lhs = expr();
    expect(";");
    return n;
}

Node *compoundStmt() {
    Node *blk = new Node(ND_BLOCK, tok);
    expect("{");
    enterScope();
    while (!consume("}")) {
        if (tok->kind == Token::END) errorAt(blk->tok, "unterminated block");
        if (equal("_Static_assert")) {
            Token *t = tok;
            tok = tok->next;
            expect("(");
            int64_t v = constExpr();
            if (consume(",")) { while (tok->kind == Token::STR) tok = tok->next; }
            expect(")");
            expect(";");
            if (!v) errorAt(t, "static assertion failed");
            continue;
        }
        if (isTypeName(tok) && !tok->next->is(":")) {
            Attrs a;
            Type *base = declspec(&a);
            if (consume(";")) continue;
            if (a.isTypedef) { parseTypedef(base, &a); continue; }
            Node *d = declaration(base, &a);
            for (Node *s : d->body) blk->body.push_back(s);
            continue;
        }
        blk->body.push_back(stmt());
    }
    leaveScope();
    return blk;
}

} // namespace p68
