struct P { int x; int y; };
int f(){ P p; p.x=3; p.y=4; return p.x+p.y; }
extern "C" int g(){ return 5; }
enum class E { A, B };
int eh(){ return (int)E::B; }
inline int h(int a){ return a*2; }
namespace ns { int gg = 42; int get(){ return gg; } }
int un(){ return ns::get(); }
