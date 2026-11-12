// parse.cpp - types, scopes, declarations, initialisers and the translation-unit driver.
#include "parse_int.h"

namespace p68 {

// ------------------------------------------------------------ types
static Type *mkType(TypeKind k, int size, int align, bool uns = false) {
    Type *t = new Type;
    t->kind = k;
    t->size = size;
    t->align = align;
    t->isUnsigned = uns;
    return t;
}
Type *tyVoid = mkType(TY_VOID, 1, 1);
Type *tyBool = mkType(TY_BOOL, 1, 1, true);
Type *tyChar = mkType(TY_CHAR, 1, 1);
Type *tySChar = mkType(TY_CHAR, 1, 1);
Type *tyUChar = mkType(TY_CHAR, 1, 1, true);
Type *tyShort = mkType(TY_SHORT, 2, 2);
Type *tyUShort = mkType(TY_SHORT, 2, 2, true);
Type *tyInt = mkType(TY_INT, 4, 2);
Type *tyUInt = mkType(TY_INT, 4, 2, true);
Type *tyLong = mkType(TY_LONG, 4, 2);
Type *tyULong = mkType(TY_LONG, 4, 2, true);
Type *tyLLong = mkType(TY_LLONG, 8, 2);
Type *tyULLong = mkType(TY_LLONG, 8, 2, true);
Type *tyFloat = mkType(TY_FLOAT, 4, 2);
Type *tyDouble = mkType(TY_DOUBLE, 8, 2);

Type *pointerTo(Type *base) {
    Type *t = mkType(TY_PTR, 4, 2, true);
    t->base = base;
    return t;
}
Type *arrayOf(Type *base, int len) {
    Type *t = mkType(TY_ARRAY, 0, 1);
    t->base = base;
    t->arrayLen = len;
    return t;
}
Type *funcType(Type *ret) {
    Type *t = mkType(TY_FUNC, 1, 1);
    t->base = ret;
    return t;
}
Type *copyType(Type *t) {
    Type *n = new Type(*t);
    if ((t->kind == TY_STRUCT || t->kind == TY_UNION) && !n->origin) n->origin = t;
    return n;
}
Type *sdef(Type *t) { return t->origin ? t->origin : t; }
const Type *sdef(const Type *t) { return t->origin ? t->origin : t; }
Type *unqual(Type *t) {
    if (!t->isConst && !t->isVolatile) return t;
    if (t->origin) return t->origin;
    Type *n = new Type(*t);
    n->isConst = n->isVolatile = false;
    return n;
}
int tsize(const Type *t) {
    switch (t->kind) {
    case TY_STRUCT:
    case TY_UNION: return sdef(t)->size;
    case TY_ARRAY: return t->arrayLen < 0 ? 0 : t->arrayLen * tsize(t->base);
    default: return t->size;
    }
}
int talign(const Type *t) {
    switch (t->kind) {
    case TY_STRUCT:
    case TY_UNION: return sdef(t)->align;
    case TY_ARRAY: return talign(t->base);
    default: return t->align;
    }
}
bool isInteger(const Type *t) {
    return t->kind == TY_BOOL || t->kind == TY_CHAR || t->kind == TY_SHORT || t->kind == TY_INT ||
           t->kind == TY_LONG || t->kind == TY_LLONG || t->kind == TY_ENUM;
}
bool isFlonum(const Type *t) { return t->kind == TY_FLOAT || t->kind == TY_DOUBLE; }
bool isArith(const Type *t) { return isInteger(t) || isFlonum(t); }
bool isScalar(const Type *t) { return isArith(t) || t->kind == TY_PTR; }
bool isAggregate(const Type *t) { return t->kind == TY_STRUCT || t->kind == TY_UNION || t->kind == TY_ARRAY; }

bool sameType(const Type *a, const Type *b) {
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case TY_STRUCT:
    case TY_UNION: return sdef(a) == sdef(b);
    case TY_PTR: return sameType(a->base, b->base);
    case TY_ARRAY: return a->arrayLen == b->arrayLen && sameType(a->base, b->base);
    case TY_FUNC: {
        if (!sameType(a->base, b->base) || a->variadic != b->variadic || a->cc != b->cc) return false;
        if (a->oldStyle || b->oldStyle) return true;
        if (a->params.size() != b->params.size()) return false;
        for (size_t i = 0; i < a->params.size(); i++)
            if (!sameType(a->params[i], b->params[i])) return false;
        return true;
    }
    default: return a->isUnsigned == b->isUnsigned && a->size == b->size;
    }
}

std::string typeName(const Type *t) {
    switch (t->kind) {
    case TY_VOID: return "void";
    case TY_BOOL: return "_Bool";
    case TY_CHAR: return t->isUnsigned ? "unsigned char" : "char";
    case TY_SHORT: return t->isUnsigned ? "unsigned short" : "short";
    case TY_INT:
    case TY_ENUM: return t->isUnsigned ? "unsigned int" : "int";
    case TY_LONG: return t->isUnsigned ? "unsigned long" : "long";
    case TY_LLONG: return t->isUnsigned ? "unsigned long long" : "long long";
    case TY_FLOAT: return "float";
    case TY_DOUBLE: return "double";
    case TY_PTR: return typeName(t->base) + "*";
    case TY_ARRAY: return typeName(t->base) + "[]";
    case TY_FUNC: return typeName(t->base) + "()";
    case TY_STRUCT: return "struct " + sdef(t)->tag;
    case TY_UNION: return "union " + sdef(t)->tag;
    }
    return "?";
}

int64_t truncVal(int64_t v, const Type *ty) {
    if (ty->kind == TY_BOOL) return v != 0;
    if (ty->kind == TY_PTR) return (int64_t)(uint32_t)v;
    switch (ty->size) {
    case 1: return ty->isUnsigned ? (int64_t)(uint8_t)v : (int64_t)(int8_t)v;
    case 2: return ty->isUnsigned ? (int64_t)(uint16_t)v : (int64_t)(int16_t)v;
    case 4: return ty->isUnsigned ? (int64_t)(uint32_t)v : (int64_t)(int32_t)v;
    default: return v;
    }
}

// ------------------------------------------------------------ parser state
Token *tok;
Obj *curFn;
Program *prog;

struct Scope {
    std::unordered_map<std::string, VarScope> vars;
    std::unordered_map<std::string, Type *> tags;
};
static std::vector<Scope *> scopes;
static std::map<std::string, RegParams> regParamPragmas;
static int uniqueCounter = 0;
// C++: a bare tag name (struct/class/union/enum X) also names a type, in any enclosing scope.
static std::set<std::string> cxxTypeTags;

bool equal(const char *s) { return tok->is(s); }
bool consume(const char *s) {
    if (tok->is(s)) { tok = tok->next; return true; }
    return false;
}
void expect(const char *s) {
    if (!tok->is(s)) errorAt(tok, "expected '%s'", s);
    tok = tok->next;
}

static const char *keywords[] = {
    "return", "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "goto",
    "sizeof", "void", "char", "short", "int", "long", "float", "double", "signed", "unsigned", "struct",
    "union", "enum", "typedef", "static", "extern", "inline", "register", "auto", "const", "volatile",
    "restrict", "_Bool", "_Alignof", "_Alignas", "_Noreturn", "_Static_assert", "_Thread_local", "_Atomic",
    "__attribute__", "__asm__", "asm", "__typeof__", "typeof", "pascal", "_Generic", nullptr};
static std::set<std::string> kwSet;

bool isIdentTok(const Token *t) {
    if (t->kind != Token::IDENT) return false;
    if (kwSet.empty())
        for (int i = 0; keywords[i]; i++) kwSet.insert(keywords[i]);
    return !kwSet.count(t->text);
}

void enterScope() { scopes.push_back(new Scope); }
void leaveScope() { scopes.pop_back(); }
VarScope *findVar(const std::string &name) {
    for (size_t i = scopes.size(); i-- > 0;) {
        auto it = scopes[i]->vars.find(name);
        if (it != scopes[i]->vars.end()) return &it->second;
    }
    return nullptr;
}
VarScope *pushVar(const std::string &name) {
    VarScope &v = scopes.back()->vars[name];
    v = VarScope();
    return &v;
}
Type *findTag(const std::string &name) {
    for (size_t i = scopes.size(); i-- > 0;) {
        auto it = scopes[i]->tags.find(name);
        if (it != scopes[i]->tags.end()) return it->second;
    }
    return nullptr;
}

std::string uniqueName(const char *prefix) { return format("%s%d", prefix, uniqueCounter++); }
int newLabelId() { return uniqueCounter++; }

