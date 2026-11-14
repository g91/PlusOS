/* uidemo.c - the program behind the uidemo forms: a tour of every PlusUI widget.
 *
 * The forms were drawn in the PlusUI Designer (uidemo.pui) and uidemo_ui.c / .h were
 * generated from it by tools/uidesigner/puigen.py. This file started life as the stub
 * the generator writes, and is hand-written from there on: the generator never
 * rewrites it, it only appends empty handlers for anything new in the design.
 */
#include <plusos/plusos.h>
#include <plusos/ui.h>
#include <stdio.h>
#include <string.h>
#include "uidemo_ui.h"

static void status(const char *s) { ui_set_text(main_form.lblStatus, s); }

/* ---- page 1: controls ------------------------------------------------------ */
void main_chkSound_change(UIWidget *w) { status(ui_get_value(w) ? "Sound on." : "Sound off."); }
void main_chkTurbo_change(UIWidget *w) { status(ui_get_value(w) ? "Turbo on." : "Turbo off."); }
void main_radSlow_change(UIWidget *w)  { (void)w; status("Speed: slow."); }
void main_radFast_change(UIWidget *w)  { (void)w; status("Speed: fast."); }

void main_lstFruit_change(UIWidget *w)
{
    char s[48];
    snprintf(s, sizeof s, "Picked %s.", ui_get_item(w, ui_get_value(w)));
    status(s);
}

/* double click (or Return) on a fruit */
void main_lstFruit_action(UIWidget *w)
{
    char s[64];
    snprintf(s, sizeof s, "You chose %s.", ui_get_item(w, ui_get_value(w)));
    ui_message_box("Fruit", s, UI_MB_OK);
}

void main_cmbSize_change(UIWidget *w)
{
    char s[48];
    snprintf(s, sizeof s, "Size: %s.", ui_get_item(w, ui_get_value(w)));
    status(s);
}

void main_btnAdd_action(UIWidget *w)
{
    char name[24] = "";
    (void)w;
    if (ui_input_box("Add item", "Name of the new item:", name, sizeof name) && name[0]) {
        ui_set_value(main_form.lstFruit, ui_add_item(main_form.lstFruit, name));
        status("Item added.");
    }
}

void main_btnRemove_action(UIWidget *w)
{
    int sel = ui_get_value(main_form.lstFruit);
    (void)w;
    if (sel < 0) { status("Select an item first."); return; }
    ui_remove_item(main_form.lstFruit, sel);
    status("Item removed.");
}

void main_sldVolume_change(UIWidget *w)
{
    ui_set_value(main_form.prgVolume, ui_get_value(w));
    ui_set_textf(main_form.lblVolumeVal, "%d", ui_get_value(w));
}

/* ---- page 2: text ---------------------------------------------------------- */
void main_btnGreet_action(UIWidget *w)
{
    char s[96];
    const char *name = ui_get_text(main_form.txtName);
    (void)w;
    if (!name[0]) { ui_message_box("Greet", "Type your name first.", UI_MB_OK); ui_focus(main_form.txtName); return; }
    snprintf(s, sizeof s, "Hello, %s! Your password has %d characters.", name,
             (int)strlen(ui_get_text(main_form.txtPass)));
    ui_message_box("Greet", s, UI_MB_OK);
}

void main_txtName_action(UIWidget *w) { main_btnGreet_action(w); }      /* Return in the name box */

void main_areaNotes_change(UIWidget *w)
{
    ui_set_textf(main_form.lblCount, "%d characters", (int)strlen(ui_get_text(w)));
}

/* ---- page 3: extras -------------------------------------------------------- */
void main_imgMac_action(UIWidget *w) { (void)w; status("You clicked the icon."); }

/* The canvas keeps its picture in an off-screen bitmap: the mouse handler draws
 * into it, the draw handler copies it to the form. */
static GSurface sketch;
static int last_x = -1, last_y;

void main_cnvDraw_draw(UIWidget *w, int l, int t, int r, int b)
{
    (void)w;
    if (sketch.bits) gfx_blit(&sketch, 0, 0, r - l, b - t, l, t, GFX_COPY);
}

void main_cnvDraw_mouse(UIWidget *w, int x, int y, int down)
{
    GSurface *old = gfx_get_target();
    if (!sketch.bits && gs_init(&sketch, w->w, w->h) != 0) return;
    if (!down) { last_x = -1; return; }
    gfx_target(&sketch);
    if (last_x < 0) gfx_pixel(x, y, 1);
    else gfx_line(last_x, last_y, x, y, 1);
    gfx_target(old);
    last_x = x; last_y = y;
    ui_invalidate(w);
}

void main_btnClear_action(UIWidget *w)
{
    (void)w;
    if (sketch.bits) memset(sketch.bits, 0, (size_t)sketch.rowbytes * (size_t)sketch.h);
    ui_invalidate(main_form.cnvDraw);
}

void main_scrDemo_change(UIWidget *w) { ui_set_textf(main_form.lblScroll, "scroll: %d", ui_get_value(w)); }
void main_btnDisabled_action(UIWidget *w) { (void)w; }

/* ---- the form -------------------------------------------------------------- */
static void show_about(void)
{
    if (about_form_create()) ui_form_run_modal(about_form.form);
}

void about_btnOk_action(UIWidget *w) { ui_form_end(w->form, UI_OK); }
void main_btnAbout_action(UIWidget *w) { (void)w; show_about(); }

static int confirm_quit(void)
{
    return ui_message_box("Quit", "Leave the PlusUI demo?", UI_MB_YESNO) == UI_YES;
}

void main_btnQuit_action(UIWidget *w) { (void)w; if (confirm_quit()) ui_quit(0); }

void main_menu(UIForm *f, int menu, int item)
{
    char name[32] = "World";
    (void)f;
    switch (menu) {
    case MAIN_MENU_FILE:
        switch (item) {
        case MAIN_FILE_ABOUT: show_about(); break;
        case MAIN_FILE_QUIT:  if (confirm_quit()) ui_quit(0); break;
        }
        break;
    case MAIN_MENU_DIALOGS:
        switch (item) {
        case MAIN_DIALOGS_MESSAGE_BOX:
            ui_message_box("Message", "This is a message box. Long text is wrapped to fit the window automatically.", UI_MB_OK);
            break;
        case MAIN_DIALOGS_ASK_A_QUESTION: {
            int r = ui_message_box("Question", "Do you like this framework?", UI_MB_YESNOCANCEL);
            status(r == UI_YES ? "You said yes." : r == UI_NO ? "You said no." : "You cancelled.");
            break;
        }
        case MAIN_DIALOGS_INPUT_BOX:
            if (ui_input_box("Input", "What is your name?", name, sizeof name)) {
                char s[64];
                snprintf(s, sizeof s, "Hello, %s.", name);
                status(s);
            }
            break;
        }
        break;
    }
}

/* the close box: ask first; returning 0 keeps the form open */
int main_close(UIForm *f)
{
    (void)f;
    if (confirm_quit()) ui_quit(0);
    return 0;
}

/* once a second */
static uint32_t started;
void main_timer(UIForm *f)
{
    (void)f;
    ui_set_textf(main_form.lblClock, "Running for %lu s", (unsigned long)((plusos->ticks() - started) / 60));
}

int main(void)
{
    started = plusos->ticks();
    if (!main_form_create() || ui_form_open(main_form.form) != 0) {
        printf("uidemo needs the PlusOS desktop.\n");
        return 1;
    }
    return ui_run();        /* returns after ui_quit(), or when the last form is closed */
}
