/* uitest.c - headless PlusUI render test. Runs on the bare 68k harness
 * (emulator/tools/run68k.mjs): builds forms on an attached surface, feeds events,
 * and dumps each frame as hex for run_uitest.py to turn into PNGs. */
#include "uitest_host.h"

static int clicks, changes, menu_hit = -1, canvas_hits;
static void on_click(UIWidget *w) { (void)w; clicks++; }
static void on_change(UIWidget *w) { (void)w; changes++; }
static void on_menu(UIForm *f, int m, int i) { (void)f; menu_hit = m * 10 + i; }
static void cv_draw(UIWidget *w, int l, int t, int r, int b) { (void)w; gfx_line(l, t, r - 1, b - 1, 1); gfx_line(l, b - 1, r - 1, t, 1); gfx_circle((l + r) / 2, (t + b) / 2, 10, 1); }
static void cv_mouse(UIWidget *w, int x, int y, int down) { (void)w; (void)x; (void)y; (void)down; canvas_hits++; }

static const uint8_t icon[32 * 4] = {
    0x00,0x00,0x00,0x00, 0x07,0xff,0xff,0xe0, 0x08,0x00,0x00,0x10, 0x10,0x00,0x00,0x08,
    0x10,0xff,0xff,0x08, 0x10,0x80,0x01,0x08, 0x10,0x80,0x01,0x08, 0x10,0x8c,0x31,0x08,
    0x10,0x8c,0x31,0x08, 0x10,0x80,0x01,0x08, 0x10,0x83,0xc1,0x08, 0x10,0x80,0x01,0x08,
    0x10,0x80,0x01,0x08, 0x10,0xff,0xff,0x08, 0x10,0x00,0x00,0x08, 0x10,0x00,0x00,0x08,
    0x10,0x00,0x3e,0x08, 0x10,0x00,0x00,0x08, 0x10,0x00,0x00,0x08, 0x0f,0xff,0xff,0xf0,
    0x08,0x00,0x00,0x10, 0x08,0x00,0x00,0x10, 0x0f,0xff,0xff,0xf0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

static const char *const m_file[] = { "New^N", "Open...^O", "-", "Quit^Q" };
static const char *const m_edit[] = { "Cut^X", "Copy^C", "Paste^V" };
static const UIMenu menus[] = { { "File", m_file, 4 }, { "Edit", m_edit, 3 } };

int main(void)
{
    static GSurface surf;
    host_init();

    gs_init(&surf, 432, 292);
    dump_surf = &surf;
    UIForm *f = ui_form_new("Kitchen sink", 432, 280);
    ui_form_set_menus(f, menus, 2, on_menu);
    ui_form_attach(f, &surf);

    ui_add(f, UI_LABEL, 8, 6, 120, 10, "Name:");
    UIWidget *name = ui_add(f, UI_TEXTFIELD, 56, 4, 136, 16, "");
    UIWidget *pw = ui_add(f, UI_TEXTFIELD, 200, 4, 80, 16, "secret");
    ui_set_flags(pw, UI_F_PASSWORD, 0);
    UIWidget *ok = ui_add(f, UI_BUTTON, 296, 0, 64, 24, "OK");
    ui_set_flags(ok, UI_F_DEFAULT, 0); ui_on_action(ok, on_click);
    UIWidget *cancel = ui_add(f, UI_BUTTON, 368, 3, 56, 18, "Cancel");
    ui_set_flags(cancel, UI_F_CANCEL, 0); ui_on_action(cancel, on_click);

    ui_add(f, UI_GROUP, 8, 28, 136, 64, "Options");
    UIWidget *c1 = ui_add(f, UI_CHECKBOX, 16, 40, 120, 14, "Sound");
    UIWidget *c2 = ui_add(f, UI_CHECKBOX, 16, 56, 120, 14, "Disabled");
    ui_set_value(c2, 1); ui_enable(c2, 0);
    ui_on_change(c1, on_change);
    UIWidget *r1 = ui_add(f, UI_RADIO, 16, 74, 56, 14, "One");
    UIWidget *r2 = ui_add(f, UI_RADIO, 76, 74, 56, 14, "Two");
    ui_set_value(r1, 1);

    UIWidget *list = ui_add(f, UI_LISTBOX, 152, 28, 120, 64, "Apple|Banana|Cherry|Date|Elderberry|Fig|Grape|Honeydew|Kiwi");
    ui_on_change(list, on_change);
    UIWidget *combo = ui_add(f, UI_COMBO, 280, 30, 144, 18, "Small|Medium|Large|Huge");
    ui_on_change(combo, on_change);
    UIWidget *sl = ui_add(f, UI_SLIDER, 280, 54, 144, 14, "");
    ui_set_value(sl, 30);
    UIWidget *pr = ui_add(f, UI_PROGRESS, 280, 74, 144, 12, "");
    ui_set_value(pr, 65);

    UIWidget *ta = ui_add(f, UI_TEXTAREA, 8, 98, 184, 66, "The quick brown fox jumps over the lazy dog.\nSecond line.");
    UIWidget *hs = ui_add(f, UI_SCROLLBAR, 200, 98, 160, 13, "");
    ui_set_value(hs, 40);
    UIWidget *vs = ui_add(f, UI_SCROLLBAR, 368, 98, 13, 66, "");
    UIWidget *vsl = ui_add(f, UI_SLIDER, 392, 98, 14, 66, "");
    ui_set_value(vsl, 75);
    ui_add(f, UI_SEPARATOR, 200, 116, 160, 4, "");
    UIWidget *img = ui_add(f, UI_IMAGE, 200, 126, 32, 32, "");
    ui_set_image(img, icon, 32, 32); ui_on_action(img, on_click);
    UIWidget *cv = ui_add(f, UI_CANVAS, 240, 124, 60, 40, "");
    ui_set_flags(cv, UI_F_BORDER, 0); cv->on_draw = cv_draw; cv->on_mouse = cv_mouse;
    UIWidget *lb = ui_add(f, UI_LABEL, 304, 124, 56, 40, "Centred\nlabel");
    ui_set_flags(lb, UI_F_CENTER | UI_F_BORDER, 0);

    UIWidget *tabs = ui_add(f, UI_TABS, 8, 170, 416, 104, "General|Advanced|About");
    UIWidget *t0 = ui_add(f, UI_LABEL, 20, 194, 200, 10, "This is the General page.");
    UIWidget *t0b = ui_add(f, UI_BUTTON, 20, 212, 96, 20, "Page 0 btn");
    UIWidget *t1 = ui_add(f, UI_LABEL, 20, 194, 216, 10, "Advanced settings live here");
    UIWidget *t1c = ui_add(f, UI_CHECKBOX, 20, 212, 160, 14, "Turbo mode");
    ui_set_tab_page(t0, tabs, 0); ui_set_tab_page(t0b, tabs, 0);
    ui_set_tab_page(t1, tabs, 1); ui_set_tab_page(t1c, tabs, 1);

    ui_form_draw(f);
    dump("01_initial");

    /* typing into the focused field */
    ui_focus(name);
    type(f, "Hello PlusUI");
    CHECK(!strcmp(ui_get_text(name), "Hello PlusUI"));
    key(f, KEY_BACKSPACE); key(f, KEY_LEFT); key(f, 'x');
    CHECK(!strcmp(ui_get_text(name), "Hello PlusxU"));
    key(f, KEY_HOME); key(f, KEY_DELETE);
    CHECK(!strcmp(ui_get_text(name), "ello PlusxU"));
    ui_set_text(name, "Hello PlusUI");

    /* checkbox, radio, button, list, tabs */
    click(f, 20, 46);  CHECK(ui_get_value(c1) == 1); CHECK(changes == 1);
    click(f, 20, 62);  CHECK(ui_get_value(c2) == 1);            /* disabled: unchanged */
    click(f, 80, 80);  CHECK(ui_get_value(r2) == 1); CHECK(ui_get_value(r1) == 0);
    click(f, 320, 10); CHECK(clicks == 1);
    click(f, 160, 52); CHECK(ui_get_value(list) == 2);
    key(f, KEY_DOWN); key(f, KEY_DOWN); key(f, KEY_DOWN); key(f, KEY_DOWN); key(f, KEY_DOWN);
    CHECK(ui_get_value(list) == 7); CHECK(list->scroll == 2);
    click(f, 268, 88);                                          /* list scroll bar: down arrow */
    CHECK(list->scroll == 3);
    click(f, 300, 60); CHECK(ui_get_value(sl) > 5 && ui_get_value(sl) < 20);
    click(f, 350, 104); CHECK(ui_get_value(hs) == 41);          /* right arrow */
    click(f, 340, 104); CHECK(ui_get_value(hs) == 51);          /* page area */
    click(f, 374, 160); CHECK(ui_get_value(vs) == 1);
    click(f, 210, 140); CHECK(clicks == 2);
    click(f, 260, 140); CHECK(canvas_hits == 2);
    key(f, KEY_RETURN); CHECK(clicks == 3);                     /* list has focus, no action: default button */
    key(f, KEY_ESC);    CHECK(clicks == 4);

    /* text area: type, newline, navigate */
    click(f, 12, 102);
    CHECK(f->focus == ta);
    key(f, KEY_END); type(f, " END"); key(f, KEY_RETURN); type(f, "new");
    CHECK(strstr(ui_get_text(ta), " END\nnew") != 0);
    dump("02_after_input");

    /* tab switch */
    click(f, 110, 176);
    CHECK(ui_get_value(tabs) == 1);
    click(f, 24, 218); CHECK(ui_get_value(t1c) == 1);
    dump("03_tab2");

    /* a held button, a combo pop-up and a menu, captured while the mouse is down */
    press_hold(f, 320, 10 + f->top, 3, "04_button_pressed");
    CHECK(clicks == 5);
    move_x = 300; move_y = 30 + f->top + 18 + 2 * 11 + 4;       /* drag onto "Large" */
    press_hold(f, 300, 38 + f->top, 3, "05_combo_open");
    CHECK(ui_get_value(combo) == 2);
    move_x = 20; move_y = f->top + 3 * 11 + 4;                  /* drag onto "Quit" */
    press_hold(f, 20, 4, 3, "06_menu_open");
    CHECK(menu_hit == 3);
    press_hold(f, 60, 4, 3, "07_menu_edit");
    dump("08_final");

    /* dialogs, built but not run */
    {
        static GSurface ds;
        UIForm *d = ui_message_form("Save", "Save changes to \"Untitled\" before closing? Anything you have not saved will be lost.", UI_MB_YESNOCANCEL);
        gs_init(&ds, d->w, d->h);
        ui_form_attach(d, &ds);
        ui_form_draw(d);
        dump_surf = &ds; dump("09_msgbox");
        key(d, KEY_RETURN); CHECK(d->result == UI_YES);
        key(d, KEY_ESC);    CHECK(d->result == UI_CANCEL);
        ui_form_free(d);
    }
    ui_remove(lb);
    ui_form_free(f);
    puts_(failures ? "RESULT FAIL\n" : "RESULT OK\n");
    return failures;
}
