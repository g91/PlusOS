#!/usr/bin/env python3
"""puigen.py - turn a PlusUI project (.pui, drawn in the PlusUI Designer) into code.

    python tools/uidesigner/puigen.py myapp.pui [--out DIR] [--lang c|cpp]

For an app called `myapp` it writes, into DIR (default: next to the .pui file):

  --lang c  (default; builds with the P68 toolchain and with gcc)
    myapp_ui.h   GENERATED  one struct per form, handler prototypes, menu constants
    myapp_ui.c   GENERATED  the code that builds every form
    myapp.c      STUB       main() and an empty function for every handler

  --lang cpp  (class per form; needs the gcc toolchain - P68 has no C++ classes yet)
    myapp_ui.hpp / myapp_ui.cpp   GENERATED   a <Form>Base class per form, virtual handlers
    myapp.cpp                     STUB        subclasses with the handlers, and main()

The GENERATED files are rewritten on every run. The STUB is written once and then
belongs to you: later runs never rewrite it, they only append stubs for handlers that
were added in the designer since (C), so regenerating after a design change is safe.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import puimodel as M  # noqa: E402

EDIT = ("textfield", "textarea")


# ---- naming -----------------------------------------------------------------
def camel(name):
    return "".join(p[:1].upper() + p[1:] for p in re.split(r"_+", name) if p) or "X"


def cstr(s):
    out = []
    for ch in str(s):
        o = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append(" ")
        elif 0x20 <= o < 0x7F:
            out.append(ch)
        else:
            out.append("?")             # the system font is ASCII only
    return '"' + "".join(out) + '"'


def const_name(s):
    s = re.sub(r"\^.$", "", s)          # drop the "^Q" shortcut
    s = re.sub(r"[^A-Za-z0-9]+", "_", s).strip("_").upper()
    return s or "ITEM"


def menu_consts(form):
    """[(menu const, index)], [(item const, index)] with unique names"""
    fu = form["name"].upper()
    menus, items, used = [], [], set()

    def uniq(n):
        base, k = n, 2
        while n in used:
            n = "%s_%d" % (base, k)
            k += 1
        used.add(n)
        return n

    for mi, m in enumerate(form.get("menus", [])):
        mc = const_name(m["title"])
        menus.append((uniq("%s_MENU_%s" % (fu, mc)), mi))
        for ii, it in enumerate(m["items"]):
            if it != "-":
                items.append((uniq("%s_%s_%s" % (fu, mc, const_name(it))), ii, mi, it))
    return menus, items


def handlers(form):
    """Every handler the form needs: [(key, C name, return type, C args, comment, body)]"""
    fn = form["name"]
    out = []
    for w in form["widgets"]:
        for ev in w.get("events", []):
            if ev not in M.KIND_EVENTS.get(w["kind"], []):
                continue
            name = "%s_%s_%s" % (fn, w["name"], ev)
            if ev == "draw":
                out.append((w["name"] + "." + ev, name, "void", "UIWidget *w, int l, int t, int r, int b",
                            "draw %s: (l, t)-(r, b) is its rectangle on the current gfx target" % w["name"], ""))
            elif ev == "mouse":
                out.append((w["name"] + "." + ev, name, "void", "UIWidget *w, int x, int y, int down",
                            "the mouse is at (x, y) inside %s; down = 0 on release" % w["name"], ""))
            else:
                what = {"action": {"button": "was pressed", "image": "was clicked", "textfield": "got Return",
                                   "listbox": "item was double-clicked"}.get(w["kind"], "action"),
                        "change": "changed (ui_get_value / ui_get_text)"}[ev]
                out.append((w["name"] + "." + ev, name, "void", "UIWidget *w", "%s %s" % (w["name"], what), ""))
    if form.get("menus"):
        out.append(("menu", fn + "_menu", "void", "UIForm *f, int menu, int item", "a menu item was chosen", "MENU"))
    evs = form.get("events", [])
    if "close" in evs:
        out.append(("close", fn + "_close", "int", "UIForm *f",
                    "the close box was clicked: return 0 to keep the form open", "    return 1;\n"))
    if "key" in evs:
        out.append(("key", fn + "_key", "void", "UIForm *f, int key", "a key no widget used", ""))
    if "timer" in evs and form.get("timer", 0) > 0:
        out.append(("timer", fn + "_timer", "void", "UIForm *f",
                    "fires every %d ticks (60 ticks = 1 second)" % form["timer"], ""))
    return out


def image_bytes(img):
    return [b for row in img["rows"] for b in bytes.fromhex(row)]


def flags_of(w):
    g = lambda p: M.get(w, p)
    k = w["kind"]
    fl = []
    if not g("enabled"):
        fl.append("UI_F_DISABLED")
    if not g("visible"):
        fl.append("UI_F_HIDDEN")
    if k == "button" and g("default"):
        fl.append("UI_F_DEFAULT")
    if k == "button" and g("cancel"):
        fl.append("UI_F_CANCEL")
    if k == "textfield" and g("password"):
        fl.append("UI_F_PASSWORD")
    if k in EDIT and g("readonly"):
        fl.append("UI_F_READONLY")
    if k in ("label", "image", "canvas") and g("border"):
        fl.append("UI_F_BORDER")
    if k == "label" and g("align") == "center":
        fl.append("UI_F_CENTER")
    if k == "label" and g("align") == "right":
        fl.append("UI_F_RIGHT")
    return fl


def build_body(form, handler_ref, ind="    ", member="s->"):
    """The statements that create a form's widgets. handler_ref(key) -> C expression."""
    fn = form["name"]
    L = []
    for i, w in enumerate(form["widgets"]):
        g = lambda p, w=w: M.get(w, p)
        k = w["kind"]
        kc = M.KINDS[k][0]
        items = [str(x) for x in g("items")] if k in ("listbox", "combo", "tabs") else []
        packed = items and all("|" not in it and it and len(it) < 64 for it in items)
        if k in EDIT:
            text = '""'
        elif items:
            text = cstr("|".join(items)) if packed else '""'
        elif k in ("label", "button", "checkbox", "radio", "group"):
            text = cstr(g("text"))
        else:
            text = '""'
        L.append("%sw = %s%s = ui_add(f, %s, %d, %d, %d, %d, %s);"
                 % (ind, member, w["name"], kc, w["x"], w["y"], w["w"], w["h"], text))
        L.append("%sif (!w) return %s;" % (ind, "false" if member != "s->" else "0"))
        st = ["w->id = %d;" % (i + 1)]
        if items and not packed:
            st += ["ui_add_item(w, %s);" % cstr(it) for it in items]
        if k in EDIT:
            if g("maxlen") != M.prop_default(k, "maxlen"):
                st.append("ui_set_maxlen(w, %d);" % g("maxlen"))
            if g("text"):
                st.append("ui_set_text(w, %s);" % cstr(g("text")))
        if k in ("slider", "scrollbar", "progress"):
            st.append("ui_set_range(w, %d, %d, %d);" % (g("min"), g("max"), g("page") if k == "scrollbar" else 0))
            st.append("ui_set_value(w, %d);" % g("value"))
        if k in ("checkbox", "radio"):
            if k == "radio" and g("group"):
                st.append("ui_set_group(w, %d);" % g("group"))
            if g("checked"):
                st.append("ui_set_value(w, 1);")
        if k in ("listbox", "combo", "tabs") and g("value") != M.prop_default(k, "value"):
            st.append("ui_set_value(w, %d);" % g("value"))
        if k == "image" and g("image"):
            img = g("image")
            st.append("ui_set_image(w, %s_%s_bits, %d, %d);" % (fn, w["name"], img["w"], img["h"]))
        fl = flags_of(w)
        if fl:
            st.append("ui_set_flags(w, %s, 0);" % " | ".join(fl))
        for ev in w.get("events", []):
            if ev in M.KIND_EVENTS.get(k, []):
                st.append("w->on_%s = %s;" % (ev, handler_ref(w["name"] + "." + ev)))
        line = ind
        for s in st:                                    # pack the setters onto readable lines
            if len(line) + len(s) > 100 and line.strip():
                L.append(line.rstrip())
                line = ind
            line += s + " "
        if line.strip():
            L.append(line.rstrip())
    for w in form["widgets"]:
        if w.get("tabs"):
            L.append("%sui_set_tab_page(%s%s, %s%s, %d);"
                     % (ind, member, w["name"], member, w["tabs"], M.get(w, "tab_page")))
    return L