Obj *newLocal(const std::string &name, Type *ty, Token *t) {
    Obj *v = new Obj;
    v->name = name;
    v->ty = ty;
    v->tok = t;
    v->isLocal = true;
    if (!name.empty()) pushVar(name)->var = v;
    if (curFn) curFn->locals.push_back(v);
    return v;
}
Obj *newGlobal(const std::string &name, Type *ty, Token *t) {
    Obj *v = new Obj;
    v->name = name;
    v->asmName = name;
    v->ty = ty;
    v->tok = t;
    prog->globals.push_back(v);
    return v;
}
Obj *newAnonGlobal(Type *ty, Token *t) {
    Obj *v = newGlobal(uniqueName(".LC"), ty, t);
    v->isStatic = true;
    v->isDefinition = true;
    return v;
}
static std::map<std::string, Obj *> stringPool;
Obj *newStringLiteral(const std::string &bytes, Token *t) {
    auto it = stringPool.find(bytes);
    if (it != stringPool.end()) return it->second;
    Type *ty = arrayOf(tyChar, (int)bytes.size() + 1);
    Obj *v = newAnonGlobal(ty, t);
    v->data.assign(bytes.begin(), bytes.end());
    v->data.push_back(0);
    v->isBss = false;
    v->isString = true;
    v->isConstData = true;
    v->align = 1;
    stringPool[bytes] = v;
    return v;
}

// ------------------------------------------------------------ attributes
static void skipBalanced() { // tok at '(' : skip to after matching ')'
    int depth = 0;
    do {
        if (tok->kind == Token::END) errorAt(tok, "unbalanced parentheses");
        if (equal("(")) depth++;
        else if (equal(")")) depth--;
        tok = tok->next;
    } while (depth > 0);
}

static std::string attrName(const std::string &s) {
    if (s.size() > 4 && s.compare(0, 2, "__") == 0 && s.compare(s.size() - 2, 2, "__") == 0) return s.substr(2, s.size() - 4);
    return s;
}

static std::string parseStringLit() {
    if (tok->kind != Token::STR) errorAt(tok, "expected string literal");
    std::string s;
    while (tok->kind == Token::STR) { s += tok->sval; tok = tok->next; }
    return s;
}

void parseAttributes(Attrs *attr) {
    for (;;) {
        if (equal("__attribute__")) {
            tok = tok->next;
            expect("(");
            expect("(");
            while (!equal(")")) {
                if (consume(",")) continue;
                std::string n = attrName(tok->text);
                tok = tok->next;
                if (n == "packed") { if (attr) attr->packed = true; }
                else if (n == "noreturn") { if (attr) attr->noreturn = true; }
                else if (n == "always_inline") { if (attr) attr->alwaysInline = true; }
                else if (n == "noinline") { if (attr) attr->noInline = true; }
                else if (n == "weak") { if (attr) attr->weak = true; }
                else if (n == "gnu_inline") { if (attr) attr->gnuInline = true; }
                else if (n == "used") { if (attr) attr->used = true; }
                else if (n == "pascal") { if (attr) attr->isPascal = true; }
                else if (n == "aligned") {
                    int a = 2;
                    if (consume("(")) { a = (int)constExpr(); expect(")"); }
                    if (attr) attr->align = a;
                    continue;
                } else if (n == "section") {
                    expect("(");
                    std::string s = parseStringLit();
                    expect(")");
                    if (attr) attr->section = s;
                    continue;
                } else if (n == "unavailable" || n == "deprecated" || n == "error") {
                    std::string s = "unavailable";
                    if (consume("(")) { s = parseStringLit(); expect(")"); }
                    if (attr && n != "deprecated") attr->unavailable = s;
                    continue;
                }
                if (equal("(")) skipBalanced();
            }
            expect(")");
            expect(")");
            continue;
        }
        if ((equal("__asm__") || equal("asm")) && tok->next->is("(") && tok->next->next->kind == Token::STR) {
            tok = tok->next->next;
            std::string s = parseStringLit();
            expect(")");
            if (attr) attr->asmName = s;
            continue;
        }
        if (equal("[") && tok->next->is("[")) { // C++11 / C23 [[attribute]]
            int depth = 0;
            do {
                if (equal("[")) depth++;
                else if (equal("]")) depth--;
                tok = tok->next;
            } while (depth > 0);
            continue;
        }
        break;
    }
}

// ------------------------------------------------------------ struct / union / enum
static int alignUp(int n, int a) { return a <= 1 ? n : (n + a - 1) / a * a; }

static Type *structDecl(bool isUnion) {
    Attrs sa;
    parseAttributes(&sa);
    Token *tag = nullptr;
    if (isIdentTok(tok)) { tag = tok; tok = tok->next; }
    if (tag && !equal("{")) {
        Type *t = findTag(tag->text);
        if (t) return t;
        t = mkType(isUnion ? TY_UNION : TY_STRUCT, 0, 1);
        t->complete = false;
        t->tag = tag->text;
        scopes.back()->tags[tag->text] = t; if (opt.cxx) cxxTypeTags.insert(tag->text);
        return t;
    }
    Token *open = tok;
    expect("{");
    Type *ty = nullptr;
    if (tag) {
        auto it = scopes.back()->tags.find(tag->text);
        if (it != scopes.back()->tags.end() && !it->second->complete) ty = it->second;
    }
    if (!ty) {
        ty = mkType(isUnion ? TY_UNION : TY_STRUCT, 0, 1);
        if (tag) { ty->tag = tag->text; scopes.back()->tags[tag->text] = ty; if (opt.cxx) cxxTypeTags.insert(tag->text); }
    }
    ty->kind = isUnion ? TY_UNION : TY_STRUCT;
    ty->complete = false;
    std::vector<Member> mems;
    while (!consume("}")) {
        if (consume(";")) continue;
        if (equal("_Static_assert")) {
            tok = tok->next;
            skipBalanced();
            expect(";");
            continue;
        }
        Attrs a;
        Type *base = declspec(&a);
        // anonymous struct/union member
        if ((base->kind == TY_STRUCT || base->kind == TY_UNION) && equal(";")) {
            Member m;
            m.ty = base;
            m.tok = tok;
            mems.push_back(m);
            tok = tok->next;
            continue;
        }
        bool first = true;
        while (!consume(";")) {
            if (!first) expect(",");
            first = false;
            Member m;
            m.tok = tok;
            Attrs da;
            if (equal(":")) {
                m.ty = base;
            } else {
                Token *name = nullptr;
                m.ty = declarator(base, &name, &da);
                if (name) m.name = name->text;
            }
            if (consume(":")) {
                m.isBitfield = true;
                m.bitWidth = (int)constExpr();
                parseAttributes(&da);
            }
            if (da.align && da.align < talign(m.ty)) { /* cannot lower alignment */ }
            mems.push_back(m);
        }
    }
    parseAttributes(&sa);
    int maxAlign = 2;
    if (open->pack) maxAlign = std::min(maxAlign, open->pack);
    if (sa.packed) maxAlign = 1;
    int offset = 0, align = 1;
    bool inUnit = false;
    int unitOff = 0, unitSize = 0, bitsUsed = 0;
    int usize = 0;
    for (size_t i = 0; i < mems.size(); i++) {
        Member &m = mems[i];
        if (m.ty->kind == TY_ARRAY && m.ty->arrayLen < 0) {
            if (i + 1 != mems.size() || isUnion) errorAt(m.tok, "incomplete array member");
            m.ty = arrayOf(m.ty->base, 0);
        } else if ((m.ty->kind == TY_STRUCT || m.ty->kind == TY_UNION) && !sdef(m.ty)->complete)
            errorAt(m.tok, "member has incomplete type");
        int al = std::min(talign(m.ty), maxAlign);
        if (al < 1) al = 1;
        align = std::max(align, al);
        if (isUnion) {
            m.offset = 0;
            if (m.isBitfield) m.bitOffset = 0;
            usize = std::max(usize, tsize(m.ty));
            continue;
        }
        if (m.isBitfield) {
            int ubits = tsize(m.ty) * 8;
            if (m.bitWidth == 0) {
                if (inUnit) { offset = unitOff + unitSize; inUnit = false; }
                continue;
            }
            if (m.bitWidth > ubits) errorAt(m.tok, "bit-field wider than its type");
            if (!inUnit || unitSize != tsize(m.ty) || bitsUsed + m.bitWidth > ubits) {
                if (inUnit) offset = unitOff + unitSize;
                offset = alignUp(offset, al);
                unitOff = offset;
                unitSize = tsize(m.ty);
                bitsUsed = 0;
                inUnit = true;
            }
            m.offset = unitOff;
            m.bitOffset = bitsUsed;
            bitsUsed += m.bitWidth;
            continue;
        }
        if (inUnit) { offset = unitOff + unitSize; inUnit = false; }
        offset = alignUp(offset, al);
        m.offset = offset;
        offset += tsize(m.ty);
    }
    if (inUnit) offset = unitOff + unitSize;
    if (sa.align > align && sa.align <= 2) align = sa.align;
    ty->members = mems;
    ty->align = align;
    ty->size = alignUp(isUnion ? usize : offset, align);
    ty->complete = true;
    ty->packed = sa.packed;
    return ty;
}

