"""puirender.py - draws a PlusUI form exactly as the framework does on PlusOS.

A straight port of the drawing half of sdks/macplus/src/gfx/ui.c (and the PlusGFX
primitives it uses) onto a one-byte-per-pixel buffer, so the designer shows the form
pixel for pixel. The 8 x 8 system font is embedded below.
"""
import puimodel as M

# the PlusOS system font: ASCII 0x20..0x7E, 8 bytes per glyph (from font8x8.c)
FONT_HEX = "0000000000000000202020202000200050500000000000005050f850f85050002078a07028f02000c4c81020409818006090a040a89068002020400000000000102040404020100040201010102040000020a870a8200000002020f8202000000000000000301020000000f80000000000000000003030000808102040808000708898a8c88870002060202020207000708808102040f800f81020100888700010305090f8101000f880f00808887000304080f088887000f808102040404000708888708888700070888878081060000030300030300000003030003010200008102040201008000000f800f8000000402010081020400070880810200020007088b8a8b8807800708888f888888800f08888f08888f0007088808080887000e09088888890e000f88080f08080f800f88080f080808000708880b888887800888888f888888800702020202020700038101010109060008890a0c0a0908800808080808080f80088d8a8a8888888008888c8a8988888007088888888887000f08888f08080800070888888a8906800f08888f0a0908800788080700808f000f82020202020200088888888888870008888888888502000888888a8a8a8500088885020508888008888502020202000f80810204080f800704040404040700080804020100808007010101010107000205088000000000000000000000000ff402000000000000000007008788878008080b0c88888f0000000708080887000080868988888780000007088f8807000304840e04040400000007888887808708080b0c88888880020006020202070001000301010109060808090a0c0a0900060202020202070000000d0a8a8a888000000b0c88888880000007088888870000000f08888f0808000007888887808080000b0c880808000000078807008f0004040e040404830000000888888986800000088888850200000008888a8a85000000088502050880000008888887808700000f8102040f8001820204020201800202020202020200060101008101060000000649800000000"
FONT = bytes.fromhex(FONT_HEX)

LINE_H, SB_W, TAB_H = 10, 13, 14


