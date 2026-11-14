# PlusUI: GUI Framework, Designer & Code Generator

**PlusUI** is the PlusOS toolkit for building graphical programs. Instead of drawing every button and tracking every mouse click by hand, you draw your windows with the mouse on a modern computer, press Generate, and fill in what each button does.

It has three parts that work together:

| Part | What it is | Where it lives |
|:---|:---|:---|
| **PlusUI framework** | The library inside PlusOS that draws the widgets and runs them: windows, buttons, text boxes, lists, menus, message boxes | `plusos/ui.h`, part of `libplusgfx.a` |
| **PlusUI Designer** | A drag-and-drop form editor that runs on Windows, macOS and Linux | `tools/uidesigner/designer.py` |
| **puigen** | The code generator: turns a saved design into C or C++ source with an empty function for every handler | `tools/uidesigner/puigen.py` |

![The PlusUI Designer editing the demo form](/docs/screenshots/26_plusui-designer.png)

---

## 1. Download

**[Download PlusUI-1.0.zip](/downloads/PlusUI-1.0.zip)**

The archive holds the whole system:

- the **Designer** and the **code generator** (Python, nothing to install besides Python itself),
- the **framework sources** (`ui.h` and `ui.c` for both the Mac Plus and Mac SE SDKs),
- the **uidemo** example app: its design file, the generated code and the hand-written logic,
- the **headless tests**,
- the **manual** and a README.

The current PlusOS source tree already contains all of it, so if you build PlusOS from up-to-date sources you do not need the archive. It is there for people who only want to design forms, and for older checkouts.

### What you need

| To do this | You need |
|:---|:---|
| Design forms and save them | Python 3.8 or newer with tkinter (included in the standard Windows and macOS installers) |
| Generate code | The same |
| Build the app for the Mac | The PlusOS source tree and its toolchain (see [SDK & Application Development](/wiki/12-sdk-and-app-development)) |
| Run the app | PlusOS with the desktop running, on any supported Mac or in the emulator |

### Starting the Designer

```text
Windows:        tools\uidesigner\designer.cmd
macOS / Linux:  sh tools/uidesigner/designer.sh
anywhere:       python tools/uidesigner/designer.py [project.pui]
```

To look around first, open the example: `python tools/uidesigner/designer.py apps/development/uidemo/uidemo.pui`

---

## 2. Your First App in Five Minutes

1. Start the Designer. You get an empty form called `main`.
2. With nothing selected, the panel on the right shows the form. Set the application **name** to `hello` (this becomes the program and file name), the form **title** to `Hello`, and the size to 240 by 72.
3. Drag a **Label** from the palette onto the form. Double-click it and type `Name:`.
4. Drag a **Text field** next to it. In the panel, rename it `txtName`.
5. Drag a **Button** below. Set its text to `Greet`, rename it `btnGreet`, and tick **default (Return)** so the Return key presses it. Its handler `main_btnGreet_action()` is already ticked under Handlers to generate.
6. **File > Save** as `hello.pui` in a new folder `apps/tools/hello` inside the PlusOS source tree.
7. **File > Generate Code**, keep C, and generate into that same folder.

You now have three files next to the design:

| File | Rewritten when you generate again? | Contents |
|:---|:---|:---|
| `hello_ui.h` | yes | one struct per form with a pointer to every widget, menu constants, handler prototypes |
| `hello_ui.c` | yes | the code that builds every form |
| `hello.c` | **never** | `main()` and an empty function for each handler. This file is yours |

Open `hello.c` and fill in the handler:

```c
/* btnGreet was pressed */
void main_btnGreet_action(UIWidget *w)
{
    char s[64];
    snprintf(s, sizeof s, "Hello, %s!", ui_get_text(main_form.txtName));
    ui_message_box("Greeting", s, UI_MB_OK);
}
```

Then build and make a disk:

```text
make.cmd apps
make.cmd image
```

Every folder under `apps/<category>/` is built into a program of the same name automatically. Add `HELLO` to the `include_apps=` line in `multiboot.cfg` so it lands on the disk, boot PlusOS, open the desktop, and run `hello` from the Shell or the Programs window.

Because `hello.c` is never overwritten, you can go back to the Designer at any time, change the form, and generate again. New handlers are appended to `hello.c` as empty functions; everything you wrote stays.

---

## 3. What It Looks Like on PlusOS

These are real screenshots from PlusOS running the `uidemo` example.

![The demo app with a combo box open](/docs/screenshots/27_plusui-demo-controls.png)

![A message box opened from the Dialogs menu](/docs/screenshots/28_plusui-message-box.png)

![The canvas page: drawing with the mouse](/docs/screenshots/29_plusui-canvas.png)

And every widget on one form, as drawn by the framework:

![Every PlusUI widget on one form](/docs/screenshots/30_plusui-all-widgets.png)

