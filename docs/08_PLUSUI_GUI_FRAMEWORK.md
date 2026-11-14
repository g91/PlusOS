# PlusUI: the GUI framework, the Designer and the code generator

PlusUI is the PlusOS widget framework. It has three parts:

| Part | Where | What it does |
|:---|:---|:---|
| **PlusUI framework** | `sdks/macplus/include/plusos/ui.h`, `sdks/macplus/src/gfx/ui.c` (same files in `sdks/macse`) | Forms (windows), widgets, menus, dialogs, the event loop. Part of `libplusgfx.a`, so every app already links it. |
| **PlusUI Designer** | `tools/uidesigner/designer.py` | A drag-and-drop form editor for Windows, macOS and Linux. Saves a `.pui` project. |
| **puigen** | `tools/uidesigner/puigen.py` | Turns a `.pui` project into C (or C++) source: the code that builds the forms, plus a stub with an empty function for every handler. |

The workflow: draw the forms in the Designer, save the `.pui`, generate code, fill in the handlers, build.

---

## 1. Quick start

```
python tools/uidesigner/designer.py            (or tools\uidesigner\designer.cmd / designer.sh)
```

1. Drag widgets from the palette onto the form. Select, move, resize; edit properties on the right.
2. With nothing selected the right panel shows the form itself: set the **app name**, the form's name, title and size.
3. Tick the handlers you want under **Handlers to generate** (a button's `action` is ticked for you).
4. **File > Save**, then **File > Generate Code**. Choose a folder such as `apps/tools/myapp`.
5. Add `MYAPP` to `include_apps=` in `multiboot.cfg`, then build:

```
make.cmd apps
make.cmd image
```

Any folder `apps/<category>/<name>/` is built into `<name>.pex` automatically; there is no makefile to edit.

The same generation step from the command line:

```
python tools/uidesigner/puigen.py apps/tools/myapp/myapp.pui
python tools/uidesigner/puigen.py myapp.pui --out some/folder --lang cpp
```

A complete worked example is `apps/development/uidemo` (`uidemo.pui`, the generated `uidemo_ui.c/.h`, and the hand-written `uidemo.c`).

---

## 2. What the generator writes

For an app named `myapp` with a form named `main`:

| File | Rewritten on every run? | Contents |
|:---|:---|:---|
| `myapp_ui.h` | yes | `MainForm` struct (one `UIWidget *` per widget), `main_form_create()`, menu constants, handler prototypes |
| `myapp_ui.c` | yes | builds every form: widgets, menus, pictures, flags, handler hook-up |
| `myapp.c` | **no** | `main()` and an empty function per handler. Created once; later runs only *append* stubs for handlers that are new in the design |

So it is always safe to change the design and generate again: your code in `myapp.c` is never overwritten.

Every widget is reachable by name:

```c
#include "myapp_ui.h"

void main_btnGreet_action(UIWidget *w)          /* generated name: <form>_<widget>_<event> */
{
    char s[64];
    snprintf(s, sizeof s, "Hello, %s!", ui_get_text(main_form.txtName));
    ui_message_box("Greeting", s, UI_MB_OK);
}
```

### C or C++

* **C** (default) builds with the PlusOS toolchain (`make.cmd apps`) and with gcc. The headers are `extern "C"` clean.
* **C++** (`--lang cpp`) writes a `<Form>Base` class per form with a virtual method per handler, and a stub that subclasses it. The P68 compiler does not support C++ classes yet, so this output needs the gcc toolchain. If you build with `make.cmd`, use C.

---

## 3. The Designer

| To do this | Do this |
|:---|:---|
| Add a widget | Drag it from the palette onto the form, or click it and then click the form |
| Select | Click. Shift-click adds or removes. Drag on empty space to box-select |
| Move / resize | Drag the widget / drag a handle. Arrow keys move 1 pixel, Shift-arrows 8 |
| Edit text, list items or a picture | Double-click the widget |
| Resize the form | Drag the dark handles on its right and bottom edges, or type the size |
| Line widgets up | Select several, then the **Align** menu (the first one picked is the reference) |
| Menu bar | **Form > Menu Bar...**, one menu per line: `File: New^N, Open..., -, Quit^Q` |
| More forms | **Add** under the form list. The first form is the one the program opens |
| Tab pages | Click a tab on the form to show that page, then drop widgets onto it |
| Draw an icon | Add an **Image / icon**, double-click it: paint pixels, or import a PNG/GIF |

Undo/redo, cut/copy/paste and duplicate work as usual. Snapping to an 8-pixel grid is on by default (**View** menu).

Group boxes and tabs carry the widgets inside them when you move them. A widget dropped on a tab page belongs to that page and is only shown while that tab is selected.

The canvas is drawn by a port of the framework's own drawing code (`puirender.py`), so the form on screen is pixel-for-pixel what PlusOS draws.

### Limits worth knowing

* The largest window on a 512 x 342 Mac is 496 x 300 pixels (menu bar included); the Designer enforces it.
* Text is the 8 x 8 system font: each character is 8 pixels wide, ASCII only.
* Forms are black and white. On colour Macs the system expands them, as it does for every PlusOS window.

---

## 4. The framework

### Widgets

| Kind | Value / text | Events |
|:---|:---|:---|
| `UI_LABEL` | text, `\n` for more lines; left / centre / right | |
| `UI_BUTTON` | caption; `UI_F_DEFAULT` (Return), `UI_F_CANCEL` (Esc) | action |
| `UI_CHECKBOX` | 0 / 1 | change |
| `UI_RADIO` | 0 / 1, exclusive within its group number | change |
| `UI_TEXTFIELD` | one line; max length, password, read-only | change, action (Return) |
| `UI_TEXTAREA` | many lines, wraps | change |
| `UI_LISTBOX` | items; value = selected index (-1 none); scrolls | change, action (double click / Return) |
| `UI_COMBO` | items; value = selected index | change |
| `UI_SLIDER` | min..max; vertical when taller than wide | change |
| `UI_SCROLLBAR` | min..max and a page size | change |
| `UI_PROGRESS` | min..max | |
| `UI_GROUP` | titled frame | |
| `UI_IMAGE` | 1-bit picture; clickable | action |
| `UI_SEPARATOR` | horizontal or vertical rule | |
| `UI_TABS` | items = page titles; value = page | change |
| `UI_CANVAS` | your own drawing | draw, mouse |

### The calls you will use most

```c
/* text and values */
ui_set_text(w, "text");     ui_set_textf(w, "%d items", n);     const char *s = ui_get_text(w);
ui_set_value(w, 1);         int v = ui_get_value(w);            ui_set_range(w, 0, 100, 10);

/* lists, combos, tabs */
int i = ui_add_item(w, "New");   ui_remove_item(w, i);   ui_clear_items(w);
ui_item_count(w);                ui_get_item(w, ui_get_value(w));

/* state */
ui_enable(w, 0);   ui_show(w, 0);   ui_focus(w);   ui_select_all(w);   ui_invalidate(w);

/* dialogs */
ui_message_box("Title", "Text", UI_MB_OK);                         /* also OKCANCEL, YESNO, YESNOCANCEL */
if (ui_message_box("Quit", "Really?", UI_MB_YESNO) == UI_YES) ui_quit(0);
char name[32] = "";
if (ui_input_box("Name", "Your name:", name, sizeof name)) { ... }

/* forms */
second_form_create();  ui_form_open(second_form.form);            /* a second window */
int r = ui_form_run_modal(about_form.form);                        /* a dialog: blocks until ... */
ui_form_end(w->form, UI_OK);                                       /* ... a handler ends it */
ui_form_close(f);  ui_form_set_title(f, "New title");  ui_form_resize(f, 320, 200);
ui_form_set_timer(f, 60, on_timer);                                /* 60 ticks = 1 second */
ui_quit(0);                                                        /* leave ui_run() */
```

`ui_run()` returns when `ui_quit()` is called or the last form is closed.

Windows are moved by dragging their title bar and closed with the close box; the desktop does both. To react to the close box (for example to ask "Save changes?"), tick the form's `close` handler and return 0 from it to keep the form open.

### Keyboard and mouse behaviour built in

* Tab / Shift-Tab move between text fields and lists. Return presses the default button, Esc the cancel button.
* Text boxes: arrows, Home/End, Shift-arrows to select, drag to select, double-click selects all, Cmd-A/C/X/V with the system clipboard.
* Buttons, check boxes and radio buttons track the mouse while it is held: sliding off cancels the click.
* Menus and combo boxes work both ways: press-drag-release, or click to open and click again to choose.
* Menu items ending in `^X` get the shortcut Command-X.

### Writing a form by hand

The Designer is optional. The same thing in code:

```c
#include <plusos/plusos.h>
#include <plusos/ui.h>

static UIWidget *name;
static void on_ok(UIWidget *w) { ui_message_box("Hello", ui_get_text(name), UI_MB_OK); }

int main(void)
{
    UIForm *f = ui_form_new("Hello", 240, 72);
    ui_add(f, UI_LABEL, 8, 12, 48, 10, "Name:");
    name = ui_add(f, UI_TEXTFIELD, 64, 8, 168, 16, "");
    UIWidget *ok = ui_add(f, UI_BUTTON, 160, 36, 72, 28, "OK");
    ui_set_flags(ok, UI_F_DEFAULT, 0);
    ui_on_action(ok, on_ok);
    if (ui_form_open(f) != 0) return 1;         /* forms need the desktop running */
    return ui_run();
}
```

### Custom drawing

A canvas calls your `draw` handler with its rectangle on the current PlusGFX target (already clipped), and your `mouse` handler with coordinates inside the canvas while the button is down. Call `ui_invalidate(w)` to have it redrawn. See the sketch pad in `apps/development/uidemo/uidemo.c`.

---

## 5. Testing without a Mac

`tools/uidesigner/test/run_uitest.py` compiles the framework with the P68 toolchain, runs it on the emulator's bare 68k core with a fake kernel, feeds it clicks and keys, checks the results and writes every frame as a PNG:

```
python tools/uidesigner/test/run_uitest.py                       (frames in build_p68/uitest)
```

`test/uitest.c` covers every widget; `test/demotest.c` does the same for the generated `uidemo` code (the command line is in its header comment). `test/uitest_host.h` is the fake kernel, reusable for testing your own forms.
