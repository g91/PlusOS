"""puimodel.py - the PlusUI project model shared by the designer and the code generator.

A project (.pui file, JSON) is:

    { "format": "plusui-1", "app": "myapp",
      "forms": [ { "name": "main", "title": "My App", "w": 320, "h": 200, "pos": "auto",
                   "menus": [ { "title": "File", "items": ["New^N", "-", "Quit^Q"] } ],
                   "timer": 0, "events": ["close", "key", "timer"],
                   "widgets": [ { "kind": "button", "name": "btnOk", "x": 8, "y": 8,
                                  "w": 72, "h": 20, "text": "OK", ... } ] } ] }

Widget coordinates are pixels in the form's client area; text is an 8 x 8 font.
"""
import copy
import json
import re

FORMAT = "plusui-1"
MENU_H = 12
MAX_W, MAX_H = 496, 300         # the window server's largest window on a 512 x 342 screen

# kind -> (C constant, default w, default h, default text, palette label)
KINDS = {
    "label":     ("UI_LABEL",     64, 10, "Label",     "Label"),
    "button":    ("UI_BUTTON",    72, 20, "Button",    "Button"),
    "checkbox":  ("UI_CHECKBOX",  96, 14, "Check box", "Check box"),
    "radio":     ("UI_RADIO",     96, 14, "Radio",     "Radio button"),
    "textfield": ("UI_TEXTFIELD", 120, 16, "",         "Text field"),
    "textarea":  ("UI_TEXTAREA",  160, 64, "",         "Text area"),
    "listbox":   ("UI_LISTBOX",   120, 64, "",         "List box"),
    "combo":     ("UI_COMBO",     120, 18, "",         "Combo box"),
    "slider":    ("UI_SLIDER",    120, 14, "",         "Slider"),
    "scrollbar": ("UI_SCROLLBAR", 120, 13, "",         "Scroll bar"),
    "progress":  ("UI_PROGRESS",  120, 12, "",         "Progress bar"),
    "group":     ("UI_GROUP",     136, 64, "Group",    "Group box"),
    "image":     ("UI_IMAGE",     32, 32, "",          "Image / icon"),
    "separator": ("UI_SEPARATOR", 120, 4, "",          "Separator"),
    "tabs":      ("UI_TABS",      240, 100, "",        "Tabs"),
    "canvas":    ("UI_CANVAS",    96, 64, "",          "Canvas"),
}
KIND_ORDER = ["label", "button", "checkbox", "radio", "textfield", "textarea", "listbox", "combo",
              "slider", "scrollbar", "progress", "group", "image", "separator", "tabs", "canvas"]

# property name -> (type, default). Types: str, text (multi-line), int, bool, items, choice:<a,b,c>, image
COMMON_PROPS = [("name", "str"), ("x", "int"), ("y", "int"), ("w", "int"), ("h", "int"),
                ("enabled", "bool"), ("visible", "bool"), ("tabs", "str"), ("tab_page", "int")]
KIND_PROPS = {
    "label":     [("text", "text"), ("align", "choice:left,center,right"), ("border", "bool")],
    "button":    [("text", "str"), ("default", "bool"), ("cancel", "bool")],
    "checkbox":  [("text", "str"), ("checked", "bool")],
    "radio":     [("text", "str"), ("checked", "bool"), ("group", "int")],
    "textfield": [("text", "str"), ("maxlen", "int"), ("password", "bool"), ("readonly", "bool")],
    "textarea":  [("text", "text"), ("maxlen", "int"), ("readonly", "bool")],
    "listbox":   [("items", "items"), ("value", "int")],
    "combo":     [("items", "items"), ("value", "int")],
    "slider":    [("min", "int"), ("max", "int"), ("value", "int")],
    "scrollbar": [("min", "int"), ("max", "int"), ("page", "int"), ("value", "int")],
    "progress":  [("min", "int"), ("max", "int"), ("value", "int")],
    "group":     [("text", "str")],
    "image":     [("image", "image"), ("border", "bool")],
    "separator": [],
    "tabs":      [("items", "items"), ("value", "int")],
    "canvas":    [("border", "bool")],
}
# events a widget kind can raise -> handler suffix
KIND_EVENTS = {
    "button": ["action"], "checkbox": ["change"], "radio": ["change"],
    "textfield": ["change", "action"], "textarea": ["change"], "listbox": ["change", "action"],
    "combo": ["change"], "slider": ["change"], "scrollbar": ["change"], "image": ["action"],
    "tabs": ["change"], "canvas": ["draw", "mouse"],
}
DEFAULT_EVENTS = {"button": ["action"], "canvas": ["draw"]}
FORM_EVENTS = ["close", "key", "timer"]

DEFAULTS = {"text": "", "enabled": True, "visible": True, "tabs": "", "tab_page": 0,
            "align": "left", "border": False, "default": False, "cancel": False, "checked": False,
            "group": 0, "maxlen": 63, "password": False, "readonly": False, "items": [],
            "value": 0, "min": 0, "max": 100, "page": 10, "image": None}

PREFIX = {"label": "lbl", "button": "btn", "checkbox": "chk", "radio": "rad", "textfield": "txt",
          "textarea": "area", "listbox": "lst", "combo": "cmb", "slider": "sld", "scrollbar": "scr",
          "progress": "prg", "group": "grp", "image": "img", "separator": "sep", "tabs": "tabs",
          "canvas": "cnv"}


def props_for(kind):
    return COMMON_PROPS + KIND_PROPS[kind]


