/* gfx.c - QuickDraw demos: GUI, MOUSE, PAINT. All drawing goes through the
 * ROM's QuickDraw; the text console takes the screen back afterwards. */
#include "kernel.h"

static int need_qd(void)
{
    if (g_flags & PLUSOS_HAVE_QD) return 1;
    printf("QuickDraw is not initialised.\n");
    return 0;
}

static int key_or_click(void)
{
    int k = kbd_poll();
    if (k) return k;
    if (Button()) { while (Button()) {} return 1; }
    return 0;
}

static void back_to_text(void)
{
    HideCursor();
    con_clear();
}

void gfx_demo(void)
{
    if (!need_qd()) return;
    Rect r;
    EraseRect(&g_port.portRect);
    FillRect(&g_port.portRect, &qd.gray);

    SetRect(&r, 40, 30, 472, 312);                 /* a "window" */
    EraseRect(&r);
    FrameRect(&r);
    SetRect(&r, 40, 30, 472, 48);
    FrameRect(&r);
    for (int y = 34; y <= 44; y += 2) { MoveTo(44, y); LineTo(468, y); }

    SetRect(&r, 70, 70, 190, 170);  PaintOval(&r);
    SetRect(&r, 210, 70, 330, 170); FrameRoundRect(&r, 24, 24);
    SetRect(&r, 225, 85, 315, 155); FillRect(&r, &qd.ltGray);
    SetRect(&r, 350, 70, 450, 170); PaintArc(&r, 0, 270);
    PenSize(3, 3);
    MoveTo(70, 200); LineTo(450, 290);
    MoveTo(70, 290); LineTo(450, 200);
    PenNormal();
    SetRect(&r, 160, 215, 360, 275); FrameOval(&r);
    InvertRect(&r);

    InitCursor();                                  /* arrow cursor, visible */
    while (!key_or_click()) {}
    back_to_text();
}

void gfx_mouse(void)
{
    if (!need_qd()) return;
    printf("Mouse position (QuickDraw GetMouse). Any key to stop.\n");
    InitCursor();
    int c, r;
    con_getpos(&c, &r);
    while (!kbd_poll()) {
        Point p;
        GetMouse(&p);
        char line[48];
        int n = snprintf(line, sizeof line, "x=%3d  y=%3d  button=%s   ", p.h, p.v, Button() ? "DOWN" : "up");
        HideCursor();                              /* don't let the arrow smear the text */
        con_goto(0, r);
        con_write(line, n);
        ShowCursor();
    }
    HideCursor();
    con_goto(0, r + 1);
}

void gfx_paint(void)
{
    if (!need_qd()) return;
    EraseRect(&g_port.portRect);
    MoveTo(4, 12);
    InitCursor();
    Point last = { -1, -1 };
    PenSize(2, 2);
    while (!kbd_poll()) {
        Point p;
        GetMouse(&p);
        if (Button()) {
            if (last.h < 0) MoveTo(p.h, p.v);
            LineTo(p.h, p.v);
            last = p;
        } else {
            last.h = -1;
        }
    }
    PenNormal();
    back_to_text();
}