def static_data(form):
    """image bitmaps and menu tables for one form"""
    fn = form["name"]
    L = []
    for w in form["widgets"]:
        img = M.get(w, "image") if w["kind"] == "image" else None
        if img:
            data = image_bytes(img)
            L.append("static const uint8_t %s_%s_bits[] = {    /* %d x %d, 1 bpp */" % (fn, w["name"], img["w"], img["h"]))
            for i in range(0, len(data), 16):
                L.append("    " + ",".join("0x%02x" % b for b in data[i:i + 16]) + ",")
            L.append("};")
    menus = form.get("menus", [])
    for mi, m in enumerate(menus):
        L.append("static const char *const %s_menu%d_items[] = { %s };"
                 % (fn, mi, ", ".join(cstr(i) for i in m["items"]) or "0"))
    if menus:
        L.append("static const UIMenu %s_menus[] = {" % fn)
        for mi, m in enumerate(menus):
            L.append("    { %s, %s_menu%d_items, %d }," % (cstr(m["title"]), fn, mi, len(m["items"])))
        L.append("};")
    return L


def menu_enums(form, ind=""):
    menus, items = menu_consts(form)
    L = []
    if menus:
        L.append("%senum { %s };" % (ind, ", ".join("%s = %d" % (n, i) for n, i in menus)))
        by_menu = {}
        for n, ii, mi, _it in items:
            by_menu.setdefault(mi, []).append("%s = %d" % (n, ii))
        for mi in sorted(by_menu):
            L.append("%senum { %s };" % (ind, ", ".join(by_menu[mi])))
    return L