---

## 4. Using the Designer

### The window

- **Left**: the widget palette, and below it the list of forms in the project.
- **Middle**: the form, shown exactly as PlusOS draws it, inside a mock window frame.
- **Right**: the properties of whatever is selected. With nothing selected it shows the form and the application name.
- **Bottom**: the size of the form and the position of the selection.

### Working with widgets

| To do this | Do this |
|:---|:---|
| Add a widget | Drag it from the palette onto the form. Or click it in the palette, then click the form |
| Select | Click it. Shift-click adds or removes one. Drag across empty space to select everything inside the box |
| Move | Drag it. Arrow keys move one pixel, Shift-arrows eight |
| Resize | Drag one of the eight handles around a single selected widget |
| Edit text, list items or a picture | Double-click the widget |
| Copy, cut, paste, duplicate | Edit menu, or Ctrl/Cmd C, X, V, D |
| Undo and redo | Ctrl/Cmd Z and Y, up to 100 steps |
| Delete | Delete or Backspace |
| Put one in front of or behind another | Edit > Bring to Front, Send to Back |
| Line several up | Select them, then the Align menu. The first one you picked is the reference |
| Cancel a tool or clear the selection | Esc |

Widgets snap to an 8-pixel grid, which matches the 8-pixel character width and keeps things lined up. Turn snapping or the grid dots off in the View menu. The View menu also sets the zoom from 1x to 4x.

### The form

- **Resize** it by dragging the dark handles on its right edge, bottom edge or corner, or type the width and height.
- **Position**: `auto` lets the desktop cascade the window; `center` centres it on the screen.
- **Menu bar**: Form > Menu Bar. Type one menu per line.

```text
File: New^N, Open...^O, -, Quit^Q
Edit: Cut^X, Copy^C, Paste^V
```

A lone `-` is a separator line. An item ending in `^Q` gets the keyboard shortcut Command-Q. An item named Quit or Exit is wired to quit the program in the generated stub.

- **Timer**: set timer ticks (60 ticks is one second) and tick the `timer` handler to have a function called regularly.
- **Handlers**: `close` is called when the close box is clicked, so you can ask "Save changes?"; `key` receives keys no widget used.

### Several forms

Click **Add** under the form list for another window: an About box, a settings dialog, a second document window. The first form in the list is the one the program opens when it starts. The others are opened from your code, either as a normal window or as a dialog that blocks until it is answered (see section 7).

### Tabs and group boxes

- A **Tabs** widget has one page per item. Click a tab on the form to show that page, then drop widgets onto it. They belong to that page and are only visible while it is selected, both in the Designer and on PlusOS.
- Dragging a widget out of the tabs area takes it off the page.
- Moving a tabs widget or a **Group box** carries along everything inside it.

### Drawing icons and pictures

Add an **Image / icon** widget and double-click it to open the picture editor.

- Click or drag to paint. The first pixel you touch decides whether you are painting black or white.
- Set any size up to 256 by 256, clear or invert the picture.
- **Import** reads a PNG or GIF and converts it to black and white.

Tick the image's `action` handler to make it a clickable icon button.

### Limits the Designer enforces

- The largest window on a 512 by 342 compact Mac is **496 by 300** pixels, menu bar included.
- Names must be valid C identifiers and unique, because they become variable names.
- Text is the 8 by 8 system font: every character is 8 pixels wide, plain ASCII.
- Forms are black and white. On colour Macs PlusOS expands them, as it does for every window.

---

## 5. Widget Reference

| Widget | What it holds | Handlers |
|:---|:---|:---|
| **Label** | Text; a new line starts a second row. Left, centre or right aligned, optional border | none |
| **Button** | A caption. Can be the default button (Return) or the cancel button (Esc) | action |
| **Check box** | On or off | change |
| **Radio button** | On or off; only one in the same group number can be on | change |
| **Text field** | One line of text. Maximum length, password dots, read-only | change, action (Return) |
| **Text area** | Many lines, wraps at the edge | change |
| **List box** | A scrolling list of items; the selected row, or -1 for none | change, action (double-click or Return) |
| **Combo box** | A drop-down list; the selected item | change |
| **Slider** | A value between min and max. Vertical when taller than wide | change |
| **Scroll bar** | A value between min and max with a page size; arrows, page areas and a thumb | change |
| **Progress bar** | A value between min and max, display only | none |
| **Group box** | A titled frame around related widgets | none |
| **Image / icon** | A 1-bit picture, optionally clickable | action |
| **Separator** | A horizontal or vertical line | none |
| **Tabs** | A row of tabs; widgets are assigned to pages | change |
| **Canvas** | An area you draw yourself | draw, mouse |

Every widget also has a name, position, size, and the **enabled** and **visible** switches. A disabled widget is drawn greyed out and ignores the mouse.

