#!/usr/bin/env python3
"""designer.py - PlusUI Designer: draw PlusOS forms with the mouse.

    python tools/uidesigner/designer.py [project.pui]

Pure standard library (tkinter), so it runs unchanged on Windows, macOS and Linux.

  * Drag a widget from the palette onto the form (or click it, then click the form).
  * Click to select, Shift-click or drag a box to select several; drag to move; drag
    the handles to resize; arrow keys nudge. Double-click a widget to edit its text,
    items or picture.
  * The panel on the right edits the properties of the selection, or of the form
    when nothing is selected.
  * File > Generate Code writes the C (or C++) sources with puigen.py.

The canvas is drawn by puirender.py, a port of the framework's own drawing code, so
what you see is the form pixel for pixel as PlusOS shows it.
"""
import copy
import json
import os
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import puigen  # noqa: E402
import puimodel as M  # noqa: E402
import puirender as R  # noqa: E402

OX, OY = 28, 44                 # where the form's window content starts on the canvas
HANDLES = ("nw", "n", "ne", "e", "se", "s", "sw", "w")
CONTAINERS = ("group", "tabs")
IS_MAC = sys.platform == "darwin"
MODKEY = "Command" if IS_MAC else "Control"


# =============================================================================
# small modal dialogs
# =============================================================================
class Dialog(tk.Toplevel):
    def modal(self):
        """Show the dialog and wait for it to close."""
        self.update_idletasks()
        try:
            self.wait_visibility()
            self.grab_set()
        except tk.TclError:
            pass
        self.wait_window()


class TextDialog(Dialog):
    """Edit a block of text; result is None when cancelled."""

    def __init__(self, parent, title, text, hint="", height=10):
        super().__init__(parent)
        self.title(title)
        self.transient(parent)
        self.result = None
        if hint:
            ttk.Label(self, text=hint, justify="left", wraplength=420).pack(anchor="w", padx=10, pady=(10, 0))
        self.txt = tk.Text(self, width=52, height=height, font=("Courier", 10), undo=True)
        self.txt.pack(fill="both", expand=True, padx=10, pady=8)
        self.txt.insert("1.0", text)
        row = ttk.Frame(self)
        row.pack(fill="x", padx=10, pady=(0, 10))
        ttk.Button(row, text="OK", command=self.ok).pack(side="right")
        ttk.Button(row, text="Cancel", command=self.destroy).pack(side="right", padx=6)
        self.bind("<Escape>", lambda e: self.destroy())
        self.bind("<%s-Return>" % MODKEY, lambda e: self.ok())
        self.txt.focus_set()
        self.modal()

    def ok(self):
        self.result = self.txt.get("1.0", "end-1c")
        self.destroy()