def menu_switch(form, ind="    ", quit_stmt="ui_quit(0);"):
    menus, items = menu_consts(form)
    L = ["%sswitch (menu) {" % ind]
    for mn, mi in menus:
        L.append("%scase %s:" % (ind, mn))
        L.append("%s    switch (item) {" % ind)
        for n, _ii, m2, it in items:
            if m2 != mi:
                continue
            label = re.sub(r"\^.$", "", it)
            body = quit_stmt if label.lower() in ("quit", "exit") else "/* TODO: %s */" % label
            L.append("%s    case %s: %s break;" % (ind, n, body))
        L.append("%s    }" % ind)
        L.append("%s    break;" % ind)
    L.append("%s}" % ind)
    return L


def pos_stmt(form):
    pos = form.get("pos", "auto")
    if pos == "center":
        return "ui_form_set_pos(f, UI_POS_CENTER, UI_POS_CENTER);"
    return None


HEADER_NOTE = "GENERATED by puigen.py from %s - do not edit; change the design and re-run the generator."


# ---- C ----------------------------------------------------------------------
def gen_c(project, src_name):
    app = project["app"]
    guard = app.upper() + "_UI_H"
    H = ["/* %s_ui.h - %s */" % (app, HEADER_NOTE % src_name),
         "#ifndef %s" % guard, "#define %s" % guard, "",
         "#include <plusos/ui.h>", "", "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
    C = ["/* %s_ui.c - %s */" % (app, HEADER_NOTE % src_name),
         '#include "%s_ui.h"' % app, ""]
    for form in project["forms"]:
        fn = form["name"]
        T = camel(fn) + "Form"
        hs = handlers(form)
        H.append("/* ---- form \"%s\" (%s) %s */" % (fn, form["title"], "-" * max(4, 50 - len(fn) - len(form["title"]))))
        H.append("typedef struct %s {" % T)
        H.append("    UIForm   *form;")
        for w in form["widgets"]:
            H.append("    UIWidget *%s;" % w["name"])
        H.append("} %s;" % T)
        H.append("extern %s %s_form;" % (T, fn))
        H.append("UIForm *%s_form_create(void);      /* builds the form on first call; 0 = out of memory */" % fn)
        H += menu_enums(form)
        if hs:
            H.append("/* handlers: write these in %s.c */" % app)
            for _k, name, ret, args, cm, _b in hs:
                H.append("%s %s(%s);" % (ret.ljust(4), name, args))
        H.append("")

        C += static_data(form)
        C.append("%s %s_form;" % (T, fn))
        C.append("")
        C.append("UIForm *%s_form_create(void)" % fn)
        C.append("{")
        C.append("    %s *s = &%s_form;" % (T, fn))
        C.append("    UIForm *f;")
        C.append("    UIWidget *w;")
        C.append("")
        C.append("    if (s->form) return s->form;")
        C.append("    f = ui_form_new(%s, %d, %d);" % (cstr(form["title"]), form["w"], form["h"]))
        C.append("    if (!f) return 0;")
        if pos_stmt(form):
            C.append("    " + pos_stmt(form))
        href = {k: n for k, n, _r, _a, _c, _b in hs}
        if form.get("menus"):
            C.append("    ui_form_set_menus(f, %s_menus, %d, %s);" % (fn, len(form["menus"]), href["menu"]))
        if "close" in href:
            C.append("    f->on_close = %s;" % href["close"])
        if "key" in href:
            C.append("    f->on_key = %s;" % href["key"])
        if "timer" in href:
            C.append("    ui_form_set_timer(f, %d, %s);" % (form["timer"], href["timer"]))
        C.append("    (void)w;")
        C.append("")
        C += build_body(form, lambda k: href[k])
        C.append("    s->form = f;")
        C.append("    return f;")
        C.append("}")
        C.append("")
    H += ["#ifdef __cplusplus", "}", "#endif", "#endif", ""]
    return "\n".join(H), "\n".join(C)


def c_stub_fn(form, h):
    _k, name, ret, args, cm, body = h
    L = ["/* %s */" % cm, "%s %s(%s)" % (ret, name, args), "{"]
    if body == "MENU":
        L += menu_switch(form)
    elif body:
        L.append(body.rstrip("\n"))
    L += ["}", ""]
    return L


def gen_c_stub(project, src_name):
    app = project["app"]
    main = project["forms"][0]
    S = ["/* %s.c - the program behind the %s forms." % (app, app),
         " *",
         " * puigen.py wrote this file once as a starting point; it is yours now. Running the",
         " * generator again never rewrites it - it only appends empty handlers for anything",
         " * new in the design. The forms themselves live in %s_ui.c (regenerated every time)." % app,
         " *",
         " * Every widget is reachable as <form>_form.<name>, for example:",
         " *     ui_set_text(%s_form.%s, \"hello\");" % (main["name"], ([w for w in main["widgets"] if w["kind"] in ("label", "textfield")] or main["widgets"] or [{"name": "lblName"}])[0]["name"]),
         " *     int v = ui_get_value(...);   ui_message_box(\"Title\", \"Text\", UI_MB_OK);",
         " */",
         "#include <plusos/plusos.h>", "#include <plusos/ui.h>", "#include <stdio.h>", "#include <string.h>",
         '#include "%s_ui.h"' % app, ""]
    for form in project["forms"]:
        for h in handlers(form):
            S += c_stub_fn(form, h)
    S += ["int main(void)", "{",
          "    if (!%s_form_create() || ui_form_open(%s_form.form) != 0) {" % (main["name"], main["name"]),
          '        printf("%s needs the PlusOS desktop.\\n");' % app,
          "        return 1;", "    }"]
    for form in project["forms"][1:]:
        S.append("    /* to show \"%s\":  %s_form_create(); ui_form_open(%s_form.form);   or, as a dialog:" % (form["name"], form["name"], form["name"]))
        S.append("     *                  int r = ui_form_run_modal(%s_form.form);  (end it with ui_form_end(f, r)) */" % form["name"])
    S += ["    return ui_run();        /* returns after ui_quit(), or when the last form is closed */", "}", ""]
    return "\n".join(S)


def append_missing_c(project, path):
    """Append stubs for handlers the user's file does not define yet. Returns their names."""
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    add, names = [], []
    for form in project["forms"]:
        for h in handlers(form):
            if not re.search(r"\b%s\s*\(" % re.escape(h[1]), text):
                add += c_stub_fn(form, h)
                names.append(h[1])
    if add:
        with open(path, "a", encoding="utf-8", newline="\n") as fh:
            fh.write(("" if text.endswith("\n") else "\n") + "\n" + "\n".join(add))
    return names


# ---- C++ --------------------------------------------------------------------
def cpp_method(h):
    """(method name, return type, args without the widget/form pointer, call args)"""
    key, _name, ret, args, cm, body = h
    m = key.replace(".", "_")
    if key == "close":
        return "closing", "bool", "", ""
    parts = [a.strip() for a in args.split(",")][1:]            # drop UIWidget *w / UIForm *f
    return m, ret, ", ".join(parts), ", ".join(p.split()[-1].lstrip("*") for p in parts)


def gen_cpp(project, src_name):
    app = project["app"]
    guard = app.upper() + "_UI_HPP"
    H = ["// %s_ui.hpp - %s" % (app, HEADER_NOTE % src_name),
         "#ifndef %s" % guard, "#define %s" % guard, "", "#include <plusos/ui.h>", ""]
    C = ["// %s_ui.cpp - %s" % (app, HEADER_NOTE % src_name), '#include "%s_ui.hpp"' % app, ""]
    for form in project["forms"]:
        fn = form["name"]
        T = camel(fn) + "FormBase"
        hs = handlers(form)
        H.append("// ---- form \"%s\" (%s): subclass it and override the handlers ----" % (fn, form["title"]))
        H.append("class %s {" % T)
        H.append("public:")
        H.append("    UIForm   *form;")
        for w in form["widgets"]:
            H.append("    UIWidget *%s;" % w["name"])
        H += menu_enums(form, "    ")
        H.append("")
        H.append("    %s() : form(0) {}" % T)
        H.append("    bool create();                  // builds the form on first call")
        H.append("    bool open()     { return create() && ui_form_open(form) == 0; }")
        H.append("    void close()    { if (form) ui_form_close(form); }")
        H.append("    int  runModal() { return create() ? ui_form_run_modal(form) : UI_CANCEL; }")
        H.append("    void end(int result) { if (form) ui_form_end(form, result); }")
        H.append("")
        H.append("protected:")
        for h in hs:
            m, ret, args, _call = cpp_method(h)
            dflt = " return true; " if ret == "bool" else ""
            H.append("    virtual %s %s(%s) {%s}     // %s" % (ret, m, args, dflt or " ", h[4]))
        H.append("")
        H.append("private:")
        for h in hs:
            H.append("    static %s t_%s(%s);" % (h[2], cpp_method(h)[0], h[3]))
        H.append("};")
        H.append("")

        C += static_data(form)
        for h in hs:
            m, ret, _args, call = cpp_method(h)
            owner = "f->user" if h[3].startswith("UIForm") else "w->form->user"
            stmt = "static_cast<%s *>(%s)->%s(%s)" % (T, owner, m, call)
            if h[0] == "close":
                C.append("int %s::t_%s(%s) { return %s ? 1 : 0; }" % (T, m, h[3], stmt))
            else:
                C.append("void %s::t_%s(%s) { %s; }" % (T, m, h[3], stmt))
        C.append("")
        C.append("bool %s::create()" % T)
        C.append("{")
        C.append("    UIForm *f;")
        C.append("    UIWidget *w;")
        C.append("")
        C.append("    if (form) return true;")
        C.append("    f = ui_form_new(%s, %d, %d);" % (cstr(form["title"]), form["w"], form["h"]))
        C.append("    if (!f) return false;")
        C.append("    f->user = this;")
        if pos_stmt(form):
            C.append("    " + pos_stmt(form))
        href = {h[0]: "t_" + cpp_method(h)[0] for h in hs}
        if form.get("menus"):
            C.append("    ui_form_set_menus(f, %s_menus, %d, %s);" % (fn, len(form["menus"]), href["menu"]))
        if "close" in href:
            C.append("    f->on_close = %s;" % href["close"])
        if "key" in href:
            C.append("    f->on_key = %s;" % href["key"])
        if "timer" in href:
            C.append("    ui_form_set_timer(f, %d, %s);" % (form["timer"], href["timer"]))
        C.append("    (void)w;")
        C.append("")
        C += build_body(form, lambda k: href[k], member="this->")
        C.append("    form = f;")
        C.append("    return true;")
        C.append("}")
        C.append("")
    H += ["#endif", ""]
    return "\n".join(H), "\n".join(C)


def gen_cpp_stub(project, src_name):
    app = project["app"]
    S = ["// %s.cpp - the program behind the %s forms." % (app, app),
         "//",
         "// puigen.py wrote this file once as a starting point; it is yours now and is never",
         "// rewritten. The form classes live in %s_ui.hpp / %s_ui.cpp (regenerated every time)." % (app, app),
         "#include <plusos/plusos.h>", "#include <plusos/ui.h>", "#include <stdio.h>",
         '#include "%s_ui.hpp"' % app, ""]
    for form in project["forms"]:
        T = camel(form["name"]) + "Form"
        S.append("class %s : public %sBase {" % (T, T))
        S.append("protected:")
        for h in handlers(form):
            m, ret, args, _call = cpp_method(h)
            S.append("    // %s" % h[4])
            S.append("    %s %s(%s) override" % (ret, m, args))
            S.append("    {")
            if h[5] == "MENU":
                S += menu_switch(form, "        ")
            elif ret == "bool":
                S.append("        return true;")
            S.append("    }")
        S.append("};")
        S.append("static %s %s_form;" % (T, form["name"]))
        S.append("")
    main = project["forms"][0]["name"]
    S += ["int main()", "{",
          "    if (!%s_form.open()) {" % main,
          '        printf("%s needs the PlusOS desktop.\\n");' % app,
          "        return 1;", "    }", "    return ui_run();", "}", ""]
    return "\n".join(S)


# ---- driver -----------------------------------------------------------------
def write(path, text):
    old = None
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            old = fh.read()
    if old != text:
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    return old != text


def generate(project, out_dir, lang="c", src_name="the design"):
    """Writes the files; returns a list of human-readable lines describing what happened."""
    errs = M.validate(project)
    if errs:
        raise ValueError("\n".join(errs))
    os.makedirs(out_dir, exist_ok=True)
    app = project["app"]
    log = []
    if lang == "cpp":
        h, c = gen_cpp(project, src_name)
        files = [(app + "_ui.hpp", h), (app + "_ui.cpp", c)]
        stub, stub_text = app + ".cpp", gen_cpp_stub(project, src_name)
    else:
        h, c = gen_c(project, src_name)
        files = [(app + "_ui.h", h), (app + "_ui.c", c)]
        stub, stub_text = app + ".c", gen_c_stub(project, src_name)
    for name, text in files:
        changed = write(os.path.join(out_dir, name), text)
        log.append("%-9s %s" % ("wrote" if changed else "unchanged", name))
    sp = os.path.join(out_dir, stub)
    if not os.path.exists(sp):
        write(sp, stub_text)
        log.append("%-9s %s  (your code goes here; it will not be overwritten)" % ("created", stub))
    elif lang == "c":
        added = append_missing_c(project, sp)
        log.append("%-9s %s" % ("kept", stub) + ("  (appended: %s)" % ", ".join(added) if added else ""))
    else:
        log.append("%-9s %s  (add overrides for any new handlers yourself)" % ("kept", stub))
    return log


def main():
    ap = argparse.ArgumentParser(description="Generate PlusUI code from a .pui design.")
    ap.add_argument("project", help="the .pui file saved by the PlusUI Designer")
    ap.add_argument("--out", help="output folder (default: next to the .pui file)")
    ap.add_argument("--lang", choices=["c", "cpp"], default="c")
    a = ap.parse_args()
    try:
        project = M.load(a.project)
        out = a.out or os.path.dirname(os.path.abspath(a.project))
        for line in generate(project, out, a.lang, os.path.basename(a.project)):
            print(line)
    except (ValueError, OSError) as e:
        sys.stderr.write("puigen: %s\n" % e)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