### Behaviour you get for free

- **Tab** and **Shift-Tab** move between text boxes and lists.
- **Return** presses the default button, **Esc** the cancel button.
- Text boxes: arrow keys, Home and End, Shift-arrows to select, drag to select, double-click to select all, and Command-A, C, X, V with the system clipboard.
- Buttons, check boxes and radio buttons follow the mouse while it is held, so sliding off before releasing cancels the click, as on a real Mac.
- Menus and combo boxes work both ways: press, drag and release; or click to open and click again to choose.
- Scroll bar arrows and page areas repeat while held; the thumb drags.
- Windows are moved by their title bar and closed with the close box, handled by the desktop.

---

## 6. The Generated Code

For an app named `myapp` with a form named `main`, generated handler names follow one pattern: form, widget, event.

```c
void main_btnOk_action(UIWidget *w);        /* a button was pressed            */
void main_chkSound_change(UIWidget *w);     /* a value or text changed         */
void main_menu(UIForm *f, int menu, int item);   /* a menu item was chosen     */
int  main_close(UIForm *f);                 /* close box: return 0 to stay open */
void main_key(UIForm *f, int key);          /* a key no widget used            */
void main_timer(UIForm *f);                 /* the form's timer fired          */
void main_cnvDraw_draw(UIWidget *w, int l, int t, int r, int b);   /* canvas   */
void main_cnvDraw_mouse(UIWidget *w, int x, int y, int down);      /* canvas   */
```

Every widget is reachable by name through the form's struct:

```c
ui_set_text(main_form.lblStatus, "Saved.");
int volume = ui_get_value(main_form.sldVolume);
```

Menus get named constants, so the generated menu handler is a readable switch:

```c
void main_menu(UIForm *f, int menu, int item)
{
    switch (menu) {
    case MAIN_MENU_FILE:
        switch (item) {
        case MAIN_FILE_NEW:  /* TODO: New */ break;
        case MAIN_FILE_QUIT: ui_quit(0); break;
        }
        break;
    }
}
```

### From the command line

The Designer's Generate Code command and this are the same thing:

```text
python tools/uidesigner/puigen.py myapp.pui
python tools/uidesigner/puigen.py myapp.pui --out apps/tools/myapp
python tools/uidesigner/puigen.py myapp.pui --lang cpp
```

This is what makes the hand-off work: a designer sends you a `.pui` file, and you run one command to get compilable source.

### C or C++

- **C** is the default and the one to use with the standard PlusOS build (`make.cmd apps`). The headers are also usable from C++.
- **C++** (`--lang cpp`) writes one base class per form with a virtual method per handler, plus a stub that subclasses it. The PlusOS compiler does not support C++ classes yet, so this output needs the gcc cross toolchain.

```cpp
class MainForm : public MainFormBase {
protected:
    void btnGreet_action() override
    {
        ui_message_box("Greeting", ui_get_text(txtName), UI_MB_OK);
    }
};
```

---

## 7. Framework Quick Reference

Include `plusos/ui.h`. All calls start with `ui_`.

### Text and values

```c
ui_set_text(w, "text");
ui_set_textf(w, "%d items", n);          /* printf style */
const char *s = ui_get_text(w);
ui_set_value(w, 1);                      /* check, radio, slider, list, combo, tabs, progress */
int v = ui_get_value(w);
ui_set_range(w, 0, 100, 10);             /* min, max, page size */
ui_set_maxlen(w, 31);                    /* text boxes */
```

### Lists, combo boxes and tabs

```c
int i = ui_add_item(w, "New item");      /* returns its index */
ui_remove_item(w, i);
ui_clear_items(w);
int n = ui_item_count(w);
const char *sel = ui_get_item(w, ui_get_value(w));
```

### State

```c
ui_enable(w, 0);          /* grey it out */
ui_show(w, 0);            /* hide it */
ui_focus(w);              /* put the caret in a text box */
ui_select_all(w);
ui_invalidate(w);         /* redraw (for a canvas whose picture changed) */
```

### Dialogs

```c
ui_message_box("Title", "Text", UI_MB_OK);

if (ui_message_box("Quit", "Really quit?", UI_MB_YESNO) == UI_YES)
    ui_quit(0);

int r = ui_message_box("Save", "Save changes?", UI_MB_YESNOCANCEL);   /* UI_YES, UI_NO or UI_CANCEL */

char name[32] = "";
if (ui_input_box("Name", "What is your name?", name, sizeof name)) {
    /* OK was pressed; name holds the text */
}
```

Long message text is word-wrapped and the box sizes itself. Dialogs open centred. While one is open the windows behind it do not respond, and clicking them brings the dialog back to the front.

### Forms