Member *findMember(Type *ty, const std::string &name, int *extraOffset) {
    Type *d = sdef(ty);
    for (auto &m : d->members) {
        if (!m.name.empty() && m.name == name) return &m;
    }
    (void)extraOffset;
    return nullptr;
}

static Type *enumDecl() {
    Attrs ea;
    parseAttributes(&ea);
    Token *tag = nullptr;
    if (isIdentTok(tok)) { tag = tok; tok = tok->next; }
    if (opt.cxx && consume(":")) { Attrs a; declspec(&a); }
    if (tag && !equal("{")) {
        Type *t = findTag(tag->text);
        if (t) return t;
        t = copyType(tyInt);
        scopes.back()->tags[tag->text] = t; if (opt.cxx) cxxTypeTags.insert(tag->text);
        return t;
    }
    expect("{");
    Type *ty = copyType(tyInt);
    if (tag) { scopes.back()->tags[tag->text] = ty; if (opt.cxx) cxxTypeTags.insert(tag->text); }
    int64_t val = 0;
    while (!consume("}")) {
        if (!isIdentTok(tok)) errorAt(tok, "expected enumerator name");
        std::string name = tok->text;
        tok = tok->next;
        parseAttributes(nullptr);
        if (consume("=")) val = constExpr();
        VarScope *vs = pushVar(name);
        vs->isEnum = true;
        vs->enumVal = val;
        vs->enumTy = (val > 0x7FFFFFFFLL) ? tyUInt : tyInt;
        val++;
        if (!consume(",")) { expect("}"); break; }
    }
    parseAttributes(&ea);
    return ty;
}

// ------------------------------------------------------------ declaration specifiers
bool isTypeName(Token *t) {
    if (t->kind != Token::IDENT) return false;
    static std::set<std::string> names = {
        "void", "char", "short", "int", "long", "float", "double", "signed", "unsigned", "struct", "union",
        "enum", "typedef", "static", "extern", "inline", "register", "auto", "const", "volatile", "restrict",
        "_Bool", "_Alignas", "_Noreturn", "_Thread_local", "_Atomic", "__attribute__", "__typeof__", "typeof",
        "pascal", "__builtin_va_list"};
    if (names.count(t->text)) return true;
    if (opt.cxx && (t->text == "bool" || t->text == "class" || t->text == "constexpr" ||
                    t->text == "wchar_t" || t->text == "char16_t" || t->text == "char32_t")) return true;
    if (opt.cxx && cxxTypeTags.count(t->text)) return true;
    VarScope *vs = findVar(t->text);
    return vs && vs->typeDef;
}

Type *declspec(Attrs *attr) {
    enum { VOID = 1 << 0, BOOL = 1 << 2, CHAR = 1 << 4, SHORT = 1 << 6, INT = 1 << 8, LONG = 1 << 10,
           FLOAT = 1 << 12, DOUBLE = 1 << 14, OTHER = 1 << 16, SIGNED = 1 << 17, UNSIGNED = 1 << 18 };
    int counter = 0;
    Type *ty = tyInt;
    bool isConst = false, isVolatile = false;
    Token *start = tok;
    for (;;) {
        if (tok->kind != Token::IDENT) break;
        const std::string &s = tok->text;
        if (s == "typedef" || s == "static" || s == "extern" || s == "inline" || s == "register" || s == "auto" ||
            s == "_Noreturn" || s == "_Thread_local" || s == "pascal" || (opt.cxx && s == "constexpr")) {
            if (!attr) errorAt(tok, "storage class specifier is not allowed here");
            if (s == "typedef") attr->isTypedef = true;
            else if (s == "static") attr->isStatic = true;
            else if (s == "extern") attr->isExtern = true;
            else if (s == "inline") attr->isInline = true;
            else if (s == "register") attr->isRegister = true;
            else if (s == "_Noreturn") attr->noreturn = true;
            else if (s == "pascal") attr->isPascal = true;
            else if (s == "constexpr") { attr->isConstexpr = true; isConst = true; }
            tok = tok->next;
            continue;
        }
        if (s == "const") { isConst = true; tok = tok->next; continue; }
        if (s == "volatile") { isVolatile = true; tok = tok->next; continue; }
        if (s == "restrict" || s == "_Atomic") { tok = tok->next; continue; }
        if (s == "__attribute__") { parseAttributes(attr); continue; }
        if (s == "_Alignas") {
            tok = tok->next;
            expect("(");
            int a;
            if (isTypeName(tok)) a = talign(typeName());
            else a = (int)constExpr();
            expect(")");
            if (attr) attr->align = a;
            continue;
        }
        VarScope *vs = findVar(s);
        bool isTypedefName = vs && vs->typeDef;
        // C++: a bare tag name (no struct/class/enum keyword) names the type it was defined as.
        bool bareTag = opt.cxx && !isTypedefName && cxxTypeTags.count(s) && !(vs && (vs->var || vs->isEnum));
        if (bareTag) {
            Type *tg = findTag(s);
            if (tg) {
                if (counter) break;
                ty = tg;
                tok = tok->next;
                counter += OTHER;
                continue;
            }
        }
        if (s == "struct" || s == "union" || s == "enum" || s == "__typeof__" || s == "typeof" ||
            s == "__builtin_va_list" || (opt.cxx && s == "class") || isTypedefName) {
            if (counter) break;
            if (s == "struct" || s == "class") { tok = tok->next; ty = structDecl(false); }
            else if (s == "union") { tok = tok->next; ty = structDecl(true); }
            else if (s == "enum") { tok = tok->next; if (opt.cxx && (equal("class") || equal("struct"))) tok = tok->next; ty = enumDecl(); }
            else if (s == "__builtin_va_list") { tok = tok->next; ty = pointerTo(tyChar); }
            else if (s == "__typeof__" || s == "typeof") {
                tok = tok->next;
                expect("(");
                if (isTypeName(tok)) ty = typeName();
                else ty = expr()->ty;
                expect(")");
            } else { ty = vs->typeDef; tok = tok->next; }
            counter += OTHER;
            continue;
        }
        if (s == "void") counter += VOID;
        else if (s == "_Bool" || (opt.cxx && s == "bool")) counter += BOOL;
        else if (s == "char") counter += CHAR;
        else if (s == "short") counter += SHORT;
        else if (s == "int") counter += INT;
        else if (s == "long") counter += LONG;
        else if (s == "float") counter += FLOAT;
        else if (s == "double") counter += DOUBLE;
        else if (s == "signed") counter |= SIGNED;
        else if (s == "unsigned") counter |= UNSIGNED;
        else break;
        tok = tok->next;
        switch (counter) {
        case VOID: ty = tyVoid; break;
        case BOOL: ty = tyBool; break;
        case CHAR: ty = tyChar; break;
        case SIGNED + CHAR: ty = tySChar; break;
        case UNSIGNED + CHAR: ty = tyUChar; break;
        case SHORT: case SHORT + INT: case SIGNED + SHORT: case SIGNED + SHORT + INT: ty = tyShort; break;
        case UNSIGNED + SHORT: case UNSIGNED + SHORT + INT: ty = tyUShort; break;
        case INT: case SIGNED: case SIGNED + INT: ty = tyInt; break;
        case UNSIGNED: case UNSIGNED + INT: ty = tyUInt; break;
        case LONG: case LONG + INT: case SIGNED + LONG: case SIGNED + LONG + INT: ty = tyLong; break;
        case UNSIGNED + LONG: case UNSIGNED + LONG + INT: ty = tyULong; break;
        case LONG + LONG: case LONG + LONG + INT: case SIGNED + LONG + LONG: case SIGNED + LONG + LONG + INT:
            ty = tyLLong; break;
        case UNSIGNED + LONG + LONG: case UNSIGNED + LONG + LONG + INT: ty = tyULLong; break;
        case FLOAT: ty = tyFloat; break;
        case DOUBLE: case LONG + DOUBLE: ty = tyDouble; break;
        default: errorAt(tok, "invalid combination of type specifiers");
        }
    }
    if (tok == start) errorAt(tok, "expected a type");
    if (isConst || isVolatile) {
        ty = copyType(ty);
        ty->isConst = ty->isConst || isConst;
        ty->isVolatile = ty->isVolatile || isVolatile;
    }
    return ty;
}

// ------------------------------------------------------------ declarators
static Type *typeSuffix(Type *ty);

static Type *funcParams(Type *ret) {
    Type *fn = funcType(ret);
    if (equal("void") && tok->next->is(")")) {
        tok = tok->next->next;
        return fn;
    }
    if (consume(")")) {
        fn->oldStyle = !opt.cxx;
        return fn;
    }
    for (;;) {
        if (consume("...")) {
            fn->variadic = true;
            expect(")");
            break;
        }
        Attrs a;
        Type *base = declspec(&a);
        Token *name = nullptr;
        Type *pt = declarator(base, &name, &a);
        if (pt->kind == TY_ARRAY) {
            Type *p = pointerTo(pt->base);
            pt = p;
        } else if (pt->kind == TY_FUNC)
            pt = pointerTo(pt);
        if (a.isPascal && pt->kind == TY_PTR && pt->base->kind == TY_FUNC) {
            Type *f = copyType(pt->base);
            f->cc = CC_PASCAL;
            pt = pointerTo(f);
        }
        fn->params.push_back(pt);
        fn->paramNames.push_back(name ? name->text : "");
        if (opt.cxx && consume("=")) assign(); // default argument (value ignored for now)
        if (consume(")")) break;
        expect(",");
    }
    return fn;
}