def prop_default(kind, prop):
    if prop == "text":
        return KINDS[kind][3]
    if prop == "maxlen":
        return 1023 if kind == "textarea" else 63
    if prop == "value":
        return -1 if kind == "listbox" else 50 if kind in ("slider", "progress") else 0
    if prop == "items":
        return {"listbox": ["Item 1", "Item 2", "Item 3"], "combo": ["Choice 1", "Choice 2"],
                "tabs": ["Tab 1", "Tab 2"]}.get(kind, [])
    if prop == "border":
        return kind == "canvas"
    return copy.deepcopy(DEFAULTS.get(prop))


def get(w, prop):
    """A widget property, falling back to the kind's default."""
    v = w.get(prop)
    return prop_default(w["kind"], prop) if v is None else v


def is_ident(s):
    return bool(re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", s or ""))


def new_widget(form, kind, x, y):
    _c, w, h, _t, _l = KINDS[kind]
    wd = {"kind": kind, "name": unique_name(form, PREFIX[kind]), "x": x, "y": y, "w": w, "h": h}
    for p, _t in KIND_PROPS[kind]:
        wd[p] = prop_default(kind, p)
    wd["events"] = list(DEFAULT_EVENTS.get(kind, []))
    return wd


def unique_name(form, prefix):
    names = {w["name"] for w in form["widgets"]}
    prefix = re.sub(r"\d+$", "", prefix) or "w"
    n = 1
    while "%s%d" % (prefix, n) in names:
        n += 1
    return "%s%d" % (prefix, n)


def new_form(project, name=None):
    names = {f["name"] for f in project["forms"]}
    if not name:
        n = 1
        name = "main" if "main" not in names else None
        while not name:
            n += 1
            if "form%d" % n not in names:
                name = "form%d" % n
    return {"name": name, "title": name.capitalize(), "w": 320, "h": 200, "pos": "auto",
            "menus": [], "timer": 0, "events": [], "widgets": []}


def new_project(app="myapp"):
    p = {"format": FORMAT, "app": app, "forms": []}
    p["forms"].append(new_form(p))
    return p


def form_top(form):
    return MENU_H if form.get("menus") else 0


def menus_to_text(menus):
    return "\n".join("%s: %s" % (m["title"], ", ".join(m["items"])) for m in menus)


def menus_from_text(text):
    """One menu per line: `File: New^N, Open..., -, Quit^Q`."""
    menus = []
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        title, _, rest = line.partition(":")
        items = [i.strip() for i in rest.split(",") if i.strip()]
        if title.strip():
            menus.append({"title": title.strip(), "items": items})
    return menus


def validate(project):
    """Returns a list of problems (strings); empty when the project can be generated."""
    errs = []
    if not is_ident(project.get("app", "")):
        errs.append("The app name %r is not a valid C identifier." % project.get("app"))
    seen = set()
    for f in project.get("forms", []):
        fn = f.get("name", "")
        if not is_ident(fn):
            errs.append("Form name %r is not a valid C identifier." % fn)
        if fn in seen:
            errs.append("Two forms are named %r." % fn)
        seen.add(fn)
        if f["w"] > MAX_W or f["h"] + form_top(f) > MAX_H:
            errs.append("Form %s is %dx%d; the largest window on a compact Mac is %dx%d."
                        % (fn, f["w"], f["h"] + form_top(f), MAX_W, MAX_H))
        wn = set()
        tabs = {w["name"]: w for w in f["widgets"] if w["kind"] == "tabs"}
        for w in f["widgets"]:
            n = w.get("name", "")
            if w["kind"] not in KINDS:
                errs.append("%s.%s: unknown widget kind %r." % (fn, n, w["kind"]))
                continue
            if not is_ident(n):
                errs.append("%s: widget name %r is not a valid C identifier." % (fn, n))
            if n in wn:
                errs.append("%s: two widgets are named %r." % (fn, n))
            wn.add(n)
            t = w.get("tabs")
            if t and (t not in tabs or t == n):
                errs.append("%s.%s: its tabs property names %r, which is not a tabs widget." % (fn, n, t))
    if not project.get("forms"):
        errs.append("The project has no forms.")
    return errs


def load(path):
    with open(path, "r", encoding="utf-8") as fh:
        p = json.load(fh)
    if p.get("format") != FORMAT:
        raise ValueError("%s is not a PlusUI project (format %r)" % (path, p.get("format")))
    for f in p["forms"]:
        f.setdefault("menus", [])
        f.setdefault("events", [])
        f.setdefault("timer", 0)
        f.setdefault("pos", "auto")
        f.setdefault("widgets", [])
        for w in f["widgets"]:
            w.setdefault("events", [])
    return p


def save(project, path):
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(project, fh, indent=1)
        fh.write("\n")


# ---- 1-bit images -----------------------------------------------------------
def image_new(w=32, h=32):
    return {"w": w, "h": h, "rows": ["0" * (((w + 15) // 16) * 4)] * h}


def image_get(img, x, y):
    row = bytes.fromhex(img["rows"][y])
    return (row[x >> 3] >> (7 - (x & 7))) & 1


def image_pixels(img):
    """rows of 0/1 lists"""
    out = []
    for y in range(img["h"]):
        row = bytes.fromhex(img["rows"][y])
        out.append([(row[x >> 3] >> (7 - (x & 7))) & 1 for x in range(img["w"])])
    return out


def image_from_pixels(px):
    h = len(px)
    w = len(px[0]) if h else 0
    rb = ((w + 15) // 16) * 2
    rows = []
    for y in range(h):
        b = bytearray(rb)
        for x in range(w):
            if px[y][x]:
                b[x >> 3] |= 0x80 >> (x & 7)
        rows.append(b.hex())
    return {"w": w, "h": h, "rows": rows}
