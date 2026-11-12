struct Base { int x; Base(int v): x(v) {} virtual int f() const { return x; } virtual ~Base() {} };
struct Derived : Base { int y; Derived(int a,int b): Base(a), y(b) {} int f() const override { return x + y; } };
template<class T> T maxv(T a, T b){ return a>b?a:b; }
int use(Base* b){ return b->f(); }
int tmpl(){ return maxv<int>(3,7) + maxv<long>(10,2); }
int virt(){ Derived d(5,9); Base b(100); Base* p = &d; return use(p) + use(&b); }
namespace ns { int g = 42; int get(){ return g; } }
int refs(){ int a=1; int& r=a; r=99; return a; }