static Type *typeSuffix(Type *ty) {
    if (consume("(")) return funcParams(ty);
    if (consume("[")) {
        while (equal("static") || equal("const") || equal("restrict") || equal("volatile")) tok = tok->next;
        int len = -1;
        if (!equal("]")) {
            Token *t = tok;
            int64_t v = constExpr();
            if (v < 0) errorAt(t, "negative array size");
            len = (int)v;
        }
        expect("]");
        ty = typeSuffix(ty);
        return arrayOf(ty, len);
    }
    return ty;
}

static Type *pointers(Type *ty, Attrs *attr) {
    for (;;) {
        if (consume("*")) ty = pointerTo(ty);
        else if (equal("pascal") && tok->next->is("*")) { if (attr) attr->isPascal = true; tok = tok->next; continue; }
        else break;
        for (;;) {
            if (consume("const")) { ty = copyType(ty); ty->isConst = true; }
            else if (consume("volatile")) { ty = copyType(ty); ty->isVolatile = true; }
            else if (consume("restrict")) {}
            else if (equal("__attribute__")) parseAttributes(attr);
            else break;
        }
    }
    return ty;
}

Type *declarator(Type *ty, Token **nameTok, Attrs *attr) {
    parseAttributes(attr);
    ty = pointers(ty, attr);
    parseAttributes(attr);
    if (equal("pascal")) { if (attr) attr->isPascal = true; tok = tok->next; }
    if (equal("(")) {
        // "(" starts either a nested declarator or a parameter list of an abstract function type
        Token *n = tok->next;
        bool isParams = n->is(")") || n->is("...") || (isTypeName(n) && !n->is("pascal") && !n->is("__attribute__"));
        if (!isParams) {
            Token *start = tok->next;
            tok = start;
            Type dummy;
            dummy.kind = TY_INT;
            Token *ignored = nullptr;
            Attrs ia;
            declarator(&dummy, &ignored, &ia);
            expect(")");
            ty = typeSuffix(ty);
            Token *end = tok;
            tok = start;
            Attrs inner;
            ty = declarator(ty, nameTok, &inner);
            if (inner.isPascal && attr) attr->isPascal = true;
            tok = end;
            parseAttributes(attr);
            return ty;
        }
    }
    if (isIdentTok(tok)) {
        if (nameTok) *nameTok = tok;
        tok = tok->next;
    }
    ty = typeSuffix(ty);
    parseAttributes(attr);
    return ty;
}

// make the first function type reachable through pointers/arrays use the Pascal convention
static Type *applyPascal(Type *ty) {
    if (ty->kind == TY_FUNC) {
        Type *f = copyType(ty);
        f->cc = CC_PASCAL;
        return f;
    }
    if (ty->kind == TY_PTR || ty->kind == TY_ARRAY) {
        Type *n = copyType(ty);
        n->base = applyPascal(ty->base);
        return n;
    }
    return ty;
}

Type *typeName() {
    Attrs a;
    Type *base = declspec(&a);
    Token *name = nullptr;
    Type *ty = declarator(base, &name, &a);
    if (name) errorAt(name, "unexpected identifier in type name");
    if (a.isPascal) ty = applyPascal(ty);
    return ty;
}

void parseTypedef(Type *base, Attrs *attr) {
    bool first = true;
    while (!consume(";")) {
        if (!first) expect(",");
        first = false;
        Attrs da;
        Token *name = nullptr;
        Type *ty = declarator(base, &name, &da);
        if (!name) errorAt(tok, "typedef name omitted");
        if (attr->isPascal || da.isPascal) ty = applyPascal(ty);
        pushVar(name->text)->typeDef = ty;
    }
}

// ------------------------------------------------------------ constant evaluation
static bool evalLval(Node *n, int64_t *val, std::string *sym) {
    switch (n->kind) {
    case ND_VAR:
        if (n->var->isLocal) return false;
        if (!sym || !sym->empty()) return false;
        *sym = n->var->asmName;
        n->var->referenced = true;
        *val = 0;
        return true;
    case ND_DEREF: return evalConst(n->lhs, val, sym);
    case ND_MEMBER:
        if (!evalLval(n->lhs, val, sym)) return false;
        *val += n->member->offset;
        return true;
    default: return false;
    }
}

bool evalFloat(Node *n, double *val) {
    switch (n->kind) {
    case ND_FNUM: *val = n->fval; return true;
    case ND_NUM: *val = (double)n->ival; return true;
    case ND_NEG: if (!evalFloat(n->lhs, val)) return false; *val = -*val; return true;
    case ND_CAST: {
        if (isFlonum(n->lhs->ty)) {
            if (!evalFloat(n->lhs, val)) return false;
        } else {
            int64_t v;
            if (!evalConst(n->lhs, &v, nullptr)) return false;
            *val = n->lhs->ty->isUnsigned ? (double)(uint64_t)v : (double)v;
        }
        if (n->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    }
    case ND_ADD: case ND_SUB: case ND_MUL: case ND_DIV: {
        double a, b;
        if (!evalFloat(n->lhs, &a) || !evalFloat(n->rhs, &b)) return false;
        *val = n->kind == ND_ADD ? a + b : n->kind == ND_SUB ? a - b : n->kind == ND_MUL ? a * b : a / b;
        if (n->ty->kind == TY_FLOAT) *val = (float)*val;
        return true;
    }
    case ND_COND: {
        int64_t c;
        if (!evalConst(n->cond, &c, nullptr)) return false;
        return evalFloat(c ? n->then : n->els, val);
    }
    case ND_COMMA: return evalFloat(n->rhs, val);
    default: return false;
    }
}

bool evalConst(Node *n, int64_t *val, std::string *sym) {
    int64_t a = 0, b = 0;
    if (isFlonum(n->ty)) return false;
    switch (n->kind) {
    case ND_NUM: *val = n->ival; return true;
    case ND_ADD:
        if (!evalConst(n->lhs, &a, sym)) return false;
        if (!evalConst(n->rhs, &b, sym)) return false;
        *val = a + b;
        break;
    case ND_SUB: {
        std::string s2;
        if (!evalConst(n->lhs, &a, sym)) return false;
        if (!evalConst(n->rhs, &b, &s2)) return false;
        if (!s2.empty()) {
            if (!sym || *sym != s2) return false;
            sym->clear(); // &x[i] - &x[j]
        }
        *val = a - b;
        break;
    }
    case ND_MUL: case ND_DIV: case ND_MOD: case ND_AND: case ND_OR: case ND_XOR: case ND_SHL: case ND_SHR:
    case ND_EQ: case ND_NE: case ND_LT: case ND_LE: {
        if (!evalConst(n->lhs, &a, nullptr) || !evalConst(n->rhs, &b, nullptr)) {
            // comparisons / arithmetic on floating operands
            if ((n->kind == ND_EQ || n->kind == ND_NE || n->kind == ND_LT || n->kind == ND_LE) && isFlonum(n->lhs->ty)) {
                double x, y;
                if (!evalFloat(n->lhs, &x) || !evalFloat(n->rhs, &y)) return false;
                *val = n->kind == ND_EQ ? x == y : n->kind == ND_NE ? x != y : n->kind == ND_LT ? x < y : x <= y;
                return true;
            }
            return false;
        }
        bool u = n->lhs->ty->isUnsigned;
        bool wide = tsize(n->lhs->ty) == 8;
        uint64_t ua = wide ? (uint64_t)a : (uint32_t)a, ub = wide ? (uint64_t)b : (uint32_t)b;
        switch (n->kind) {
        case ND_MUL: *val = a * b; break;
        case ND_DIV:
            if (b == 0) return false;
            *val = u ? (int64_t)(ua / ub) : a / b;
            break;
        case ND_MOD:
            if (b == 0) return false;
            *val = u ? (int64_t)(ua % ub) : a % b;
            break;
        case ND_AND: *val = a & b; break;
        case ND_OR: *val = a | b; break;
        case ND_XOR: *val = a ^ b; break;
        case ND_SHL: *val = (int64_t)((uint64_t)a << (b & 63)); break;
        case ND_SHR: *val = u ? (int64_t)(ua >> (b & 63)) : a >> (b & 63); break;
        case ND_EQ: *val = a == b; return true;
        case ND_NE: *val = a != b; return true;
        case ND_LT: *val = u ? ua < ub : a < b; return true;
        case ND_LE: *val = u ? ua <= ub : a <= b; return true;
        default: break;
        }
        break;
    }
    case ND_NEG: if (!evalConst(n->lhs, &a, nullptr)) return false; *val = -a; break;
    case ND_BITNOT: if (!evalConst(n->lhs, &a, nullptr)) return false; *val = ~a; break;
    case ND_NOT:
        if (isFlonum(n->lhs->ty)) { double d; if (!evalFloat(n->lhs, &d)) return false; *val = d == 0; return true; }
        if (!evalConst(n->lhs, &a, nullptr)) return false;
        *val = !a;
        return true;
    case ND_LOGAND:
        if (!evalConst(n->lhs, &a, nullptr)) return false;
        if (!a) { *val = 0; return true; }
        if (!evalConst(n->rhs, &b, nullptr)) return false;
        *val = b != 0;
        return true;
    case ND_LOGOR:
        if (!evalConst(n->lhs, &a, nullptr)) return false;
        if (a) { *val = 1; return true; }
        if (!evalConst(n->rhs, &b, nullptr)) return false;
        *val = b != 0;
        return true;
    case ND_COND:
        if (!evalConst(n->cond, &a, nullptr)) return false;
        return evalConst(a ? n->then : n->els, val, sym);
    case ND_COMMA: return evalConst(n->rhs, val, sym);
    case ND_CAST: {
        if (isFlonum(n->lhs->ty)) {
            double d;
            if (!evalFloat(n->lhs, &d)) return false;
            *val = n->ty->isUnsigned && d >= 9223372036854775808.0 ? (int64_t)(uint64_t)d : (int64_t)d;
            *val = truncVal(*val, n->ty);
            return true;
        }
        std::string s;
        if (!evalConst(n->lhs, &a, sym ? sym : &s)) return false;
        if ((sym && !sym->empty()) || !s.empty()) {
            if (!sym || tsize(n->ty) != 4) return false;
            *val = a;
            return true;
        }
        *val = truncVal(a, n->ty);
        return true;
    }
    case ND_ADDR: return evalLval(n->lhs, val, sym);
    case ND_MEMBER:
        if (n->ty->kind != TY_ARRAY) return false;
        return evalLval(n, val, sym);
    case ND_VAR:
        if (n->ty->kind != TY_ARRAY && n->ty->kind != TY_FUNC) return false;
        return evalLval(n, val, sym);
    default: return false;
    }
    if (!sym || sym->empty()) *val = truncVal(*val, n->ty);
    return true;
}

int64_t evalConstExpr(Node *n, const Token *where) {
    int64_t v;
    if (!evalConst(n, &v, nullptr)) errorAt(where, "expression is not an integer constant");
    return v;
}

// ------------------------------------------------------------ initialisers
struct Init {
    Type *ty = nullptr;
    Token *tok = nullptr;
    bool flexible = false;
    Node *expr = nullptr;
    std::vector<Init *> kids;
    int unionIdx = 0;
};

static Init *newInit(Type *ty, bool flexible) {
    Init *in = new Init;
    in->ty = ty;
    if (ty->kind == TY_ARRAY) {
        if (flexible && ty->arrayLen < 0) { in->flexible = true; return in; }
        for (int i = 0; i < ty->arrayLen; i++) in->kids.push_back(newInit(ty->base, false));
    } else if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        Type *d = sdef(ty);
        for (size_t i = 0; i < d->members.size(); i++) {
            Member &m = d->members[i];
            bool flex = flexible && ty->kind == TY_STRUCT && i + 1 == d->members.size() && m.ty->kind == TY_ARRAY && m.ty->arrayLen == 0;
            if (flex) {
                Init *k = new Init;
                k->ty = arrayOf(m.ty->base, -1);
                k->flexible = true;
                in->kids.push_back(k);
            } else
                in->kids.push_back(newInit(m.ty, false));
        }
    }
    return in;
}

