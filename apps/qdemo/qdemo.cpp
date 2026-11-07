// qdemo.cpp - a PlusOS program in C++: classes, virtual functions, new/delete
// and QuickDraw through the ROM, drawing in the kernel's QuickDraw port.
// Shapes bounce around (drawn with XOR) until a key or mouse click.
#include <plusos/plusos.h>
#include <plusrom/QuickDraw.h>
#include <plusrom/ToolboxEvent.h>

class Shape {
public:
    Shape(short x, short y, short dx, short dy) : x_(x), y_(y), dx_(dx), dy_(dy) {}
    virtual ~Shape() {}
    virtual void draw() const = 0;           // XOR draw: drawing twice erases

    void step(const Rect &bounds)
    {
        draw();                               // erase at the old position
        x_ += dx_;
        y_ += dy_;
        if (x_ < bounds.left || x_ + size() > bounds.right)  dx_ = -dx_;
        if (y_ < bounds.top  || y_ + size() > bounds.bottom) dy_ = -dy_;
        draw();                               // draw at the new position
    }

protected:
    virtual short size() const = 0;
    Rect box() const { Rect r; SetRect(&r, x_, y_, x_ + size(), y_ + size()); return r; }

private:
    short x_, y_, dx_, dy_;
};

class Circle : public Shape {
public:
    using Shape::Shape;
    void draw() const override { Rect r = box(); InvertOval(&r); }
protected:
    short size() const override { return 40; }
};

class Square : public Shape {
public:
    using Shape::Shape;
    void draw() const override { Rect r = box(); InvertRect(&r); }
protected:
    short size() const override { return 32; }
};

int main(int, char **)
{
    if (!plusos_has(PLUSOS_HAVE_QD)) {
        printf("qdemo needs QuickDraw\n");
        return 1;
    }
    GrafPtr port = static_cast<QDGlobals *>(plusos->qd)->thePort;
    Rect bounds = port->portRect;
    EraseRect(&bounds);

    const int N = 12;
    Shape *shapes[N];
    for (int i = 0; i < N; i++) {
        short x = 20 + (Random() & 0x7FFF) % 420;
        short y = 20 + (Random() & 0x7FFF) % 260;
        short dx = (i % 3) + 1, dy = ((i + 1) % 3) + 1;
        if (i & 1) shapes[i] = new Circle(x, y, dx, dy);
        else       shapes[i] = new Square(x, y, dx, -dy);
        shapes[i]->draw();
    }

    while (!plusos->key_poll() && !Button())
        for (Shape *s : shapes)
            s->step(bounds);
    while (Button()) {}

    for (Shape *s : shapes) delete s;
    plusos->con_clear();
    printf("qdemo: %d C++ shapes animated with QuickDraw and deleted.\n", N);
    return 0;
}