class ImageDialog(Dialog):
    """A 1-bit picture editor: paint with the mouse. result is an image dict or None."""

    def __init__(self, parent, img):
        super().__init__(parent)
        self.title("Picture")
        self.transient(parent)
        self.result = None
        img = img or M.image_new(32, 32)
        self.px = M.image_pixels(img)
        self.w, self.h = img["w"], img["h"]
        self.paint = 1
        bar = ttk.Frame(self)
        bar.pack(fill="x", padx=10, pady=(10, 4))
        ttk.Label(bar, text="Width").pack(side="left")
        self.vw = tk.StringVar(value=str(self.w))
        ttk.Entry(bar, textvariable=self.vw, width=5).pack(side="left", padx=(2, 8))
        ttk.Label(bar, text="Height").pack(side="left")
        self.vh = tk.StringVar(value=str(self.h))
        ttk.Entry(bar, textvariable=self.vh, width=5).pack(side="left", padx=(2, 8))
        ttk.Button(bar, text="Resize", command=self.resize).pack(side="left")
        ttk.Button(bar, text="Clear", command=self.clear).pack(side="left", padx=4)
        ttk.Button(bar, text="Invert", command=self.invert).pack(side="left")
        ttk.Button(bar, text="Import...", command=self.load).pack(side="left", padx=4)
        self.cv = tk.Canvas(self, bg="#808080", highlightthickness=0)
        self.cv.pack(padx=10, pady=6)
        self.cv.bind("<Button-1>", self.down)
        self.cv.bind("<B1-Motion>", self.drag)
        ttk.Label(self, text="Click or drag to draw. The first pixel you touch decides black or white.").pack(padx=10)
        row = ttk.Frame(self)
        row.pack(fill="x", padx=10, pady=10)
        ttk.Button(row, text="OK", command=self.ok).pack(side="right")
        ttk.Button(row, text="Cancel", command=self.destroy).pack(side="right", padx=6)
        self.bind("<Escape>", lambda e: self.destroy())
        self.redraw()
        self.modal()

    def cell(self):
        return max(3, min(12, 420 // max(self.w, self.h, 1)))

    def redraw(self):
        c = self.cell()
        self.cv.config(width=self.w * c + 1, height=self.h * c + 1)
        self.cv.delete("all")
        for y in range(self.h):
            for x in range(self.w):
                self.cv.create_rectangle(x * c, y * c, x * c + c, y * c + c, outline="#c0c0c0",
                                         fill="black" if self.px[y][x] else "white", tags="p%d_%d" % (x, y))

    def at(self, e):
        c = self.cell()
        x, y = e.x // c, e.y // c
        return (x, y) if 0 <= x < self.w and 0 <= y < self.h else None

    def set(self, x, y, v):
        if self.px[y][x] != v:
            self.px[y][x] = v
            self.cv.itemconfig("p%d_%d" % (x, y), fill="black" if v else "white")

    def down(self, e):
        p = self.at(e)
        if p:
            self.paint = 0 if self.px[p[1]][p[0]] else 1
            self.set(p[0], p[1], self.paint)

    def drag(self, e):
        p = self.at(e)
        if p:
            self.set(p[0], p[1], self.paint)

    def resize(self):
        try:
            w, h = max(1, min(256, int(self.vw.get()))), max(1, min(256, int(self.vh.get())))
        except ValueError:
            return
        self.px = [[self.px[y][x] if y < self.h and x < self.w else 0 for x in range(w)] for y in range(h)]
        self.w, self.h = w, h
        self.redraw()

    def clear(self):
        self.px = [[0] * self.w for _ in range(self.h)]
        self.redraw()

    def invert(self):
        self.px = [[1 - v for v in row] for row in self.px]
        self.redraw()

    def load(self):
        path = filedialog.askopenfilename(parent=self, title="Import a picture",
                                          filetypes=[("Images", "*.png *.gif *.ppm *.pgm"), ("All files", "*.*")])
        if not path:
            return
        try:
            ph = tk.PhotoImage(file=path)
        except tk.TclError as e:
            messagebox.showerror("Import", "Could not read that picture.\n%s" % e, parent=self)
            return
        w, h = min(ph.width(), 256), min(ph.height(), 256)
        px = []
        for y in range(h):
            row = []
            for x in range(w):
                v = ph.get(x, y)
                if isinstance(v, str):
                    v = [int(p) for p in v.split()]
                transparent = False
                try:
                    transparent = ph.transparency_get(x, y)
                except (tk.TclError, AttributeError):
                    pass
                lum = (v[0] * 30 + v[1] * 59 + v[2] * 11) // 100
                row.append(0 if transparent else (1 if lum < 128 else 0))
            px.append(row)
        self.px, self.w, self.h = px, w, h
        self.vw.set(str(w))
        self.vh.set(str(h))
        self.redraw()

    def ok(self):
        self.result = M.image_from_pixels(self.px)
        self.destroy()


class GenerateDialog(Dialog):
    def __init__(self, parent, out_dir, lang):
        super().__init__(parent)
        self.title("Generate Code")
        self.transient(parent)
        self.result = None
        self.vdir = tk.StringVar(value=out_dir)
        self.vlang = tk.StringVar(value=lang)
        f = ttk.Frame(self, padding=12)
        f.pack(fill="both", expand=True)
        ttk.Label(f, text="Output folder").grid(row=0, column=0, sticky="w")
        ttk.Entry(f, textvariable=self.vdir, width=56).grid(row=1, column=0, sticky="we")
        ttk.Button(f, text="Browse...", command=self.browse).grid(row=1, column=1, padx=(6, 0))
        ttk.Label(f, text="Language").grid(row=2, column=0, sticky="w", pady=(10, 0))
        ttk.Radiobutton(f, text="C  -  builds with the PlusOS toolchain (make.cmd apps)", value="c",
                        variable=self.vlang).grid(row=3, column=0, sticky="w")
        ttk.Radiobutton(f, text="C++  -  one class per form; needs the gcc toolchain", value="cpp",
                        variable=self.vlang).grid(row=4, column=0, sticky="w")
        ttk.Label(f, wraplength=440, justify="left", foreground="#555",
                  text="Tip: a folder inside apps/<category>/ named after the app is picked up by the "
                       "build automatically. The *_ui files are rewritten each time; your own source "
                       "file is created once and then left alone.").grid(row=5, column=0, columnspan=2, sticky="w", pady=(10, 0))
        row = ttk.Frame(f)
        row.grid(row=6, column=0, columnspan=2, sticky="e", pady=(12, 0))
        ttk.Button(row, text="Generate", command=self.ok).pack(side="right")
        ttk.Button(row, text="Cancel", command=self.destroy).pack(side="right", padx=6)
        self.bind("<Escape>", lambda e: self.destroy())
        self.modal()

    def browse(self):
        d = filedialog.askdirectory(parent=self, initialdir=self.vdir.get() or None)
        if d:
            self.vdir.set(d)

    def ok(self):
        if self.vdir.get().strip():
            self.result = (self.vdir.get().strip(), self.vlang.get())
            self.destroy()


# =============================================================================
# the designer window
# =============================================================================
class Designer(tk.Tk):
    def __init__(self, path=None):
        super().__init__()
        self.geometry("1180x760")
        self.minsize(900, 560)
        self.project = M.new_project()
        self.path = None
        self.dirty = False
        self.fi = 0                 # current form index
        self.sel = []               # selected widget dicts
        self.zoom = 2
        self.grid_px = 8
        self.snap = tk.BooleanVar(value=True)
        self.show_grid = tk.BooleanVar(value=True)
        self.undo_stack, self.redo_stack = [], []
        self.armed = None           # palette kind waiting for a click on the form
        self.mode = None            # current canvas drag: move / resize / band / form
        self.clip = []              # copied widgets
        self.photo = None
        self.pvars = {}
        self.panel_key = None
        self.gen_dir, self.gen_lang = "", "c"
        self.build_menu()
        self.build_ui()
        if path:
            self.open_path(path)
        self.refresh(rebuild_panel=True)
        self.protocol("WM_DELETE_WINDOW", self.quit_app)

    # ---- convenience --------------------------------------------------------
    @property
    def form(self):
        return self.project["forms"][self.fi]

    def top(self):
        return M.form_top(self.form)

    def snapv(self, v):
        g = self.grid_px
        return int(round(v / g)) * g if self.snap.get() else int(round(v))

    def checkpoint(self):
        """Call before changing the project: makes the change undoable."""
        self.undo_stack.append(self.state())
        del self.undo_stack[:-100]
        self.redo_stack = []
        self.dirty = True

    def state(self):
        return json.dumps({"p": self.project, "fi": self.fi, "sel": [w["name"] for w in self.sel]})

    def restore(self, s):
        d = json.loads(s)
        self.project = d["p"]
        self.fi = min(d["fi"], len(self.project["forms"]) - 1)
        self.sel = [w for w in self.form["widgets"] if w["name"] in d["sel"]]
        self.dirty = True
        self.refresh(rebuild_panel=True)

    def undo(self, _e=None):
        if self.undo_stack and not self.entry_focused():
            self.redo_stack.append(self.state())
            self.restore(self.undo_stack.pop())

    def redo(self, _e=None):
        if self.redo_stack and not self.entry_focused():
            self.undo_stack.append(self.state())
            self.restore(self.redo_stack.pop())

    def entry_focused(self):
        """True while the user is typing in a property box (so shortcuts stay with the text)."""
        w = self.focus_get()
        return isinstance(w, (tk.Entry, ttk.Entry, tk.Text, ttk.Combobox))

    # ---- window layout ------------------------------------------------------
    def build_menu(self):
        mb = tk.Menu(self)
        acc = "Cmd+" if IS_MAC else "Ctrl+"

        def add(menu, label, cmd, key=None, shift=False):
            menu.add_command(label=label, command=cmd,
                             accelerator=(acc + ("Shift+" if shift else "") + key.upper()) if key else "")
            if key:
                seq = "<%s-%s>" % (MODKEY, key.upper() if shift else key)
                self.bind(seq, lambda e, c=cmd: (c(), "break")[1])

        m = tk.Menu(mb, tearoff=0)
        add(m, "New", self.new_project, "n")
        add(m, "Open...", self.open_dialog, "o")
        add(m, "Save", self.save, "s")
        add(m, "Save As...", self.save_as, "s", shift=True)
        m.add_separator()
        add(m, "Generate Code...", self.generate, "g")
        m.add_separator()
        m.add_command(label="Quit", command=self.quit_app)
        mb.add_cascade(label="File", menu=m)

        m = tk.Menu(mb, tearoff=0)
        add(m, "Undo", self.undo, "z")
        add(m, "Redo", self.redo, "y")
        m.add_separator()
        add(m, "Cut", self.cut, "x")
        add(m, "Copy", self.copy, "c")
        add(m, "Paste", self.paste, "v")
        add(m, "Duplicate", self.duplicate, "d")
        m.add_command(label="Delete", command=self.delete, accelerator="Del")
        add(m, "Select All", self.select_all, "a")
        m.add_separator()
        m.add_command(label="Bring to Front", command=lambda: self.restack(True))
        m.add_command(label="Send to Back", command=lambda: self.restack(False))
        mb.add_cascade(label="Edit", menu=m)

        m = tk.Menu(mb, tearoff=0)
        for label, how in (("Left Edges", "l"), ("Right Edges", "r"), ("Top Edges", "t"), ("Bottom Edges", "b"),
                           ("Same Width", "w"), ("Same Height", "h")):
            m.add_command(label=label, command=lambda h=how: self.align(h))
        mb.add_cascade(label="Align", menu=m)

        m = tk.Menu(mb, tearoff=0)
        m.add_command(label="Add Form", command=self.add_form)
        m.add_command(label="Delete Form", command=self.delete_form)
        m.add_separator()
        m.add_command(label="Menu Bar...", command=self.edit_menus)
        mb.add_cascade(label="Form", menu=m)

        m = tk.Menu(mb, tearoff=0)
        self.vzoom = tk.IntVar(value=self.zoom)
        for z in (1, 2, 3, 4):
            m.add_radiobutton(label="Zoom %dx" % z, value=z, variable=self.vzoom, command=self.set_zoom)
        m.add_separator()
        m.add_checkbutton(label="Show Grid", variable=self.show_grid, command=self.refresh)
        m.add_checkbutton(label="Snap to Grid (8 pixels)", variable=self.snap)
        mb.add_cascade(label="View", menu=m)
        self.config(menu=mb)

    def build_ui(self):
        self.columnconfigure(1, weight=1)
        self.rowconfigure(0, weight=1)

        left = ttk.Frame(self, padding=6)
        left.grid(row=0, column=0, sticky="ns")
        ttk.Label(left, text="WIDGETS", font=("TkDefaultFont", 8, "bold")).pack(anchor="w")
        ttk.Label(left, text="drag onto the form", foreground="#666").pack(anchor="w", pady=(0, 4))
        self.pal = {}
        for kind in M.KIND_ORDER:
            b = tk.Label(left, text=M.KINDS[kind][4], relief="raised", bd=1, padx=8, pady=3, anchor="w",
                         width=15, cursor="hand2", bg="#f4f4f4")
            b.pack(fill="x", pady=1)
            b.bind("<Button-1>", lambda e, k=kind: self.pal_press(e, k))
            b.bind("<B1-Motion>", self.pal_motion)
            b.bind("<ButtonRelease-1>", self.pal_release)
            self.pal[kind] = b
        ttk.Label(left, text="FORMS", font=("TkDefaultFont", 8, "bold")).pack(anchor="w", pady=(12, 2))
        self.formlist = tk.Listbox(left, height=6, exportselection=False, activestyle="none")
        self.formlist.pack(fill="x")
        self.formlist.bind("<<ListboxSelect>>", self.pick_form)
        row = ttk.Frame(left)
        row.pack(fill="x", pady=4)
        ttk.Button(row, text="Add", width=7, command=self.add_form).pack(side="left")
        ttk.Button(row, text="Delete", width=7, command=self.delete_form).pack(side="right")

        mid = ttk.Frame(self)
        mid.grid(row=0, column=1, sticky="nsew")
        mid.rowconfigure(0, weight=1)
        mid.columnconfigure(0, weight=1)
        self.cv = tk.Canvas(mid, bg="#9aa0a8", highlightthickness=0)
        self.cv.grid(row=0, column=0, sticky="nsew")
        sy = ttk.Scrollbar(mid, orient="vertical", command=self.cv.yview)
        sx = ttk.Scrollbar(mid, orient="horizontal", command=self.cv.xview)
        sy.grid(row=0, column=1, sticky="ns")
        sx.grid(row=1, column=0, sticky="we")
        self.cv.config(xscrollcommand=sx.set, yscrollcommand=sy.set)
        self.cv.bind("<Button-1>", self.cv_press)
        self.cv.bind("<B1-Motion>", self.cv_motion)
        self.cv.bind("<ButtonRelease-1>", self.cv_release)
        self.cv.bind("<Double-Button-1>", self.cv_double)
        self.cv.bind("<Motion>", self.cv_hover)

        right = ttk.Frame(self, padding=6, width=280)
        right.grid(row=0, column=2, sticky="ns")
        right.grid_propagate(False)
        right.pack_propagate(False)
        self.panel_title = ttk.Label(right, text="", font=("TkDefaultFont", 9, "bold"))
        self.panel_title.pack(anchor="w")
        self.panel = ttk.Frame(right)
        self.panel.pack(fill="both", expand=True, pady=(6, 0))

        self.status = ttk.Label(self, text="", anchor="w", padding=(8, 2))
        self.status.grid(row=1, column=0, columnspan=3, sticky="we")

        for key, fn in (("<Delete>", self.delete), ("<BackSpace>", self.delete), ("<Escape>", self.escape)):
            self.bind(key, lambda e, f=fn: None if self.entry_focused() else f())
        for key, dx, dy in (("Left", -1, 0), ("Right", 1, 0), ("Up", 0, -1), ("Down", 0, 1)):
            self.bind("<%s>" % key, lambda e, dx=dx, dy=dy: self.nudge(dx, dy))
            self.bind("<Shift-%s>" % key, lambda e, dx=dx, dy=dy: self.nudge(dx * 8, dy * 8))

    # ---- drawing ------------------------------------------------------------
    def refresh(self, rebuild_panel=False):
        f = self.form
        z = self.zoom
        top = self.top()
        self.sel = [w for w in self.sel if any(w is x for x in f["widgets"])]
        surf = R.render_form(f)
        if self.show_grid.get():
            g = self.grid_px
            for y in range(top, surf.h, g):
                row = surf.rows[y]
                for x in range(0, surf.w, g):
                    if not row[x]:
                        row[x] = 2
        tr = bytes([255, 0, 176]) + bytes(253)
        pgm = b"P5 %d %d 255\n" % (surf.w, surf.h) + b"".join(bytes(r).translate(tr) for r in surf.rows)
        img = tk.PhotoImage(data=pgm, format="ppm")
        self.photo = img.zoom(z) if z > 1 else img
        cv = self.cv
        cv.delete("all")
        W, H = surf.w * z, surf.h * z
        # a mock of the desktop's window frame
        cv.create_rectangle(OX - 9, OY - 23, OX + W + 12, OY + H + 12, fill="#404040", outline="")
        cv.create_rectangle(OX - 12, OY - 26, OX + W + 9, OY + H + 9, fill="white", outline="black")
        cv.create_rectangle(OX - 12, OY - 26, OX + W + 9, OY - 4, fill="#e8e8e8", outline="black")
        cv.create_rectangle(OX - 6, OY - 21, OX + 6, OY - 9, fill="white", outline="black")
        cv.create_text(OX + W // 2, OY - 15, text=f["title"], font=("TkDefaultFont", 9, "bold"))
        cv.create_image(OX, OY, image=self.photo, anchor="nw")
        cv.create_rectangle(OX - 1, OY - 1, OX + W, OY + H, outline="black")
        cv.config(scrollregion=(0, 0, OX + W + 60, OY + H + 60))
        self.draw_overlay()
        self.formlist.delete(0, "end")
        for i, fm in enumerate(self.project["forms"]):
            self.formlist.insert("end", ("%s  (main)" if i == 0 else "%s") % fm["name"])
        self.formlist.selection_set(self.fi)
        self.build_panel(rebuild_panel)
        name = os.path.basename(self.path) if self.path else "untitled.pui"
        self.title("%s%s - PlusUI Designer" % ("* " if self.dirty else "", name))
        self.set_status()

    def set_status(self, extra=""):
        f = self.form
        s = "%s: %d x %d" % (f["name"], f["w"], f["h"])
        if len(self.sel) == 1:
            w = self.sel[0]
            s += "     %s (%s)  x %d  y %d  w %d  h %d" % (w["name"], w["kind"], w["x"], w["y"], w["w"], w["h"])
        elif self.sel:
            s += "     %d widgets selected" % len(self.sel)
        if self.armed:
            s += "     click the form to place a %s (Esc cancels)" % M.KINDS[self.armed][4].lower()
        self.status.config(text=s + extra)

    def c2f(self, cx, cy):
        """canvas -> client coordinates"""
        return (cx - OX) / self.zoom, (cy - OY) / self.zoom - self.top()

    def f2c(self, x, y):
        return OX + x * self.zoom, OY + (y + self.top()) * self.zoom

    def handle_points(self, w):
        x0, y0 = self.f2c(w["x"], w["y"])
        x1, y1 = self.f2c(w["x"] + w["w"], w["y"] + w["h"])
        xm, ym = (x0 + x1) / 2, (y0 + y1) / 2
        return {"nw": (x0, y0), "n": (xm, y0), "ne": (x1, y0), "e": (x1, ym),
                "se": (x1, y1), "s": (xm, y1), "sw": (x0, y1), "w": (x0, ym)}

    def form_handles(self):
        f = self.form
        W, H = f["w"] * self.zoom, (f["h"] + self.top()) * self.zoom
        return {"fe": (OX + W + 4, OY + H / 2), "fs": (OX + W / 2, OY + H + 4), "fse": (OX + W + 4, OY + H + 4)}

    def draw_overlay(self):
        cv = self.cv
        cv.delete("ov")
        for w in self.sel:
            x0, y0 = self.f2c(w["x"], w["y"])
            x1, y1 = self.f2c(w["x"] + w["w"], w["y"] + w["h"])
            cv.create_rectangle(x0 - 1, y0 - 1, x1, y1, outline="#0a64d8", width=2, tags="ov")
        if len(self.sel) == 1:
            for x, y in self.handle_points(self.sel[0]).values():
                cv.create_rectangle(x - 4, y - 4, x + 3, y + 3, fill="white", outline="#0a64d8", tags="ov")
        for x, y in self.form_handles().values():
            cv.create_rectangle(x - 4, y - 4, x + 4, y + 4, fill="#303030", outline="white", tags="ov")

    # ---- hit testing --------------------------------------------------------
    def widget_at(self, x, y):
        """The smallest shown widget under a client point."""
        best = None
        for w in self.form["widgets"]:
            if not R.widget_shown(self.form, w):
                continue
            if w["x"] <= x < w["x"] + w["w"] and w["y"] <= y < w["y"] + w["h"]:
                if best is None or w["w"] * w["h"] <= best["w"] * best["h"]:
                    best = w
        return best

    def handle_at(self, cx, cy):
        if len(self.sel) == 1:
            for name, (x, y) in self.handle_points(self.sel[0]).items():
                if abs(cx - x) <= 5 and abs(cy - y) <= 5:
                    return name
        for name, (x, y) in self.form_handles().items():
            if abs(cx - x) <= 6 and abs(cy - y) <= 6:
                return name
        return None

    def children_of(self, cont):
        """Widgets lying wholly inside a group box or tabs widget (they move with it)."""
        out = []
        for w in self.form["widgets"]:
            if w is cont or any(w is s for s in self.sel):
                continue
            if (w["x"] >= cont["x"] and w["y"] >= cont["y"] and w["x"] + w["w"] <= cont["x"] + cont["w"]
                    and w["y"] + w["h"] <= cont["y"] + cont["h"]):
                if cont["kind"] != "tabs" or w.get("tabs") == cont["name"]:
                    out.append(w)
        return out

    # ---- canvas mouse -------------------------------------------------------
    def canvas_xy(self, e):
        return self.cv.canvasx(e.x), self.cv.canvasy(e.y)

    def cv_hover(self, e):
        cx, cy = self.canvas_xy(e)
        h = self.handle_at(cx, cy)
        cur = {"nw": "size_nw_se", "se": "size_nw_se", "ne": "size_ne_sw", "sw": "size_ne_sw",
               "n": "sb_v_double_arrow", "s": "sb_v_double_arrow", "e": "sb_h_double_arrow",
               "w": "sb_h_double_arrow", "fe": "sb_h_double_arrow", "fs": "sb_v_double_arrow",
               "fse": "size_nw_se"}.get(h, "crosshair" if self.armed else "")
        try:
            self.cv.config(cursor=cur)
        except tk.TclError:
            self.cv.config(cursor="")

    def cv_press(self, e):
        self.cv.focus_set()
        cx, cy = self.canvas_xy(e)
        fx, fy = self.c2f(cx, cy)
        shift = bool(e.state & 0x0001)
        self.mode = None
        if self.armed:
            kind, self.armed = self.armed, None
            self.place(kind, fx, fy)
            return
        h = self.handle_at(cx, cy)
        if h:
            self.mode = {"kind": "form" if h.startswith("f") else "resize", "h": h, "sx": fx, "sy": fy,
                         "before": self.state(), "changed": False,
                         "orig": copy.deepcopy(self.sel[0]) if self.sel and not h.startswith("f") else None,
                         "fw": self.form["w"], "fh": self.form["h"]}
            return
        w = self.widget_at(fx, fy)
        if w is None:
            if not shift:
                self.sel = []
            self.mode = {"kind": "band", "sx": cx, "sy": cy, "base": list(self.sel)}
            self.draw_overlay()
            self.build_panel()
            self.set_status()
            return
        before = self.state()
        if w["kind"] == "tabs" and fy < w["y"] + R.TAB_H:            # clicking a tab shows its page
            for i, (a, b) in enumerate(R.tab_rects(w)):
                if a <= fx - w["x"] < b and i != M.get(w, "value"):
                    self.checkpoint()
                    w["value"] = i
                    self.sel = [w]
                    self.refresh(rebuild_panel=True)
                    return
        if shift:
            if any(w is s for s in self.sel):
                self.sel = [s for s in self.sel if s is not w]
            else:
                self.sel.append(w)
        elif not any(w is s for s in self.sel):
            self.sel = [w]
        moving = list(self.sel)
        for s in self.sel:
            if s["kind"] in CONTAINERS:
                moving += [c for c in self.children_of(s) if not any(c is m for m in moving)]
        self.mode = {"kind": "move", "sx": fx, "sy": fy, "before": before, "changed": False,
                     "items": [(m, m["x"], m["y"]) for m in moving]}
        self.draw_overlay()
        self.build_panel()
        self.set_status()

    def cv_motion(self, e):
        m = self.mode
        if not m:
            return
        cx, cy = self.canvas_xy(e)
        fx, fy = self.c2f(cx, cy)
        if m["kind"] == "band":
            self.cv.delete("band")
            self.cv.create_rectangle(m["sx"], m["sy"], cx, cy, outline="#0a64d8", dash=(3, 3), tags="band")
            return
        if m["kind"] == "move":
            if not self.sel:
                return
            lead, lx, ly = m["items"][0]
            dx = self.snapv(lx + fx - m["sx"]) - lx
            dy = self.snapv(ly + fy - m["sy"]) - ly
            for w, ox, oy in m["items"]:
                w["x"], w["y"] = ox + dx, oy + dy
            m["changed"] = m["changed"] or dx != 0 or dy != 0
        elif m["kind"] == "resize":
            w, o, h = self.sel[0], m["orig"], m["h"]
            l, t, r, b = o["x"], o["y"], o["x"] + o["w"], o["y"] + o["h"]
            if "w" in h:
                l = min(self.snapv(fx), r - 4)
            if "e" in h:
                r = max(self.snapv(fx), l + 4)
            if "n" in h:
                t = min(self.snapv(fy), b - 4)
            if "s" in h:
                b = max(self.snapv(fy), t + 4)
            w["x"], w["y"], w["w"], w["h"] = l, t, r - l, b - t
            m["changed"] = True
        elif m["kind"] == "form":
            f = self.form
            if "e" in m["h"]:
                f["w"] = max(48, min(M.MAX_W, self.snapv(m["fw"] + fx - m["sx"])))
            if "s" in m["h"]:
                f["h"] = max(24, min(M.MAX_H - self.top(), self.snapv(m["fh"] + fy - m["sy"])))
            m["changed"] = True
            self.cv.delete("ghost")
            x1, y1 = self.f2c(f["w"], f["h"])
            self.cv.create_rectangle(OX, OY, x1, y1, outline="#0a64d8", dash=(4, 3), tags="ghost")
            self.set_status()
            return
        self.draw_overlay()
        self.sync_panel()
        self.set_status()

    def cv_release(self, e):
        m, self.mode = self.mode, None
        if not m:
            return
        if m["kind"] == "band":
            cx, cy = self.canvas_xy(e)
            self.cv.delete("band")
            x0, y0 = self.c2f(min(m["sx"], cx), min(m["sy"], cy))
            x1, y1 = self.c2f(max(m["sx"], cx), max(m["sy"], cy))
            if x1 - x0 > 2 or y1 - y0 > 2:
                picked = [w for w in self.form["widgets"] if R.widget_shown(self.form, w)
                          and w["x"] >= x0 and w["y"] >= y0 and w["x"] + w["w"] <= x1 and w["y"] + w["h"] <= y1]
                self.sel = m["base"] + [w for w in picked if not any(w is b for b in m["base"])]
            self.refresh(rebuild_panel=True)
            return
        if m.get("changed"):
            self.undo_stack.append(m["before"])
            self.redo_stack = []
            self.dirty = True
            if m["kind"] == "move":
                for w, _ox, _oy in m["items"]:
                    self.adopt(w)
        self.refresh(rebuild_panel=m["kind"] == "form")

    def cv_double(self, e):
        fx, fy = self.c2f(*self.canvas_xy(e))
        w = self.widget_at(fx, fy)
        self.mode = None
        if not w:
            return
        self.sel = [w]
        props = dict(M.KIND_PROPS[w["kind"]])
        for p in ("text", "items", "image"):
            if p in props:
                self.edit_prop(w, p, props[p])
                return

    # ---- palette drag and drop ----------------------------------------------
    def pal_press(self, e, kind):
        self.pal_drag = {"kind": kind, "x": e.x_root, "y": e.y_root, "moved": False}

    def pal_motion(self, e):
        d = getattr(self, "pal_drag", None)
        if not d:
            return
        if abs(e.x_root - d["x"]) + abs(e.y_root - d["y"]) > 6:
            d["moved"] = True
        self.cv.delete("ghost")
        if d["moved"] and self.over_canvas(e):
            fx, fy = self.root_to_form(e)
            _c, w, h, _t, _l = M.KINDS[d["kind"]]
            x0, y0 = self.f2c(self.snapv(fx), self.snapv(fy))
            self.cv.create_rectangle(x0, y0, x0 + w * self.zoom, y0 + h * self.zoom, outline="#0a64d8",
                                     dash=(3, 3), width=2, tags="ghost")
        try:
            e.widget.config(cursor="plus" if d["moved"] else "hand2")
        except tk.TclError:
            pass

    def pal_release(self, e):
        d, self.pal_drag = getattr(self, "pal_drag", None), None
        self.cv.delete("ghost")
        if not d:
            return
        e.widget.config(cursor="hand2")
        if d["moved"]:
            if self.over_canvas(e):
                fx, fy = self.root_to_form(e)
                self.place(d["kind"], fx, fy)
        else:                                   # a plain click arms the tool
            self.armed = None if self.armed == d["kind"] else d["kind"]
            for k, b in self.pal.items():
                b.config(relief="sunken" if k == self.armed else "raised", bg="#cfe0ff" if k == self.armed else "#f4f4f4")
            self.set_status()

    def over_canvas(self, e):
        return self.winfo_containing(e.x_root, e.y_root) is self.cv

    def root_to_form(self, e):
        cx = self.cv.canvasx(e.x_root - self.cv.winfo_rootx())
        cy = self.cv.canvasy(e.y_root - self.cv.winfo_rooty())
        return self.c2f(cx, cy)

    def place(self, kind, fx, fy):
        f = self.form
        self.checkpoint()
        w = M.new_widget(f, kind, 0, 0)
        w["x"] = max(0, min(self.snapv(fx), f["w"] - 8))
        w["y"] = max(0, min(self.snapv(fy), f["h"] - 8))
        if kind in CONTAINERS:
            f["widgets"].insert(0, w)           # containers sit behind what they hold
        else:
            f["widgets"].append(w)
        self.adopt(w)
        self.sel = [w]
        self.armed = None
        for b in self.pal.values():
            b.config(relief="raised", bg="#f4f4f4")
        self.refresh(rebuild_panel=True)

    def adopt(self, w):
        """Dropping a widget on a tabs page puts it on that page; dragging it off takes it out."""
        if w["kind"] == "tabs":
            return
        owner = None
        for t in self.form["widgets"]:
            if t["kind"] == "tabs" and t is not w and R.widget_shown(self.form, t):
                if (w["x"] >= t["x"] and w["y"] >= t["y"] + R.TAB_H - 1 and w["x"] + w["w"] <= t["x"] + t["w"]
                        and w["y"] + w["h"] <= t["y"] + t["h"]):
                    if owner is None or t["w"] * t["h"] < owner["w"] * owner["h"]:
                        owner = t
        if owner:
            w["tabs"], w["tab_page"] = owner["name"], M.get(owner, "value")
        elif w.get("tabs"):
            w["tabs"], w["tab_page"] = "", 0

    # ---- edit commands ------------------------------------------------------
    def escape(self):
        self.armed = None
        for b in self.pal.values():
            b.config(relief="raised", bg="#f4f4f4")
        self.sel = []
        self.refresh(rebuild_panel=True)

    def delete(self):
        if not self.sel:
            return
        self.checkpoint()
        gone = {w["name"] for w in self.sel if w["kind"] == "tabs"}
        self.form["widgets"] = [w for w in self.form["widgets"] if not any(w is s for s in self.sel)]
        for w in self.form["widgets"]:
            if w.get("tabs") in gone:
                w["tabs"], w["tab_page"] = "", 0
        self.sel = []
        self.refresh(rebuild_panel=True)

    def copy(self):
        if self.entry_focused():
            return
        self.clip = copy.deepcopy(self.sel)

    def cut(self):
        if self.entry_focused():
            return
        self.copy()
        self.delete()

    def paste(self, offset=16):
        if self.entry_focused() or not self.clip:
            return
        self.checkpoint()
        f = self.form
        new = []
        for src in self.clip:
            w = copy.deepcopy(src)
            w["name"] = M.unique_name(f, w["name"])
            w["x"] = min(w["x"] + offset, max(f["w"] - 8, 0))
            w["y"] = min(w["y"] + offset, max(f["h"] - 8, 0))
            if w.get("tabs") and not any(t["name"] == w["tabs"] for t in f["widgets"]):
                w["tabs"] = ""
            if w["kind"] in CONTAINERS:
                f["widgets"].insert(0, w)
            else:
                f["widgets"].append(w)
            new.append(w)
        self.clip = copy.deepcopy(new)
        self.sel = new
        self.refresh(rebuild_panel=True)

    def duplicate(self):
        if self.entry_focused() or not self.sel:
            return
        keep, self.clip = self.clip, copy.deepcopy(self.sel)
        self.paste()
        self.clip = keep

    def select_all(self):
        if self.entry_focused():
            return
        self.sel = [w for w in self.form["widgets"] if R.widget_shown(self.form, w)]
        self.refresh(rebuild_panel=True)

    def restack(self, front):
        if not self.sel:
            return
        self.checkpoint()
        rest = [w for w in self.form["widgets"] if not any(w is s for s in self.sel)]
        picked = [w for w in self.form["widgets"] if any(w is s for s in self.sel)]
        self.form["widgets"] = rest + picked if front else picked + rest
        self.refresh()

    def nudge(self, dx, dy):
        if self.entry_focused() or not self.sel:
            return
        self.checkpoint()
        for w in self.sel:
            w["x"] += dx
            w["y"] += dy
        self.refresh()
        self.sync_panel()

    def align(self, how):
        if len(self.sel) < 2:
            messagebox.showinfo("Align", "Select two or more widgets first (Shift-click, or drag a box around them).")
            return
        self.checkpoint()
        a = self.sel[0]
        for w in self.sel[1:]:
            if how == "l":
                w["x"] = a["x"]
            elif how == "r":
                w["x"] = a["x"] + a["w"] - w["w"]
            elif how == "t":
                w["y"] = a["y"]
            elif how == "b":
                w["y"] = a["y"] + a["h"] - w["h"]
            elif how == "w":
                w["w"] = a["w"]
            elif how == "h":
                w["h"] = a["h"]
        self.refresh()

    def set_zoom(self):
        self.zoom = self.vzoom.get()
        self.refresh()

    # ---- forms --------------------------------------------------------------
    def pick_form(self, _e=None):
        s = self.formlist.curselection()
        if s and s[0] != self.fi:
            self.fi = s[0]
            self.sel = []
            self.refresh(rebuild_panel=True)

    def add_form(self):
        self.checkpoint()
        self.project["forms"].append(M.new_form(self.project))
        self.fi = len(self.project["forms"]) - 1
        self.sel = []
        self.refresh(rebuild_panel=True)

    def delete_form(self):
        if len(self.project["forms"]) < 2:
            messagebox.showinfo("Delete Form", "A project needs at least one form.")
            return
        if not messagebox.askyesno("Delete Form", "Delete the form \"%s\" and everything on it?" % self.form["name"]):
            return
        self.checkpoint()
        del self.project["forms"][self.fi]
        self.fi = max(0, self.fi - 1)
        self.sel = []
        self.refresh(rebuild_panel=True)

    def edit_menus(self):
        d = TextDialog(self, "Menu Bar", M.menus_to_text(self.form.get("menus", [])), height=8,
                       hint="One menu per line:   Title: item, item, -, item\n"
                            "A lone - is a separator. End an item with ^X for the shortcut Command-X.\n"
                            "Example:   File: New^N, Open...^O, -, Quit^Q\n"
                            "Leave it empty for no menu bar.")
        if d.result is None:
            return
        self.checkpoint()
        self.form["menus"] = M.menus_from_text(d.result)
        self.refresh(rebuild_panel=True)

    # ---- the property panel -------------------------------------------------
    def build_panel(self, force=False):
        key = (self.fi, tuple(id(w) for w in self.sel), len(self.project["forms"]))
        if key == self.panel_key and not force:
            self.sync_panel()
            return
        self.panel_key = key
        for c in self.panel.winfo_children():
            c.destroy()
        self.pvars = {}
        p = self.panel
        p.columnconfigure(1, weight=1)
        self.prow = 0
        if len(self.sel) > 1:
            self.panel_title.config(text="%d widgets selected" % len(self.sel))
            ttk.Label(p, wraplength=250, justify="left",
                      text="Drag to move them together.\nUse the Align menu to line them up or make them "
                           "the same size; the first one you picked is the reference.").grid(sticky="w")
            return
        if len(self.sel) == 1:
            w = self.sel[0]
            self.panel_title.config(text="%s  -  %s" % (M.KINDS[w["kind"]][4], w["name"]))
            for prop, typ in M.props_for(w["kind"]):
                self.prop_row(w, prop, typ)
            evs = M.KIND_EVENTS.get(w["kind"], [])
            if evs:
                self.section("Handlers to generate")
                for ev in evs:
                    v = tk.BooleanVar(value=ev in w.get("events", []))
                    ttk.Checkbutton(p, text="%s_%s_%s()" % (self.form["name"], w["name"], ev), variable=v,
                                    command=lambda ev=ev, v=v, w=w: self.toggle_event(w, ev, v.get())
                                    ).grid(row=self.prow, column=0, columnspan=2, sticky="w")
                    self.prow += 1
            return
        f = self.form
        self.panel_title.config(text="Form  -  %s" % f["name"])
        self.section("Application")
        self.simple_row("app", "name", self.project["app"], lambda v: self.set_app(v))
        self.section("Form")
        for prop, typ in (("name", "str"), ("title", "str"), ("w", "int"), ("h", "int"),
                          ("pos", "choice:auto,center"), ("timer", "int")):
            self.prop_row(f, prop, typ, is_form=True)
        ttk.Label(p, text="menu bar").grid(row=self.prow, column=0, sticky="w", pady=1)
        n = len(f.get("menus", []))
        ttk.Button(p, text="Edit...  (%s)" % ("%d menu%s" % (n, "" if n == 1 else "s") if n else "none"),
                   command=self.edit_menus).grid(row=self.prow, column=1, sticky="we", pady=1)
        self.prow += 1
        self.section("Handlers to generate")
        for ev, label in (("close", "close box clicked"), ("key", "unused key pressed"),
                          ("timer", "timer (set timer ticks; 60 = 1 s)")):
            v = tk.BooleanVar(value=ev in f.get("events", []))
            ttk.Checkbutton(p, text="%s_%s()  -  %s" % (f["name"], ev, label), variable=v,
                            command=lambda ev=ev, v=v: self.toggle_event(f, ev, v.get())
                            ).grid(row=self.prow, column=0, columnspan=2, sticky="w")
            self.prow += 1
        ttk.Label(p, wraplength=250, justify="left", foreground="#555",
                  text="The first form in the list is the one the program opens at start-up."
                  ).grid(row=self.prow, column=0, columnspan=2, sticky="w", pady=(10, 0))

    def section(self, text):
        ttk.Label(self.panel, text=text.upper(), font=("TkDefaultFont", 8, "bold"), foreground="#444"
                  ).grid(row=self.prow, column=0, columnspan=2, sticky="w", pady=(10, 2))
        self.prow += 1

    def simple_row(self, key, label, value, setter):
        p = self.panel
        ttk.Label(p, text=label).grid(row=self.prow, column=0, sticky="w", pady=1)
        v = tk.StringVar(value=str(value))
        e = ttk.Entry(p, textvariable=v, width=18)
        e.grid(row=self.prow, column=1, sticky="we", pady=1)
        e.bind("<Return>", lambda ev: setter(v.get()))
        e.bind("<FocusOut>", lambda ev: setter(v.get()))
        self.pvars[key] = (v, lambda: value)
        self.prow += 1

    def prop_row(self, obj, prop, typ, is_form=False):
        p = self.panel
        label = {"w": "width", "h": "height", "tab_page": "tab page", "maxlen": "max length", "pos": "position",
                 "timer": "timer ticks", "tabs": "on tabs", "default": "default (Return)",
                 "cancel": "cancel (Esc)", "value": "selected" if obj.get("kind") in ("listbox", "combo", "tabs") else "value"
                 }.get(prop, prop)
        getv = (lambda: obj.get(prop)) if is_form else (lambda: M.get(obj, prop))
        ttk.Label(p, text=label).grid(row=self.prow, column=0, sticky="w", pady=1)
        commit = lambda val: self.set_prop(obj, prop, typ, val, is_form)
        if typ == "bool":
            v = tk.BooleanVar(value=bool(getv()))
            ttk.Checkbutton(p, variable=v, command=lambda: commit(v.get())).grid(row=self.prow, column=1, sticky="w")
            self.pvars[prop] = (v, getv)
        elif typ.startswith("choice:") or (prop == "tabs" and not is_form):
            if prop == "tabs":
                values = [""] + [t["name"] for t in self.form["widgets"] if t["kind"] == "tabs" and t is not obj]
            else:
                values = typ.split(":", 1)[1].split(",")
            v = tk.StringVar(value=str(getv() or ""))
            cb = ttk.Combobox(p, textvariable=v, values=values, state="readonly", width=16)
            cb.grid(row=self.prow, column=1, sticky="we", pady=1)
            cb.bind("<<ComboboxSelected>>", lambda e: commit(v.get()))
            self.pvars[prop] = (v, lambda: getv() or "")
        elif typ in ("text", "items", "image"):
            ttk.Button(p, text="Edit...", command=lambda: self.edit_prop(obj, prop, typ)
                       ).grid(row=self.prow, column=1, sticky="we", pady=1)
        else:
            v = tk.StringVar(value=str(getv()))
            e = ttk.Entry(p, textvariable=v, width=18)
            e.grid(row=self.prow, column=1, sticky="we", pady=1)
            e.bind("<Return>", lambda ev: commit(v.get()))
            e.bind("<FocusOut>", lambda ev: commit(v.get()))
            self.pvars[prop] = (v, getv)
        self.prow += 1

    def sync_panel(self):
        """Push model values into the panel (after a drag, undo, ...)."""
        for _prop, (var, getv) in self.pvars.items():
            try:
                val = getv()
                if isinstance(var, tk.BooleanVar):
                    var.set(bool(val))
                elif str(var.get()) != str(val):
                    var.set(str(val))
            except tk.TclError:
                pass

    def set_app(self, v):
        v = v.strip()
        if v == self.project["app"]:
            return
        if not M.is_ident(v):
            messagebox.showerror("Application name", "Use letters, digits and _ only (it becomes the file and program name).")
            self.sync_panel()
            return
        self.checkpoint()
        self.project["app"] = v.lower()
        self.pvars["app"] = (self.pvars["app"][0], lambda: self.project["app"])
        self.refresh()

    def set_prop(self, obj, prop, typ, val, is_form=False):
        cur = obj.get(prop) if is_form else M.get(obj, prop)
        if typ == "int":
            try:
                val = int(str(val).strip())
            except ValueError:
                self.sync_panel()
                return
            if prop in ("w", "h"):
                val = max(4, val)
            if is_form and prop == "w":
                val = min(val, M.MAX_W)
            if is_form and prop == "h":
                val = min(val, M.MAX_H - self.top())
        elif typ == "str" and prop == "name":
            val = str(val).strip()
            if val == cur:
                return
            others = self.project["forms"] if is_form else self.form["widgets"]
            if not M.is_ident(val) or any(o is not obj and o["name"] == val for o in others):
                messagebox.showerror("Name", "A name must be unique and use only letters, digits and _ "
                                             "(it becomes a C identifier).")
                self.sync_panel()
                return
        if val == cur:
            return
        self.checkpoint()
        if prop == "name" and not is_form and obj["kind"] == "tabs":
            for w in self.form["widgets"]:
                if w.get("tabs") == cur:
                    w["tabs"] = val
        if prop == "checked" and val and obj.get("kind") == "radio":
            for w in self.form["widgets"]:
                if w is not obj and w["kind"] == "radio" and M.get(w, "group") == M.get(obj, "group"):
                    w["checked"] = False
        obj[prop] = val
        self.refresh(rebuild_panel=prop == "name")

    def toggle_event(self, obj, ev, on):
        self.checkpoint()
        evs = [e for e in obj.get("events", []) if e != ev]
        if on:
            evs.append(ev)
        obj["events"] = evs
        self.refresh()

    def edit_prop(self, obj, prop, typ):
        if typ == "image":
            d = ImageDialog(self, M.get(obj, "image"))
            if d.result is None:
                return
            self.checkpoint()
            obj["image"] = d.result
            if (obj["w"], obj["h"]) == (32, 32) or not M.get(obj, "border"):
                obj["w"], obj["h"] = d.result["w"], d.result["h"]
        elif typ == "items":
            d = TextDialog(self, "Items of %s" % obj["name"], "\n".join(M.get(obj, "items")),
                           hint="One item per line.")
            if d.result is None:
                return
            self.checkpoint()
            obj["items"] = [ln.strip() for ln in d.result.splitlines() if ln.strip()]
        else:
            d = TextDialog(self, "Text of %s" % obj["name"], str(M.get(obj, prop)), height=6)
            if d.result is None:
                return
            self.checkpoint()
            obj[prop] = d.result if typ == "text" else d.result.replace("\n", " ")
        self.refresh(rebuild_panel=True)

    # ---- files --------------------------------------------------------------
    def confirm_discard(self):
        if not self.dirty:
            return True
        r = messagebox.askyesnocancel("PlusUI Designer", "Save the changes to this project first?")
        if r is None:
            return False
        return self.save() if r else True

    def new_project(self):
        if not self.confirm_discard():
            return
        self.project, self.path, self.dirty, self.fi, self.sel = M.new_project(), None, False, 0, []
        self.undo_stack, self.redo_stack = [], []
        self.refresh(rebuild_panel=True)

    def open_dialog(self):
        if not self.confirm_discard():
            return
        path = filedialog.askopenfilename(title="Open a PlusUI project",
                                          filetypes=[("PlusUI projects", "*.pui"), ("All files", "*.*")])
        if path:
            self.open_path(path)
            self.refresh(rebuild_panel=True)

    def open_path(self, path):
        try:
            self.project = M.load(path)
        except (OSError, ValueError, KeyError) as e:
            messagebox.showerror("Open", "Could not open %s\n\n%s" % (path, e))
            return
        self.path, self.dirty, self.fi, self.sel = path, False, 0, []
        self.undo_stack, self.redo_stack = [], []

    def save(self):
        if not self.path:
            return self.save_as()
        try:
            M.save(self.project, self.path)
        except OSError as e:
            messagebox.showerror("Save", str(e))
            return False
        self.dirty = False
        self.refresh()
        return True

    def save_as(self):
        path = filedialog.asksaveasfilename(title="Save the project", defaultextension=".pui",
                                            initialfile=self.project["app"] + ".pui",
                                            filetypes=[("PlusUI projects", "*.pui")])
        if not path:
            return False
        self.path = path
        return self.save()

    def generate(self):
        self.focus_set()                        # commit a property box that is still being edited
        self.update_idletasks()
        errs = M.validate(self.project)
        if errs:
            messagebox.showerror("Generate Code", "Fix these first:\n\n" + "\n".join(errs))
            return
        default = self.gen_dir or (os.path.dirname(self.path) if self.path else os.getcwd())
        d = GenerateDialog(self, default, self.gen_lang)
        if not d.result:
            return
        self.gen_dir, self.gen_lang = d.result
        try:
            log = puigen.generate(self.project, self.gen_dir, self.gen_lang,
                                  os.path.basename(self.path) if self.path else "the PlusUI Designer")
        except (ValueError, OSError) as e:
            messagebox.showerror("Generate Code", str(e))
            return
        messagebox.showinfo("Generate Code", "In %s:\n\n%s" % (self.gen_dir, "\n".join(log)))

    def quit_app(self):
        if self.confirm_discard():
            self.destroy()


def main():
    app = Designer(sys.argv[1] if len(sys.argv) > 1 else None)
    app.mainloop()


if __name__ == "__main__":
    main()