```c
/* a second window */
if (settings_form_create()) ui_form_open(settings_form.form);

/* a dialog you designed: blocks until a handler ends it */
if (about_form_create()) {
    int result = ui_form_run_modal(about_form.form);
}
void about_btnOk_action(UIWidget *w) { ui_form_end(w->form, UI_OK); }

ui_form_close(f);
ui_form_set_title(f, "Untitled - edited");
ui_form_resize(f, 320, 200);
ui_form_set_timer(f, 60, on_timer);       /* every second */
ui_quit(0);                               /* leave ui_run() */
```

`ui_run()` returns when `ui_quit()` is called or the last form is closed.

### Custom drawing

A canvas calls your `draw` handler with its rectangle on the current PlusGFX drawing target, already clipped to the canvas, so any call from [the graphics engine](/wiki/10-graphics-and-3d-engine) works. The `mouse` handler receives positions inside the canvas while the button is held, and once more with `down` set to 0 on release. Call `ui_invalidate(w)` after changing what should be shown. The sketch pad in `apps/development/uidemo/uidemo.c` is a complete example.

### Without the Designer

The Designer is optional. The same form in plain code:

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
    if (ui_form_open(f) != 0) return 1;      /* forms need the desktop running */
    return ui_run();
}
```

The complete API with comments is in `sdks/macplus/include/plusos/ui.h`.

---

## 8. The Example: uidemo

`apps/development/uidemo` is a tour of the whole toolkit and is included in disk images built from the current sources. Run `uidemo` from the desktop Shell.

| File | What it is |
|:---|:---|
| `uidemo.pui` | The design. Open it in the Designer |
| `uidemo_ui.h`, `uidemo_ui.c` | Generated from the design |
| `uidemo.c` | The hand-written logic: about 200 lines for the whole app |

It has three tab pages (controls, text entry, extras with an icon and a sketch pad), a menu bar, a one-second timer, a second form used as a modal About box, message boxes, an input box, and a close-box handler that asks before quitting.

---

## 9. The .pui File

A project is a small JSON text file, easy to email, diff and keep in version control.

```text
{
 "format": "plusui-1",
 "app": "hello",
 "forms": [
  {
   "name": "main", "title": "Hello", "w": 240, "h": 72, "pos": "auto",
   "menus": [ { "title": "File", "items": ["Quit^Q"] } ],
   "timer": 0, "events": [],
   "widgets": [
    { "kind": "button", "name": "btnGreet", "x": 160, "y": 36, "w": 72, "h": 28,
      "text": "Greet", "default": true, "events": ["action"] }
   ]
  }
 ]
}
```

Coordinates are pixels inside the window, below the menu bar. Pictures are stored as rows of hexadecimal, one bit per pixel.

---

## 10. Testing Without a Mac

PlusUI comes with tests that need no Macintosh and no screen. They compile the framework with the PlusOS toolchain, run it on the emulator's 68000 core with a stand-in kernel, feed it clicks and keys, check the results and save every frame as a PNG.

```text
python tools/uidesigner/test/run_uitest.py
```

Frames are written to `build_p68/uitest`. `test/uitest.c` covers every widget; `test/demotest.c` does the same for the generated uidemo code. `test/uitest_host.h` is the stand-in kernel, which you can reuse to test your own forms the same way.

The Designer's preview is checked against these frames: it matches the real rendering pixel for pixel.

---

## 11. Troubleshooting

| Problem | Cause and fix |
|:---|:---|
| The program prints "needs the PlusOS desktop" | Forms are windows on the desktop. Start the desktop first, then run the app from its Shell or Programs window |
| The app is not on the disk | Add its name in capitals to `include_apps=` in `multiboot.cfg`, then rebuild the image |
| Generate Code says a name is not valid | Widget, form and app names must be letters, digits and underscores, and unique |
| The Designer will not start | Python needs tkinter. On Linux install the `python3-tk` package |
| A new handler does not exist in my source | Generate again. Missing handlers are appended to the end of your `.c` file |
| The form is cut off on the Mac | Keep it within 496 by 300 pixels including the menu bar |
| C++ output does not build with make.cmd | Expected: use C with the PlusOS toolchain, or build the C++ output with gcc |
| Text shows a question mark | The system font is ASCII only; other characters are replaced |

---

## Navigation

- **Download**: [PlusUI-1.0.zip](/downloads/PlusUI-1.0.zip)
- **Manual (plain text)**: [08_PLUSUI_GUI_FRAMEWORK.md](/docs/08_PLUSUI_GUI_FRAMEWORK.md)
- **Previous Chapter**: [← 21. IKANET: Chat Client & Server](/wiki/21-ikanet-chat-client)
- **Building Apps**: [12. SDK & Application Development Guide →](/wiki/12-sdk-and-app-development)
- **Windows & Desktop**: [7. Desktop & Window Server →](/wiki/07-desktop-and-window-server)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