static void initializer2(Init *in);

static bool isEndTok() { return equal("}") || (equal(",") && tok->next->is("}")); }
static bool consumeEnd() {
    if (consume("}")) return true;
    if (equal(",") && tok->next->is("}")) { tok = tok->next->next; return true; }
    return false;
}
static void skipExcess() {
    if (consume("{")) {
        do { skipExcess(); } while (!consumeEnd() && (consume(","), true));
        return;
    }
    assign();
}

static void stringInit(Init *in) {
    std::string s = parseStringLit();
    if (in->flexible) {
        Type *t = arrayOf(in->ty->base, (int)s.size() + 1);
        *in = *newInit(t, false);
    }
    int len = std::min(in->ty->arrayLen, (int)s.size() + 1);
    for (int i = 0; i < len; i++) {
        int c = i < (int)s.size() ? (unsigned char)s[i] : 0;
        in->kids[i]->expr = newNum(in->ty->base->isUnsigned ? c : (signed char)c, tok, in->ty->base);
    }
}

static int countArrayInit(Type *ty) {
    Token *save = tok;
    Init *dummy = newInit(ty->base, true);
    int i = 0, mx = 0;
    bool first = true;
    while (!consumeEnd()) {
        if (!first) expect(",");
        first = false;
        if (equal("[")) {
            tok = tok->next;
            i = (int)constExpr();
            if (consume("...")) i = (int)constExpr();
            expect("]");
            consume("=");
            initializer2(dummy);
        } else
            initializer2(dummy);
        i++;
        mx = std::max(mx, i);
    }
    tok = save;
    return mx;
}

static void structInit2(Init *in, size_t from);
static void arrayInit2(Init *in, int from);

static void designation(Init *in) {
    if (equal("[")) {
        if (in->ty->kind != TY_ARRAY) errorAt(tok, "array index in non-array initializer");
        tok = tok->next;
        int begin = (int)constExpr(), end = begin;
        if (consume("...")) end = (int)constExpr();
        expect("]");
        if (begin < 0 || end >= in->ty->arrayLen || end < begin) errorAt(tok, "array designator index out of range");
        Token *save = tok;
        for (int i = begin; i <= end; i++) {
            tok = save;
            designation(in->kids[i]);
        }
        arrayInit2(in, end + 1);
        return;
    }
    if (equal(".") && (in->ty->kind == TY_STRUCT || in->ty->kind == TY_UNION)) {
        tok = tok->next;
        Type *d = sdef(in->ty);
        size_t idx = d->members.size();
        for (size_t i = 0; i < d->members.size(); i++)
            if (d->members[i].name == tok->text) idx = i;
        if (idx == d->members.size()) {
            // search anonymous members
            for (size_t i = 0; i < d->members.size() && idx == d->members.size(); i++) {
                Member &m = d->members[i];
                if (m.name.empty() && (m.ty->kind == TY_STRUCT || m.ty->kind == TY_UNION) && findMember(m.ty, tok->text, nullptr)) {
                    idx = i;
                    tok = tok; // re-parse ".name" against the anonymous member
                    Token *nameTok = tok;
                    (void)nameTok;
                    // rewind one token so the nested designation sees ".name"
                    static Token dot;
                    dot = *tok;
                    dot.kind = Token::PUNCT;
                    dot.text = ".";
                    dot.next = tok;
                    tok = &dot;
                    if (in->ty->kind == TY_UNION) in->unionIdx = (int)i;
                    designation(in->kids[i]);
                    if (in->ty->kind == TY_STRUCT) structInit2(in, i + 1);
                    return;
                }
            }
            errorAt(tok, "struct has no member named '%s'", tok->text.c_str());
        }
        tok = tok->next;
        if (in->ty->kind == TY_UNION) {
            in->unionIdx = (int)idx;
            designation(in->kids[idx]);
            return;
        }
        designation(in->kids[idx]);
        structInit2(in, idx + 1);
        return;
    }
    if (equal(".")) errorAt(tok, "field name not in struct or union initializer");
    consume("=");
    initializer2(in);
}

static void arrayInit1(Init *in) {
    expect("{");
    if (in->flexible) {
        int n = countArrayInit(in->ty);
        *in = *newInit(arrayOf(in->ty->base, n), false);
    }
    bool first = true;
    for (int i = 0; !consumeEnd(); i++) {
        if (!first) expect(",");
        first = false;
        if (equal("[")) {
            tok = tok->next;
            int begin = (int)constExpr(), end = begin;
            if (consume("...")) end = (int)constExpr();
            expect("]");
            if (begin < 0 || end >= in->ty->arrayLen || end < begin) errorAt(tok, "array designator index out of range");
            Token *save = tok;
            for (int j = begin; j <= end; j++) {
                tok = save;
                designation(in->kids[j]);
            }
            i = end;
            continue;
        }
        if (i < in->ty->arrayLen) initializer2(in->kids[i]);
        else skipExcess();
    }
}