class Surface:
    """1 = black. Rectangles are half-open like PlusGFX."""

    def __init__(self, w, h):
        self.w, self.h = w, h
        self.rows = [bytearray(w) for _ in range(h)]
        self.clip = (0, 0, w, h)

    def set_clip(self, l, t, r, b):
        self.clip = (max(l, 0), max(t, 0), min(r, self.w), min(b, self.h))

    def clip_in(self, l, t, r, b):
        old = self.clip
        self.clip = (max(l, old[0]), max(t, old[1]), max(min(r, old[2]), max(l, old[0])),
                     max(min(b, old[3]), max(t, old[1])))
        return old

    def _span(self, l, t, r, b):
        cl, ct, cr, cb = self.clip
        return max(l, cl), max(t, ct), min(r, cr), min(b, cb)

    def fill(self, l, t, r, b, black):
        l, t, r, b = self._span(l, t, r, b)
        if l >= r or t >= b:
            return
        line = (b"\x01" if black else b"\x00") * (r - l)
        for y in range(t, b):
            self.rows[y][l:r] = line

    def gray(self, l, t, r, b):
        l, t, r, b = self._span(l, t, r, b)
        for y in range(t, b):
            row = self.rows[y]
            for x in range(l, r):
                row[x] = 1 if ((x ^ y) & 1) == 0 else 0

    def pattern(self, l, t, r, b, even, odd):
        l, t, r, b = self._span(l, t, r, b)
        for y in range(t, b):
            pat = odd if y & 1 else even
            row = self.rows[y]
            for x in range(l, r):
                row[x] = (pat >> (7 - (x & 7))) & 1

    def dim(self, l, t, r, b):
        l, t, r, b = self._span(l, t, r, b)
        for y in range(t, b):
            row = self.rows[y]
            for x in range(l, r):
                if ((x ^ y) & 1) == 0:
                    row[x] = 0

    def invert(self, l, t, r, b):
        l, t, r, b = self._span(l, t, r, b)
        for y in range(t, b):
            row = self.rows[y]
            for x in range(l, r):
                row[x] ^= 1

    def pixel(self, x, y, black):
        cl, ct, cr, cb = self.clip
        if cl <= x < cr and ct <= y < cb:
            self.rows[y][x] = 1 if black else 0

    def hline(self, l, r, y, black):
        self.fill(l, y, r, y + 1, black)

    def vline(self, x, t, b, black):
        self.fill(x, t, x + 1, b, black)

    def frame(self, l, t, r, b):
        self.hline(l, r, t, 1)
        self.hline(l, r, b - 1, 1)
        self.vline(l, t, b, 1)
        self.vline(r - 1, t, b, 1)

    def line(self, x0, y0, x1, y1, black):
        dx, dy = abs(x1 - x0), abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            self.pixel(x0, y0, black)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def circle(self, cx, cy, r, black, fill=False):
        x, y, err = r, 0, 1 - r
        while x >= y:
            if fill:
                self.hline(cx - x, cx + x + 1, cy + y, black)
                self.hline(cx - x, cx + x + 1, cy - y, black)
                self.hline(cx - y, cx + y + 1, cy + x, black)
                self.hline(cx - y, cx + y + 1, cy - x, black)
            else:
                for px, py in ((x, y), (-x, y), (x, -y), (-x, -y), (y, x), (-y, x), (y, -x), (-y, -x)):
                    self.pixel(cx + px, cy + py, black)
            y += 1
            if err < 0:
                err += 2 * y + 1
            else:
                x -= 1
                err += 2 * (y - x) + 1

    def round_rect(self, l, t, r, b, radius, black, fill=False):
        if l >= r or t >= b:
            return
        radius = min(radius, (r - l) // 2, (b - t) // 2)
        if radius <= 0:
            if fill:
                self.fill(l, t, r, b, black)
            else:
                self.hline(l, r, t, black)
                self.hline(l, r, b - 1, black)
                self.vline(l, t, b, black)
                self.vline(r - 1, t, b, black)
            return
        cx0, cx1, cy0, cy1 = l + radius, r - 1 - radius, t + radius, b - 1 - radius
        if fill:
            self.fill(l, t + radius, r, b - radius, black)
        else:
            self.hline(l + radius, r - radius, t, black)
            self.hline(l + radius, r - radius, b - 1, black)
            self.vline(l, t + radius, b - radius, black)
            self.vline(r - 1, t + radius, b - radius, black)
        x, y, err = radius, 0, 1 - radius
        while x >= y:
            if fill:
                self.hline(cx0 - x, cx1 + x + 1, cy0 - y, black)
                self.hline(cx0 - y, cx1 + y + 1, cy0 - x, black)
                self.hline(cx0 - x, cx1 + x + 1, cy1 + y, black)
                self.hline(cx0 - y, cx1 + y + 1, cy1 + x, black)
            else:
                for px, py in ((cx0 - x, cy0 - y), (cx0 - y, cy0 - x), (cx1 + x, cy0 - y), (cx1 + y, cy0 - x),
                               (cx0 - x, cy1 + y), (cx0 - y, cy1 + x), (cx1 + x, cy1 + y), (cx1 + y, cy1 + x)):
                    self.pixel(px, py, black)
            y += 1
            if err < 0:
                err += 2 * y + 1
            else:
                x -= 1
                err += 2 * (y - x) + 1

    def char(self, x, y, ch, inverse=False):
        cl, ct, cr, cb = self.clip
        if x >= cr or y >= cb or x + 8 <= cl or y + 8 <= ct:
            return
        o = ord(ch) if isinstance(ch, str) else ch
        g = (o - 0x20) * 8 if 0x20 <= o < 0x7F else 0
        iv = 0xFF if inverse else 0
        for r in range(8):
            yy = y + r
            if not ct <= yy < cb:
                continue
            bits = FONT[g + r] ^ iv
            row = self.rows[yy]
            for c in range(8):
                xx = x + c
                if cl <= xx < cr:
                    row[xx] = (bits >> (7 - c)) & 1

    def text(self, x, y, s, inverse=False):
        for ch in s:
            self.char(x, y, ch, inverse)
            x += 8
        return x

    def blit(self, px, x, y, w, h):
        for r in range(min(h, len(px))):
            for c in range(min(w, len(px[r]))):
                self.pixel(x + c, y + r, px[r][c])

    def to_pgm(self):
        """binary PGM (white background) for tkinter.PhotoImage"""
        tr = bytes([255, 0]) + bytes(254)
        return b"P5 %d %d 255\n" % (self.w, self.h) + b"".join(bytes(r).translate(tr) for r in self.rows)


def _tri(s, cx, cy, d):
    for i in range(4):
        if d == 0:
            s.hline(cx - i, cx + i + 1, cy - 2 + i, 1)
        elif d == 1:
            s.hline(cx - i, cx + i + 1, cy + 1 - i, 1)
        elif d == 2:
            s.vline(cx - 2 + i, cy - i, cy + i + 1, 1)
        else:
            s.vline(cx + 1 - i, cy - i, cy + i + 1, 1)


def _scrollbar(s, l, t, r, b, vert, vmin, vmax, page, value):
    th = (r - l) if vert else (b - t)
    lo, hi = (t, b) if vert else (l, r)
    if hi - lo < 3 * th:
        th = (hi - lo) // 3
    a0, a1 = lo + th, hi - th
    track, rng = a1 - a0, vmax - vmin
    th0 = th1 = a0
    if rng > 0 and track > 0:
        ln = track * page // (rng + page) if page > 0 else 12
        ln = min(max(ln, 8), track)
        th0 = a0 + (track - ln) * (value - vmin) // rng
        th1 = th0 + ln
    s.fill(l, t, r, b, 0)
    if vert:
        cx = (l + r - 1) // 2
        if th1 > th0:
            s.gray(l + 1, a0, r - 1, a1)
            s.fill(l + 1, th0, r - 1, th1, 0)
            s.hline(l, r, th0, 1)
            s.hline(l, r, th1 - 1, 1)
        s.hline(l, r, a0 - 1, 1)
        s.hline(l, r, a1, 1)
        _tri(s, cx, (t + a0) // 2, 0)
        _tri(s, cx, (a1 + b) // 2, 1)
    else:
        cy = (t + b - 1) // 2
        if th1 > th0:
            s.gray(a0, t + 1, a1, b - 1)
            s.fill(th0, t + 1, th1, b - 1, 0)
            s.vline(th0, t, b, 1)
            s.vline(th1 - 1, t, b, 1)
        s.vline(a0 - 1, t, b, 1)
        s.vline(a1, t, b, 1)
        _tri(s, (l + a0) // 2, cy, 2)
        _tri(s, (a1 + r) // 2, cy, 3)
    s.frame(l, t, r, b)


def _clamp(v, lo, hi):
    return max(lo, min(v, hi))


def draw_widget(s, w, top):
    kind = w["kind"]
    g = lambda p: M.get(w, p)
    l, t = w["x"], w["y"] + top
    r, b = l + w["w"], t + w["h"]
    text = str(g("text")) if kind in ("label", "button", "checkbox", "radio", "textfield", "textarea", "group") else ""

    if kind == "label":
        lines = text.split("\n")
        y = max(t + ((b - t) - (len(lines) * LINE_H - 2)) // 2, t)
        for ln in lines:
            x = l
            if g("align") == "center":
                x = l + ((r - l) - len(ln) * 8) // 2
            elif g("align") == "right":
                x = r - len(ln) * 8
            s.text(x, y, ln)
            y += LINE_H
        if g("border"):
            s.frame(l, t, r, b)
    elif kind == "button":
        bl, bt, br, bb = l, t, r, b
        if g("default"):
            s.round_rect(l, t, r, b, 6, 1)
            s.round_rect(l + 1, t + 1, r - 1, b - 1, 5, 1)
            bl, bt, br, bb = l + 3, t + 3, r - 3, b - 3
        s.round_rect(bl, bt, br, bb, 4, 0, fill=True)
        s.round_rect(bl, bt, br, bb, 4, 1)
        s.text(bl + ((br - bl) - len(text) * 8) // 2, bt + ((bb - bt) - 8) // 2, text)
    elif kind == "checkbox":
        by = t + ((b - t) - 12) // 2
        s.fill(l, by, l + 12, by + 12, 0)
        s.frame(l, by, l + 12, by + 12)
        if g("checked"):
            s.line(l + 2, by + 2, l + 9, by + 9, 1)
            s.line(l + 9, by + 2, l + 2, by + 9, 1)
        s.text(l + 18, t + ((b - t) - 8) // 2, text)
    elif kind == "radio":
        cy = t + (b - t) // 2
        s.circle(l + 6, cy, 5, 0, fill=True)
        s.circle(l + 6, cy, 5, 1)
        if g("checked"):
            s.circle(l + 6, cy, 2, 1, fill=True)
        s.text(l + 18, t + ((b - t) - 8) // 2, text)
    elif kind == "textfield":
        s.fill(l, t, r, b, 0)
        s.frame(l, t, r, b)
        old = s.clip_in(l + 2, t + 1, r - 2, b - 1)
        s.text(l + 4, t + ((b - t) - 8) // 2, "*" * len(text) if g("password") else text)
        s.clip = old
    elif kind == "textarea":
        s.fill(l, t, r, b, 0)
        s.frame(l, t, r, b)
        old = s.clip_in(l + 2, t + 2, r - 2, b - 2)
        cols = max((w["w"] - 8) // 8, 1)
        ln = c = 0
        for ch in text:
            if ch == "\n":
                ln, c = ln + 1, 0
                continue
            s.char(l + 4 + c * 8, t + 3 + ln * LINE_H, ch)
            c += 1
            if c >= cols:
                ln, c = ln + 1, 0
        s.clip = old
    elif kind == "listbox":
        items = g("items")
        rows = max((w["h"] - 2) // LINE_H, 1)
        sbw = SB_W - 1 if len(items) > rows else 0
        sel = g("value")
        s.fill(l, t, r, b, 0)
        s.frame(l, t, r, b)
        old = s.clip_in(l + 1, t + 1, r - 1 - sbw, b - 1)
        for i in range(min(rows, len(items))):
            y = t + 1 + i * LINE_H
            if i == sel:
                s.fill(l + 1, y, r - 1 - sbw, y + LINE_H, 1)
            s.text(l + 4, y + 1, items[i], i == sel)
        s.clip = old
        if sbw:
            _scrollbar(s, r - SB_W, t, r, b, True, 0, max(len(items) - rows, 0), rows, 0)
    elif kind == "combo":
        items = g("items")
        r2, b2 = r - 1, b - 1
        cy = t + (b2 - t) // 2
        s.fill(l, t, r2, b2, 0)
        s.frame(l, t, r2, b2)
        s.hline(l + 2, r2 + 1, b2, 1)
        s.vline(r2, t + 2, b2 + 1, 1)
        s.vline(r2 - 15, t, b2, 1)
        _tri(s, r2 - 8, cy, 1)
        v = g("value")
        if 0 <= v < len(items):
            old = s.clip_in(l + 2, t + 1, r2 - 16, b2 - 1)
            s.text(l + 4, t + ((b2 - t) - 8) // 2, items[v])
            s.clip = old
    elif kind == "slider":
        rng = max(g("max") - g("min"), 1)
        v = _clamp(g("value"), g("min"), g("max")) - g("min")
        if w["h"] > w["w"]:
            cx, room = (l + r) // 2, (b - t) - 9
            ky = b - 9 - room * v // rng
            s.fill(cx - 1, t, cx + 2, b, 0)
            s.frame(cx - 1, t, cx + 2, b)
            s.fill(l, ky, r, ky + 9, 0)
            s.frame(l, ky, r, ky + 9)
            s.hline(l + 2, r - 2, ky + 4, 1)
        else:
            cy, room = (t + b) // 2, (r - l) - 9
            kx = l + room * v // rng
            s.fill(l, cy - 1, r, cy + 2, 0)
            s.frame(l, cy - 1, r, cy + 2)
            s.fill(kx, t, kx + 9, b, 0)
            s.frame(kx, t, kx + 9, b)
            s.vline(kx + 4, t + 2, b - 2, 1)
    elif kind == "scrollbar":
        _scrollbar(s, l, t, r, b, w["h"] > w["w"], g("min"), g("max"), g("page"),
                   _clamp(g("value"), g("min"), max(g("max"), g("min"))))
    elif kind == "progress":
        rng = max(g("max") - g("min"), 1)
        v = _clamp(g("value"), g("min"), g("max")) - g("min")
        s.fill(l, t, r, b, 0)
        s.frame(l, t, r, b)
        s.fill(l + 2, t + 2, l + 2 + (w["w"] - 4) * v // rng, b - 2, 1)
    elif kind == "group":
        s.frame(l, t + 4, r, b)
        if text:
            s.fill(l + 6, t, l + 10 + len(text) * 8, t + 8, 0)
            s.text(l + 8, t, text)
    elif kind == "image":
        img = g("image")
        if img:
            old = s.clip_in(l, t, r, b)
            s.blit(M.image_pixels(img), l, t, w["w"], w["h"])
            s.clip = old
        if g("border"):
            s.frame(l, t, r, b)
    elif kind == "separator":
        if w["h"] > w["w"]:
            s.vline(l + w["w"] // 2, t, b, 1)
        else:
            s.hline(l, r, t + w["h"] // 2, 1)
    elif kind == "tabs":
        items = g("items")
        tx = l + 4
        s.fill(l, t + TAB_H - 1, r, b, 0)
        s.frame(l, t + TAB_H - 1, r, b)
        for i, it in enumerate(items):
            tw = len(it) * 8 + 12
            if i == g("value"):
                s.fill(tx, t, tx + tw, t + TAB_H, 0)
                s.frame(tx, t, tx + tw, t + TAB_H)
                s.hline(tx + 1, tx + tw - 1, t + TAB_H - 1, 0)
                s.text(tx + 6, t + 3, it)
            else:
                s.frame(tx, t + 2, tx + tw, t + TAB_H)
                s.text(tx + 6, t + 4, it)
            tx += tw
    elif kind == "canvas":
        if g("border"):
            s.frame(l, t, r, b)
    if not g("enabled"):
        s.dim(l, t, r, b)


def tab_rects(w):
    """[(x0, x1)] of each tab header, relative to the widget"""
    out, tx = [], 4
    for it in M.get(w, "items"):
        tw = len(it) * 8 + 12
        out.append((tx, tx + tw))
        tx += tw
    return out


def widget_shown(form, w, design=True):
    """Is the widget on the currently selected page of its tabs (and, at run time, visible)?"""
    by_name = {x["name"]: x for x in form["widgets"]}
    for _ in range(8):
        if not design and not M.get(w, "visible"):
            return False
        t = w.get("tabs")
        if not t or t not in by_name:
            return True
        if M.get(by_name[t], "value") != M.get(w, "tab_page"):
            return False
        w = by_name[t]
    return True


def render_form(form, design=True):
    """Returns a Surface holding the whole window content (menu bar + client area)."""
    top = M.form_top(form)
    s = Surface(form["w"], form["h"] + top)
    if top:
        s.hline(0, s.w, top - 1, 1)
        x = 4
        for m in form["menus"]:
            s.text(x + 8, 2, m["title"])
            x += len(m["title"]) * 8 + 16
    s.set_clip(0, top, s.w, s.h)
    for w in form["widgets"]:
        if widget_shown(form, w, design):
            draw_widget(s, w, top)
            if design and not M.get(w, "visible"):
                s.gray(w["x"], w["y"] + top, w["x"] + w["w"], w["y"] + top + 1)     # hidden marker
    s.set_clip(0, 0, s.w, s.h)
    return s
