/* demotest.c - headless test of GENERATED code: builds the uidemo forms exactly as
 * puigen.py emitted them, runs the app's real handlers and dumps frames.
 *
 *   python tools/uidesigner/test/run_uitest.py --out build_p68/demotest -I apps/development/uidemo
 *       -D uidemo.c:main=uidemo_main --src tools/uidesigner/test/demotest.c
 *       apps/development/uidemo/uidemo_ui.c apps/development/uidemo/uidemo.c
 */
#include "uitest_host.h"
#include "uidemo_ui.h"

int main(void)
{
    static GSurface surf, asurf;
    UIForm *f;
    host_init();

    f = main_form_create();
    CHECK(f != 0);
    CHECK(main_form_create() == f);                     /* built once */
    gs_init(&surf, f->w, f->h + f->top);
    dump_surf = &surf;
    ui_form_attach(f, &surf);
    ui_form_draw(f);
    dump("d1_controls");

    /* the app's own handlers */
    click(f, 32, 64);                                   /* Turbo check box */
    CHECK(ui_get_value(main_form.chkTurbo) == 1);
    CHECK(!strcmp(ui_get_text(main_form.lblStatus), "Turbo on."));
    click(f, 92, 84);                                   /* Fast radio */
    CHECK(ui_get_value(main_form.radFast) == 1 && ui_get_value(main_form.radSlow) == 0);
    click(f, 180, 66);                                  /* third fruit */
    CHECK(!strcmp(ui_get_text(main_form.lblStatus), "Picked Cherry."));
    click(f, 300, 100);                                 /* Remove */
    CHECK(ui_item_count(main_form.lstFruit) == 9);
    click(f, 84 + 4 + 171 / 2, 128);                    /* slider -> about half */
    CHECK(ui_get_value(main_form.prgVolume) == ui_get_value(main_form.sldVolume));
    CHECK(ui_get_value(main_form.sldVolume) >= 48 && ui_get_value(main_form.sldVolume) <= 52);
    tick += 120; main_timer(f);
    ui_form_draw(f);
    dump("d2_controls_used");

    click(f, 100, 10);                                  /* "Text" tab */
    CHECK(ui_get_value(main_form.tabs) == 1);
    click(f, 80, 34);
    CHECK(f->focus == main_form.txtName);
    type(f, "Ada");
    key(f, KEY_TAB);
    CHECK(f->focus == main_form.txtPass);
    type(f, "hunter2");
    key(f, KEY_TAB);
    CHECK(f->focus == main_form.areaNotes);
    type(f, "!");
    CHECK(strstr(ui_get_text(main_form.lblCount), "characters") != 0);
    dump("d3_text");

    click(f, 150, 10);                                  /* "Extras" tab */
    CHECK(ui_get_value(main_form.tabs) == 2);
    main_cnvDraw_mouse(main_form.cnvDraw, 10, 10, 1);
    main_cnvDraw_mouse(main_form.cnvDraw, 120, 80, 1);
    main_cnvDraw_mouse(main_form.cnvDraw, 230, 12, 1);
    main_cnvDraw_mouse(main_form.cnvDraw, 230, 12, 0);
    click(f, 30, 40);                                   /* the icon */
    CHECK(!strcmp(ui_get_text(main_form.lblStatus), "You clicked the icon."));
    click(f, 272 + 90, 120);                            /* scroll bar right arrow */
    CHECK(!strcmp(ui_get_text(main_form.lblScroll), "scroll: 1"));
    click(f, 300, 170);                                 /* disabled button: nothing */
    ui_form_draw(f);
    dump("d4_extras");

    CHECK(about_form_create() != 0);
    gs_init(&asurf, about_form.form->w, about_form.form->h);
    ui_form_attach(about_form.form, &asurf);
    ui_form_draw(about_form.form);
    dump_surf = &asurf;
    dump("d5_about");

    puts_(failures ? "RESULT FAIL\n" : "RESULT OK\n");
    return failures;
}