static void arrayInit2(Init *in, int from) {
    if (in->flexible) {
        // count elements without braces: runs to the end of the enclosing list
        Token *save = tok;
        Init *dummy = newInit(in->ty->base, true);
        int n = from;
        for (; !isEndTok(); n++) {
            if (n > 0) expect(",");
            if (equal("[") || equal(".")) break;
            initializer2(dummy);
        }
        tok = save;
        *in = *newInit(arrayOf(in->ty->base, n), false);
    }
    for (int i = from; i < in->ty->arrayLen && !isEndTok(); i++) {
        Token *start = tok;
        if (i > 0) expect(",");
        if (equal("[") || equal(".")) { tok = start; return; }
        initializer2(in->kids[i]);
    }
}

static void structInit1(Init *in) {
    expect("{");
    Type *d = sdef(in->ty);
    size_t i = 0;
    bool first = true;
    while (!consumeEnd()) {
        if (!first) expect(",");
        first = false;
        if (equal(".")) {
            tok = tok->next;
            size_t idx = d->members.size();
            for (size_t k = 0; k < d->members.size(); k++)
                if (d->members[k].name == tok->text) idx = k;
            if (idx == d->members.size()) {
                for (size_t k = 0; k < d->members.size(); k++) {
                    Member &m = d->members[k];
                    if (m.name.empty() && (m.ty->kind == TY_STRUCT || m.ty->kind == TY_UNION) && findMember(m.ty, tok->text, nullptr)) {
                        idx = k;
                        break;
                    }
                }
                if (idx == d->members.size()) errorAt(tok, "struct has no member named '%s'", tok->text.c_str());
                Token *dot = new Token(*tok);
                dot->kind = Token::PUNCT;
                dot->text = ".";
                dot->next = tok;
                tok = dot;
                designation(in->kids[idx]);
                i = idx + 1;
                continue;
            }
            tok = tok->next;
            designation(in->kids[idx]);
            i = idx + 1;
            continue;
        }
        // skip unnamed bit-field padding members
        while (i < d->members.size() && d->members[i].name.empty() && d->members[i].isBitfield) i++;
        if (i < d->members.size()) initializer2(in->kids[i++]);
        else skipExcess();
    }
}

static void structInit2(Init *in, size_t from) {
    Type *d = sdef(in->ty);
    bool first = true;
    for (size_t i = from; i < d->members.size() && !isEndTok(); i++) {
        Token *start = tok;
        if (i > 0 || !first) expect(",");
        first = false;
        if (equal("[") || equal(".")) { tok = start; return; }
        initializer2(in->kids[i]);
    }
}

static void unionInit(Init *in) {
    Type *d = sdef(in->ty);
    if (equal("{") && tok->next->is(".")) {
        tok = tok->next->next;
        size_t idx = d->members.size();
        for (size_t k = 0; k < d->members.size(); k++)
            if (d->members[k].name == tok->text) idx = k;
        if (idx == d->members.size()) errorAt(tok, "union has no member named '%s'", tok->text.c_str());
        tok = tok->next;
        in->unionIdx = (int)idx;
        designation(in->kids[idx]);
        consume(",");
        expect("}");
        return;
    }
    in->unionIdx = 0;
    if (d->members.empty()) { if (consume("{")) expect("}"); return; }
    if (consume("{")) {
        initializer2(in->kids[0]);
        consume(",");
        expect("}");
    } else
        initializer2(in->kids[0]);
}

static bool isCharType(const Type *t) { return t->kind == TY_CHAR; }

static void initializer2(Init *in) {
    in->tok = tok;
    if (in->ty->kind == TY_ARRAY) {
        if (tok->kind == Token::STR && isCharType(in->ty->base)) { stringInit(in); return; }
        if (equal("{") && tok->next->kind == Token::STR && isCharType(in->ty->base)) {
            tok = tok->next;
            stringInit(in);
            consume(",");
            expect("}");
            return;
        }
        if (equal("{")) arrayInit1(in);
        else arrayInit2(in, 0);
        return;
    }
    if (in->ty->kind == TY_STRUCT) {
        if (equal("{")) { structInit1(in); return; }
        Token *start = tok;
        Node *e = assign();
        if (e->ty->kind == TY_STRUCT && sameType(e->ty, in->ty)) { in->expr = e; return; }
        tok = start;
        if (sdef(in->ty)->members.empty()) errorAt(tok, "invalid initializer");
        structInit2(in, 0);
        return;
    }
    if (in->ty->kind == TY_UNION) {
        if (!equal("{")) {
            Token *start = tok;
            Node *e = assign();
            if (e->ty->kind == TY_UNION && sameType(e->ty, in->ty)) { in->expr = e; return; }
            tok = start;
        }
        unionInit(in);
        return;
    }
    if (consume("{")) {
        if (equal("}")) { tok = tok->next; return; } // {} : zero
        initializer2(in);
        consume(",");
        expect("}");
        return;
    }
    in->expr = assign();
}

static Init *parseInitializer(Type *ty, Type **newTy) {
    Init *in = newInit(ty, true);
    initializer2(in);
    if ((ty->kind == TY_STRUCT) && sdef(ty)->members.size() && in->kids.size() && in->kids.back()->flexible == false) {
        // struct with flexible array member initialised: grow the object
        Type *d = sdef(ty);
        Member &last = d->members.back();
        if (last.ty->kind == TY_ARRAY && last.ty->arrayLen == 0 && in->kids.back()->ty->arrayLen > 0) {
            Type *nt = new Type(*d);
            nt->origin = nullptr;
            nt->members.back().ty = in->kids.back()->ty;
            nt->size = d->size + tsize(in->kids.back()->ty);
            *newTy = nt;
            return in;
        }
    }
    *newTy = in->ty;
    return in;
}

static void putBE(std::vector<unsigned char> &buf, int off, int size, uint64_t v) {
    for (int i = 0; i < size; i++) buf[off + i] = (unsigned char)(v >> (8 * (size - 1 - i)));
}
static uint64_t getBE(const std::vector<unsigned char> &buf, int off, int size) {
    uint64_t v = 0;
    for (int i = 0; i < size; i++) v = (v << 8) | buf[off + i];
    return v;
}

uint32_t floatBits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}
uint64_t doubleBits(double d) {
    uint64_t u;
    memcpy(&u, &d, 8);
    return u;
}

// Writes a constant initialiser. Returns false if some element is not constant.
static bool writeInit(Init *in, Type *ty, std::vector<unsigned char> &buf, int off, std::vector<Reloc> &rel, bool diag) {
    if (ty->kind == TY_ARRAY) {
        int es = tsize(ty->base);
        for (size_t i = 0; i < in->kids.size(); i++)
            if (!writeInit(in->kids[i], ty->base, buf, off + (int)i * es, rel, diag)) return false;
        return true;
    }
    if (ty->kind == TY_STRUCT && !in->expr) {
        Type *d = sdef(ty);
        for (size_t i = 0; i < d->members.size() && i < in->kids.size(); i++) {
            Member &m = d->members[i];
            Init *k = in->kids[i];
            if (m.isBitfield) {
                if (!k->expr) continue;
                int64_t v;
                if (!evalConst(k->expr, &v, nullptr)) {
                    if (diag) errorAt(k->tok, "initializer element is not constant");
                    return false;
                }
                int us = tsize(m.ty);
                uint64_t old = getBE(buf, off + m.offset, us);
                uint64_t mask = (m.bitWidth >= 64 ? ~0ull : ((1ull << m.bitWidth) - 1));
                int shift = us * 8 - m.bitOffset - m.bitWidth;
                old = (old & ~(mask << shift)) | (((uint64_t)v & mask) << shift);
                putBE(buf, off + m.offset, us, old);
                continue;
            }
            if (!writeInit(k, k->ty->kind == TY_ARRAY ? k->ty : m.ty, buf, off + m.offset, rel, diag)) return false;
        }
        return true;
    }
    if (ty->kind == TY_UNION && !in->expr) {
        Type *d = sdef(ty);
        if (d->members.empty()) return true;
        return writeInit(in->kids[in->unionIdx], d->members[in->unionIdx].ty, buf, off, rel, diag);
    }
    if (!in->expr) return true;
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
        if (diag) errorAt(in->tok, "initializer element is not constant");
        return false;
    }
    Node *e = newCast(in->expr, ty);
    if (isFlonum(ty)) {
        double d;
        if (!evalFloat(e, &d)) {
            if (diag) errorAt(in->tok, "initializer element is not constant");
            return false;
        }
        if (ty->kind == TY_FLOAT) putBE(buf, off, 4, floatBits((float)d));
        else putBE(buf, off, 8, doubleBits(d));
        return true;
    }
    int64_t v;
    std::string sym;
    if (!evalConst(e, &v, &sym)) {
        if (diag) errorAt(in->tok, "initializer element is not constant");
        return false;
    }
    if (!sym.empty()) {
        if (tsize(ty) != 4) {
            if (diag) errorAt(in->tok, "address constant does not fit in the initialised object");
            return false;
        }
        rel.push_back({off, sym, v});
        return true;
    }
    putBE(buf, off, tsize(ty), (uint64_t)v);
    return true;
}

