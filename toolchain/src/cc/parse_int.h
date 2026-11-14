// parse_int.h - internals shared by parse.cpp (declarations) and expr.cpp (expressions/statements).
#pragma once
#include "cc.h"

namespace p68 {

struct Attrs {
    bool isTypedef = false, isStatic = false, isExtern = false, isInline = false, isRegister = false;
    bool isPascal = false, noreturn = false, alwaysInline = false, noInline = false, weak = false;
    bool gnuInline = false, packed = false, used = false, isConstexpr = false;
    int align = 0;
    std::string section, unavailable, asmName;
};

struct VarScope {
    Obj *var = nullptr;
    Type *typeDef = nullptr;
    bool isEnum = false;
    int64_t enumVal = 0;
    Type *enumTy = nullptr;
};

extern Token *tok;
extern Obj *curFn;
extern Program *prog;

bool equal(const char *s);
bool consume(const char *s);
void expect(const char *s);
bool isIdentTok(const Token *t);

void enterScope();
void leaveScope();
VarScope *findVar(const std::string &name);
VarScope *pushVar(const std::string &name);
Type *findTag(const std::string &name);

Obj *newLocal(const std::string &name, Type *ty, Token *t);
Obj *newGlobal(const std::string &name, Type *ty, Token *t);
Obj *newAnonGlobal(Type *ty, Token *t);
Obj *newStringLiteral(const std::string &bytes, Token *t);
std::string uniqueName(const char *prefix);

bool isTypeName(Token *t);
Type *typeName();
Type *declspec(Attrs *attr);
Type *declarator(Type *ty, Token **nameTok, Attrs *attr);
void parseAttributes(Attrs *attr);
Node *declaration(Type *base, Attrs *attr);   // local declaration; returns initialisation code
void localStaticDecl(Type *base, Attrs *attr);
void parseTypedef(Type *base, Attrs *attr);
Node *localInitializer(Obj *var, Token *t);
void fileScopeInitializer(Obj *var);

Node *expr();
Node *assign();
Node *conditional();
int64_t constExpr();

Node *newNum(int64_t v, Token *t, Type *ty = nullptr);
Node *newFNum(double v, Token *t, Type *ty);
Node *newCast(Node *n, Type *ty);
Node *newBinary(NodeKind k, Node *l, Node *r, Token *t);
Node *newAssign(Node *lhs, Node *rhs, Token *t);
Node *newVarNode(Obj *v, Token *t);
Node *newDeref(Node *n, Token *t);
Node *newAddr(Node *n, Token *t);
Node *newMember(Node *n, Member *m, Token *t);
Node *newAddSub(NodeKind k, Node *l, Node *r, Token *t);
Node *rval(Node *n);                          // array/function decay
Node *stmt();
Node *compoundStmt();
int newLabelId();

int tsize(const Type *t);
int talign(const Type *t);
Type *sdef(Type *t);
const Type *sdef(const Type *t);
int64_t truncVal(int64_t v, const Type *ty);
bool evalConst(Node *n, int64_t *val, std::string *sym);  // integer / address constant
bool evalFloat(Node *n, double *val);
Member *findMember(Type *ty, const std::string &name, int *extraOffset);

} // namespace p68
