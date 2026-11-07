/* notes.c - Classic System 7.1 Note Pad Desk Accessory (8 Persistent Pages)
 *
 * Saves 8 pages of notes to "Note Pad File" on the boot HFS volume.
 * Works as a GUI window on the Desktop or from the command line.
 */
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <string.h>

#define NPAGES   8
#define PAGE_MAX 256

static char pages[NPAGES][PAGE_MAX];
static int  cur_page = 0;
static const unsigned char note_file[] = "\x0D" "Note Pad File";

static void load_notes(void)
{
    memset(pages, 0, sizeof pages);
    strlcpy(pages[0], "PlusOS Note Pad\nPage 1 of 8.\nType here to take notes!\nClick the dog-ear corner\nor press Tab for next page.", PAGE_MAX);
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    INTEGER ref;
    if (FSOpen(note_file, vref, &ref) != 0) return;
    LONGINT n = sizeof pages;
    FSRead(ref, &n, pages);
    FSClose(ref);
    for (int i = 0; i < NPAGES; i++) pages[i][PAGE_MAX - 1] = 0;
}

static void save_notes(void)
{
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    INTEGER ref;
    if (FSOpen(note_file, vref, &ref) != 0) {
        Create(note_file, vref, 0x504C4F53UL /* 'PLOS' */, 0x54455854UL /* 'TEXT' */);
        if (FSOpen(note_file, vref, &ref) != 0) return;
    }
    LONGINT n = sizeof pages;
    SetFPos(ref, 1, 0);
    FSWrite(ref, &n, pages);
    SetEOF(ref, sizeof pages);
    FSClose(ref);
    FlushVol(0, vref);
}

static void draw_notepad(int w, int h)
{
    gfx_fill(0, 0, w, h, 0);
    gfx_frame(2, 2, w - 2, h - 2);
    /* Bottom multi-sheet lines */
    gfx_hline(4, w - 4, h - 5, 1);
    gfx_hline(6, w - 6, h - 3, 1);

    /* Dog-ear fold in bottom-left corner */
    gfx_line(2, h - 26, 26, h - 5, 1);
    gfx_line(2, h - 26, 26, h - 26, 1);
    gfx_line(26, h - 26, 26, h - 5, 1);

    /* Render current page text */
    const char *s = pages[cur_page];
    int cx = 10, cy = 10;
    while (*s && cy < h - 34) {
        if (*s == '\n' || *s == '\r') {
            cx = 10;
            cy += 12;
        } else {
            gfx_char(cx, cy, (uint8_t)*s, 0);
            cx += 8;
            if (cx > w - 14) { cx = 10; cy += 12; }
        }
        s++;
    }
    /* Cursor bar */
    if (cy < h - 34)
        gfx_fill(cx, cy, cx + 6, cy + 8, 1);

    char pbuf[32];
    snprintf(pbuf, sizeof pbuf, "Page %d of %d (Tab=Turn)", cur_page + 1, NPAGES);
    gfx_text(34, h - 18, pbuf, 0);
}

int main(int argc, char **argv)
{
    load_notes();
    if (argc >= 2) {
        for (int i = 0; i < NPAGES; i++) {
            if (pages[i][0])
                printf("--- Page %d ---\n%s\n", i + 1, pages[i]);
        }
        return 0;
    }

    GWindow win;
    int W = 224, H = 190;
    if (gwin_open(&win, "Note Pad", W, H) == 0) {
        gfx_target(&win.surf);
        draw_notepad(W, H);
        gwin_update(&win);
        GEvent ev;
        while (gwin_event(&win, &ev, 1) >= 0) {
            if (ev.type == GEV_CLOSE) break;
            if (ev.type == GEV_MOUSEDOWN) {
                if (ev.x < 32 && ev.y > H - 32) {
                    cur_page = (cur_page + 1) % NPAGES;
                    draw_notepad(W, H);
                    gwin_update(&win);
                }
            } else if (ev.type == GEV_KEY) {
                int k = ev.key;
                if (k == 27) break;
                if (k == '\t') {
                    cur_page = (cur_page + 1) % NPAGES;
                } else if (k == 8 || k == 127) {
                    size_t l = strlen(pages[cur_page]);
                    if (l > 0) pages[cur_page][l - 1] = 0;
                    save_notes();
                } else if (k == '\r' || (k >= 32 && k < 127)) {
                    size_t l = strlen(pages[cur_page]);
                    if (l + 1 < PAGE_MAX) {
                        pages[cur_page][l] = (k == '\r') ? '\n' : (char)k;
                        pages[cur_page][l + 1] = 0;
                        save_notes();
                    }
                }
                draw_notepad(W, H);
                gwin_update(&win);
            }
        }
        gwin_close(&win);
        return 0;
    }

    /* Full-screen fallback */
    gfx_target(gfx_backbuffer());
    for (;;) {
        gfx_fill(0, 0, 512, 342, 0);
        gfx_clip(144, 60, 144 + W, 60 + H);
        draw_notepad(512, 342);
        gfx_noclip();
        gfx_frame(140, 56, 372, 260);
        gfx_text(144, 40, "PLUSOS NOTE PAD (TAB=NEXT PAGE, ESC=SAVE & EXIT)", 0);
        gfx_present();

        int k = plusos->key_get();
        if (k == 27) break;
        if (k == '\t') cur_page = (cur_page + 1) % NPAGES;
        else if (k == 8 || k == 127) {
            size_t l = strlen(pages[cur_page]);
            if (l > 0) pages[cur_page][l - 1] = 0;
            save_notes();
        } else if (k == '\r' || (k >= 32 && k < 127)) {
            size_t l = strlen(pages[cur_page]);
            if (l + 1 < PAGE_MAX) {
                pages[cur_page][l] = (k == '\r') ? '\n' : (char)k;
                pages[cur_page][l + 1] = 0;
                save_notes();
            }
        }
    }
    return 0;
}