static void globalInitializer(Obj *var) {
    Type *nt = nullptr;
    Init *in = parseInitializer(var->ty, &nt);
    var->ty = nt;
    var->data.assign(tsize(nt), 0);
    var->relocs.clear();
    writeInit(in, nt, var->data, 0, var->relocs, true);
    var->isBss = false;
    // all-zero data without relocations can still live in .bss
    if (var->relocs.empty()) {
        bool allZero = true;
        for (unsigned char c : var->data)
            if (c) { allZero = false; break; }
        if (allZero && !var->ty->isConst) { var->isBss = true; var->data.clear(); }
    }
}

// build assignments for a local initialiser
static Node *initDesg(Init *in, Type *ty, Node *desg, Token *t) {
    Node *result = nullptr;
    auto chain = [&](Node *n) {
        if (!n) return;
        if (!result) result = n;
        else {
            Node *c = new Node(ND_COMMA, t);
            c->lhs = result;
            c->rhs = n;
            c->ty = n->ty;
            result = c;
        }
    };
    if (ty->kind == TY_ARRAY) {
        for (size_t i = 0; i < in->kids.size(); i++) {
            Node *elem = newDeref(newAddSub(ND_ADD, rval(desg), newNum((int64_t)i, t), t), t);
            chain(initDesg(in->kids[i], ty->base, elem, t));
        }
        return result;
    }
    if (ty->kind == TY_STRUCT && !in->expr) {
        Type *d = sdef(ty);
        for (size_t i = 0; i < d->members.size() && i < in->kids.size(); i++)
            chain(initDesg(in->kids[i], d->members[i].ty, newMember(desg, &d->members[i], t), t));
        return result;
    }
    if (ty->kind == TY_UNION && !in->expr) {
        Type *d = sdef(ty);
        if (d->members.empty()) return nullptr;
        return initDesg(in->kids[in->unionIdx], d->members[in->unionIdx].ty, newMember(desg, &d->members[in->unionIdx], t), t);
    }
    if (!in->expr) return nullptr;
    return newAssign(desg, in->expr, t);
}

Node *localInitializer(Obj *var, Token *t) {
    Type *nt = nullptr;
    Init *in = parseInitializer(var->ty, &nt);
    var->ty = nt;
    Node *vn = newVarNode(var, t);
    if (!isAggregate(nt)) return initDesg(in, nt, vn, t);
    if (in->expr) return newAssign(vn, in->expr, t); // struct = struct expression
    // fully constant aggregate: copy from a read-only image
    int size = tsize(nt);
    if (size > 8) {
        std::vector<unsigned char> buf(size, 0);
        std::vector<Reloc> rel;
        if (writeInit(in, nt, buf, 0, rel, false)) {
            bool allZero = rel.empty();
            for (unsigned char c : buf)
                if (c) allZero = false;
            Node *z = new Node(ND_MEMZERO, t);
            z->var = var;
            z->ty = tyVoid;
            if (allZero) return z;
            Obj *img = newAnonGlobal(nt, t);
            img->data = buf;
            img->relocs = rel;
            img->isBss = false;
            img->isConstData = true;
            Node *a = new Node(ND_ASSIGN, t);
            a->lhs = vn;
            a->rhs = newVarNode(img, t);
            a->ty = nt;
            return a;
        }
    }
    Node *z = new Node(ND_MEMZERO, t);
    z->var = var;
    z->ty = tyVoid;
    Node *rest = initDesg(in, nt, vn, t);
    if (!rest) return z;
    Node *c = new Node(ND_COMMA, t);
    c->lhs = z;
    c->rhs = rest;
    c->ty = rest->ty;
    return c;
}

// ------------------------------------------------------------ pragmas
static int regFromName(const std::string &n) {
    std::string s = n;
    while (!s.empty() && (s[0] == '_' || s[0] == '%')) s.erase(0, 1);
    for (auto &c : s) c = (char)tolower(c);
    if (s == "sp") return 15;
    if (s == "fp") return 14;
    if (s.size() == 2 && (s[0] == 'd' || s[0] == 'a') && s[1] >= '0' && s[1] <= '7') return (s[0] == 'a' ? 8 : 0) + (s[1] - '0');
    return -1;
}
int physRegFromName(const std::string &n) { return regFromName(n); }

static void handlePragmas(Token *first) {
    std::vector<int> stack;
    int cur = 0;
    Token *prev = nullptr;
    for (Token *t = first; t; t = t->next) {
        if (t->kind == Token::OTHER && t->text == "#pragma") {
            // tokenise the pragma text
            const std::string &s = t->sval;
            std::vector<std::string> w;
            for (size_t i = 0; i < s.size();) {
                if (isspace((unsigned char)s[i])) { i++; continue; }
                if (isalnum((unsigned char)s[i]) || s[i] == '_') {
                    size_t j = i;
                    while (j < s.size() && (isalnum((unsigned char)s[j]) || s[j] == '_')) j++;
                    w.push_back(s.substr(i, j - i));
                    i = j;
                } else
                    w.push_back(std::string(1, s[i++]));
            }
            if (!w.empty() && w[0] == "pack") {
                // pack() pack(n) pack(push) pack(push,n) pack(pop)
                std::vector<std::string> a(w.begin() + 1, w.end());
                bool push = false, pop = false;
                int val = -1;
                for (auto &x : a) {
                    if (x == "push") push = true;
                    else if (x == "pop") pop = true;
                    else if (isdigit((unsigned char)x[0])) val = atoi(x.c_str());
                }
                if (push) stack.push_back(cur);
                if (pop) {
                    if (!stack.empty()) { cur = stack.back(); stack.pop_back(); }
                    else cur = 0;
                } else if (val >= 0) cur = val;
                else if (!push) cur = 0;
            } else if (w.size() >= 2 && w[0] == "parameter") {
                // parameter [__D0] Name [(__A0, __D1)]
                RegParams rp;
                rp.present = true;
                size_t i = 1;
                if (i + 1 < w.size() && regFromName(w[i]) >= 0 && w[i + 1] != "(") rp.retReg = regFromName(w[i++]);
                else if (i + 1 == w.size()) {}
                if (i < w.size()) {
                    std::string name = w[i++];
                    if (i < w.size() && w[i] == "(") {
                        for (i++; i < w.size() && w[i] != ")"; i++)
                            if (w[i] != ",") rp.argRegs.push_back(regFromName(w[i]));
                    }
                    regParamPragmas[name] = rp;
                }
            }
            // unlink the pragma token
            if (prev) prev->next = t->next;
            continue;
        }
        t->pack = cur;
        prev = t;
    }
}

// ------------------------------------------------------------ declarations
static Obj *findGlobal(const std::string &name) {
    auto it = scopes[0]->vars.find(name);
    if (it == scopes[0]->vars.end() || !it->second.var) return nullptr;
    return it->second.var;
}

static void applyFuncAttrs(Obj *fn, Attrs &a) {
    if (a.noreturn) fn->noreturn = true;
    if (a.alwaysInline) fn->alwaysInline = true;
    if (a.noInline) fn->noInline = true;
    if (a.weak) fn->isWeak = true;
    if (a.gnuInline) fn->gnuInline = true;
    if (!a.section.empty()) fn->section = a.section;
    if (!a.unavailable.empty()) fn->unavailable = a.unavailable;
    if (!a.asmName.empty()) fn->asmName = a.asmName;
    if (a.isInline) fn->isInline = true;
}

static bool pendingGnuInline = false, pendingStatic = false;
static Obj *declareFunction(Token *name, Type *ty, Attrs &a, bool atFileScope) {
    Obj *fn = findGlobal(name->text);
    if (fn && !fn->isFunction) errorAt(name, "'%s' redeclared as a function", name->text.c_str());
    if (!fn) {
        fn = newGlobal(name->text, ty, name);
        fn->isFunction = true;
        scopes[0]->vars[name->text].var = fn;
    } else {
        // keep the most informative prototype
        if (fn->ty->oldStyle || !ty->oldStyle) {
            CallConv cc = fn->ty->cc;
            fn->ty = ty;
            if (cc == CC_PASCAL && ty->cc != CC_PASCAL) { fn->ty = copyType(ty); fn->ty->cc = CC_PASCAL; }
        }
    }
    pendingGnuInline = a.gnuInline;
    pendingStatic = a.isStatic;
    if (a.isStatic || (a.isInline && !fn->isDefinition)) fn->isStatic = fn->isStatic || a.isStatic || a.isInline;
    applyFuncAttrs(fn, a);
    auto rp = regParamPragmas.find(name->text);
    if (rp != regParamPragmas.end()) fn->regParams = rp->second;
    (void)atFileScope;
    return fn;
}

