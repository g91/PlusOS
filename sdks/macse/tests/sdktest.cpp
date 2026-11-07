// sdktest.cpp - the Macintosh SE SDK headers must also work from C++.
#include <serom/serom.h>
#include <plusos/plusos.h>

struct Box {
    Rect r;
    explicit Box(short t, short l, short b, short rt) : r{t, l, b, rt} {}
    virtual ~Box() {}
    virtual void draw() const { FrameRect(&r); }
};

int sdk_cxx_test()
{
    Box *b = new Box(1, 2, 3, 4);
    b->draw();
    delete b;
    return serom_trap_in_rom(SEROM_TRAP_InitGraf);
}