static void funcDefinition(Obj *fn, Type *ty, Token *name) {
    if (fn->isDefinition && fn->body && !fn->gnuInline) errorAt(name, "redefinition of '%s'", name->text.c_str());
    if (fn->gnuInline && fn->body && !pendingGnuInline) {
        // an `extern inline __attribute__((gnu_inline))` body followed by the real definition
        fn->gnuInline = false;
        fn->isStatic = pendingStatic;
        fn->isInline = false;
    }
    fn->isDefinition = true;
    fn->ty = ty;
    fn->tok = name;
    curFn = fn;
    fn->locals.clear();
    fn->params.clear();
    enterScope();
    for (size_t i = 0; i < ty->params.size(); i++) {
        Obj *p = newLocal(ty->paramNames[i], ty->params[i], name);
        p->isParam = true;
        fn->params.push_back(p);
    }
    // __func__
    {
        VarScope *vs = pushVar("__func__");
        vs->var = newStringLiteral(fn->name, name);
        pushVar("__FUNCTION__")->var = vs->var;
    }
    fn->body = compoundStmt();
    leaveScope();
    curFn = nullptr;
}

static void globalVariable(Token *name, Type *ty, Attrs &a) {
    Obj *var = findGlobal(name->text);
    if (var && var->isFunction) errorAt(name, "'%s' redeclared as a variable", name->text.c_str());
    if (!var) {
        var = newGlobal(name->text, ty, name);
        scopes[0]->vars[name->text].var = var;
        var->isExtern = true;
    } else if (var->ty->kind == TY_ARRAY && var->ty->arrayLen < 0 && ty->kind == TY_ARRAY) {
        var->ty = ty;
    } else if (ty->kind == TY_ARRAY && ty->arrayLen < 0) {
        // keep the complete type
    } else
        var->ty = ty;
    if (a.isStatic) var->isStatic = true;
    if (a.weak) var->isWeak = true;
    if (a.align) var->align = a.align;
    if (!a.section.empty()) var->section = a.section;
    if (!a.asmName.empty()) var->asmName = a.asmName;
    bool hasInit = equal("=");
    if (hasInit) {
        tok = tok->next;
        if (var->isDefinition && !var->isTentative) errorAt(name, "redefinition of '%s'", name->text.c_str());
        var->isDefinition = true;
        var->isTentative = false;
        var->isExtern = false;
        globalInitializer(var);
    } else if (!a.isExtern) {
        if (!var->isDefinition) {
            var->isDefinition = true;
            var->isTentative = true;
            var->isExtern = false;
            var->isBss = true;
        }
    }
    var->isConstData = var->ty->isConst || (var->ty->kind == TY_ARRAY && var->ty->base->isConst);
}

void localStaticDecl(Type *base, Attrs *attr) { (void)base; (void)attr; }

Node *declaration(Type *base, Attrs *attr) {
    Node *block = new Node(ND_BLOCK, tok);
    bool first = true;
    while (!consume(";")) {
        if (!first) expect(",");
        first = false;
        Attrs da = *attr;
        Token *name = nullptr;
        Type *ty = declarator(base, &name, &da);
        if (!name) errorAt(tok, "variable name omitted");
        if (da.isPascal) ty = applyPascal(ty);
        if (ty->kind == TY_FUNC) {
            declareFunction(name, ty, da, false);
            pushVar(name->text)->var = findGlobal(name->text);
            continue;
        }
        if (ty->kind == TY_VOID) errorAt(name, "variable declared void");
        if (da.isExtern) {
            Obj *g = findGlobal(name->text);
            if (!g) {
                g = newGlobal(name->text, ty, name);
                g->isExtern = true;
                scopes[0]->vars[name->text].var = g;
            }
            pushVar(name->text)->var = g;
            continue;
        }
        if (da.isStatic) {
            Obj *g = newGlobal(name->text, ty, name);
            g->asmName = format("%s.%d", name->text.c_str(), newLabelId());
            g->isStatic = true;
            g->isDefinition = true;
            g->isBss = true;
            if (da.align) g->align = da.align;
            if (!da.section.empty()) g->section = da.section;
            pushVar(name->text)->var = g;
            if (consume("=")) globalInitializer(g);
            g->isConstData = g->ty->isConst || (g->ty->kind == TY_ARRAY && g->ty->base->isConst);
            if (tsize(g->ty) == 0 && g->ty->kind == TY_ARRAY && g->ty->arrayLen < 0) errorAt(name, "array size missing");
            continue;
        }
        Obj *var = newLocal(name->text, ty, name);
        if (da.align) var->align = da.align;
        if (!da.asmName.empty()) {
            int physRegFromName(const std::string &);
            var->regAsm = physRegFromName(da.asmName);
            if (var->regAsm < 0) errorAt(name, "unknown register '%s'", da.asmName.c_str());
        }
        if (consume("=")) {
            Node *init = localInitializer(var, name);
            if (init) {
                Node *es = new Node(ND_EXPR_STMT, name);
                es->lhs = init;
                block->body.push_back(es);
            }
        }
        if (var->ty->kind == TY_ARRAY && var->ty->arrayLen < 0) errorAt(name, "array size missing in '%s'", name->text.c_str());
        if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && !sdef(var->ty)->complete)
            errorAt(name, "variable '%s' has incomplete type", name->text.c_str());
    }
    return block;
}

// f(...) = { 0xA9xx, ... } ;  Mac trap glue
static void parseInlineWords(Obj *fn) {
    expect("{");
    fn->inlineWords.clear();
    while (!consume("}")) {
        fn->inlineWords.push_back((int)(constExpr() & 0xFFFF));
        if (!consume(",")) { expect("}"); break; }
    }
    fn->isDefinition = false;
}

Program *parse(Token *first) {
    prog = new Program;
    scopes.clear();
    enterScope();
    handlePragmas(first);
    // drop a leading pragma token if handlePragmas could not unlink it
    while (first->kind == Token::OTHER && first->text == "#pragma") first = first->next;
    convertTokens(first);
    tok = first;
    while (tok->kind != Token::END) {
        if (consume(";")) continue;
        if (equal("_Static_assert") || (opt.cxx && equal("static_assert"))) {
            Token *t = tok;
            tok = tok->next;
            expect("(");
            int64_t v = constExpr();
            std::string msg = "static assertion failed";
            if (consume(",")) msg = parseStringLit();
            expect(")");
            expect(";");
            if (!v) errorAt(t, "%s", msg.c_str());
            continue;
        }
        if ((equal("__asm__") || equal("asm")) && tok->next->is("(")) {
            tok = tok->next->next;
            prog->topAsm.push_back(parseStringLit());
            expect(")");
            expect(";");
            continue;
        }
        if (opt.cxx && equal("extern") && tok->next->kind == Token::STR) {
            tok = tok->next->next;
            if (consume("{")) { /* contents parsed as ordinary declarations; closing brace skipped below */ }
            continue;
        }
        // namespace N { ... }  /  namespace { ... }  — treated as transparent (members go to global scope)
        if (opt.cxx && equal("namespace")) {
            tok = tok->next;
            while (isIdentTok(tok) || equal("::")) tok = tok->next;
            if (consume("=")) { while (!consume(";")) tok = tok->next; continue; } // namespace alias
            expect("{");
            continue; // the body's declarations are parsed by the outer loop; the closing } is skipped above
        }
        // using namespace N;  /  using X = T;  /  using N::x;   — accepted, mostly ignored
        if (opt.cxx && equal("using")) {
            tok = tok->next;
            if (consume("namespace")) { while (!consume(";")) tok = tok->next; continue; }
            if (isIdentTok(tok) && tok->next->is("=")) {
                Token *name = tok;
                tok = tok->next->next;
                Type *ty = typeName();
                pushVar(name->text)->typeDef = ty;
                expect(";");
                continue;
            }
            while (!consume(";")) tok = tok->next;
            continue;
        }
        if (opt.cxx && equal("}")) { tok = tok->next; continue; }
        Attrs a;
        Type *base = declspec(&a);
        if (consume(";")) continue;
        if (a.isTypedef) {
            parseTypedef(base, &a);
            continue;
        }
        bool first1 = true;
        bool wasDefinition = false;
        for (;;) {
            Attrs da = a;
            Token *name = nullptr;
            Type *ty = declarator(base, &name, &da);
            if (!name) errorAt(tok, "declaration needs a name");
            if (da.isPascal) ty = applyPascal(ty);
            if (ty->kind == TY_FUNC) {
                Obj *fn = declareFunction(name, ty, da, true);
                if (first1 && equal("{")) {
                    funcDefinition(fn, ty, name);
                    wasDefinition = true;
                    break;
                }
                if (consume("=")) {
                    if (equal("{")) parseInlineWords(fn);
                    else if (opt.cxx) { tok = tok->next; } // = 0 / default / delete
                }
            } else {
                if (ty->kind == TY_VOID) errorAt(name, "variable declared void");
                globalVariable(name, ty, da);
            }
            first1 = false;
            if (!consume(",")) break;
        }
        if (!wasDefinition) expect(";");
    }
    // complete tentative arrays, drop pure declarations later in the driver
    return prog;
}

} // namespace p68
