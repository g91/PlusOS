/* keymap.c - the complete PlusOS keyboard map.
 *
 * Index = virtual key code as the Plus ROM's keyboard driver hands it to the
 * key translation procedure: the key's code from the keyboard, plus $40 for
 * keys sent with the keypad prefix ($79). One table covers every keyboard
 * that can be attached to a Mac Plus:
 *
 *   M0110A  Mac Plus keyboard (built-in keypad and arrow keys)
 *   M0110   original 128K/512K keyboard (has an Enter key next to Space)
 *   M0120   separate numeric keypad for the M0110
 *   extended/ADB numbering, as sent by Mini vMac and by many keyboard
 *           adapters (keypad * + / = on their own codes, arrows at $7B-$7E,
 *           Esc, Control, Home/End/PageUp/PageDown, forward Delete, F-keys)
 *
 * Each entry: character without Shift, character with Shift. Caps Lock only
 * affects letters. Special keys return the standard Mac character codes.
 * 0 = the key produces no character (modifiers, unknown codes).
 */
#include "kernel.h"

/* Mac character codes for special keys (Inside Macintosh / Mac Roman) */
#define C_HOME   0x01
#define C_ENTER  0x03
#define C_END    0x04
#define C_HELP   0x05
#define C_BS     0x08
#define C_TAB    0x09
#define C_PGUP   0x0B
#define C_PGDN   0x0C
#define C_RET    0x0D
#define C_FKEY   0x10    /* all function keys */
#define C_ESC    0x1B    /* also keypad Clear */
#define C_LEFT   0x1C
#define C_RIGHT  0x1D
#define C_UP     0x1E
#define C_DOWN   0x1F
#define C_DEL    0x7F    /* forward delete */

typedef struct { uint8_t plain, shift; } KeyDef;

static const KeyDef keymap[128] = {
    /* ---- main keyboard: identical on M0110, M0110A and later keyboards ---- */
    [0x00] = {'a', 'A'},  [0x01] = {'s', 'S'},  [0x02] = {'d', 'D'},  [0x03] = {'f', 'F'},
    [0x04] = {'h', 'H'},  [0x05] = {'g', 'G'},  [0x06] = {'z', 'Z'},  [0x07] = {'x', 'X'},
    [0x08] = {'c', 'C'},  [0x09] = {'v', 'V'},  [0x0A] = {'`', '~'},  /* ISO key left of 1 */
    [0x0B] = {'b', 'B'},  [0x0C] = {'q', 'Q'},  [0x0D] = {'w', 'W'},  [0x0E] = {'e', 'E'},
    [0x0F] = {'r', 'R'},  [0x10] = {'y', 'Y'},  [0x11] = {'t', 'T'},  [0x12] = {'1', '!'},
    [0x13] = {'2', '@'},  [0x14] = {'3', '#'},  [0x15] = {'4', '$'},  [0x16] = {'6', '^'},
    [0x17] = {'5', '%'},  [0x18] = {'=', '+'},  [0x19] = {'9', '('},  [0x1A] = {'7', '&'},
    [0x1B] = {'-', '_'},  [0x1C] = {'8', '*'},  [0x1D] = {'0', ')'},  [0x1E] = {']', '}'},
    [0x1F] = {'o', 'O'},  [0x20] = {'u', 'U'},  [0x21] = {'[', '{'},  [0x22] = {'i', 'I'},
    [0x23] = {'p', 'P'},  [0x24] = {C_RET, C_RET},                    /* Return */
    [0x25] = {'l', 'L'},  [0x26] = {'j', 'J'},  [0x27] = {'\'', '"'}, [0x28] = {'k', 'K'},
    [0x29] = {';', ':'},  [0x2A] = {'\\', '|'}, [0x2B] = {',', '<'},  [0x2C] = {'/', '?'},
    [0x2D] = {'n', 'N'},  [0x2E] = {'m', 'M'},  [0x2F] = {'.', '>'},
    [0x30] = {C_TAB, C_TAB},                                          /* Tab */
    [0x31] = {' ', ' '},                                              /* Space */
    [0x32] = {'`', '~'},                                              /* ` ~ */
    [0x33] = {C_BS, C_BS},                                            /* Backspace (Delete) */
    [0x34] = {C_ENTER, C_ENTER},                                      /* M0110 Enter (by Space) */
    [0x35] = {C_ESC, C_ESC},                                          /* Esc (extended) */
    /* 0x36 Control(R) 0x37 Command 0x38 Shift 0x39 Caps Lock 0x3A Option
       0x3B Control 0x3C-0x3E right Shift/Option/Control 0x3F fn: modifiers, no char */

    /* ---- keypad: M0110A / M0120 codes ($40 + code after the $79 prefix) ---- */
    [0x41] = {'.', '.'},                                              /* keypad . */
    [0x42] = {C_RIGHT, '*'},      /* M0110A: Right arrow; keypad * comes as Shift+this */
    [0x43] = {'*', '*'},          /* keypad * (extended numbering) */
    [0x45] = {'+', '+'},          /* keypad + (extended numbering) */
    [0x46] = {C_LEFT, '+'},       /* M0110A: Left arrow; keypad + = Shift+this */
    [0x47] = {C_ESC, C_ESC},      /* keypad Clear */
    [0x48] = {C_DOWN, '='},       /* M0110A: Down arrow; keypad = = Shift+this */
    [0x4B] = {'/', '/'},          /* keypad / (extended numbering) */
    [0x4C] = {C_ENTER, C_ENTER},  /* keypad Enter */
    [0x4D] = {C_UP, '/'},         /* M0110A: Up arrow; keypad / = Shift+this */
    [0x4E] = {'-', '-'},          /* keypad - */
    [0x51] = {'=', '='},          /* keypad = (extended numbering) */
    [0x52] = {'0', '0'},  [0x53] = {'1', '1'},  [0x54] = {'2', '2'},  [0x55] = {'3', '3'},
    [0x56] = {'4', '4'},  [0x57] = {'5', '5'},  [0x58] = {'6', '6'},  [0x59] = {'7', '7'},
    [0x5B] = {'8', '8'},  [0x5C] = {'9', '9'},

    /* ---- extended keyboards / adapters (ADB numbering) ---- */
    [0x60] = {C_FKEY, C_FKEY},    /* F5 */
    [0x61] = {C_FKEY, C_FKEY},    /* F6 */
    [0x62] = {C_FKEY, C_FKEY},    /* F7 */
    [0x63] = {C_FKEY, C_FKEY},    /* F3 */
    [0x64] = {C_FKEY, C_FKEY},    /* F8 */
    [0x65] = {C_FKEY, C_FKEY},    /* F9 */
    [0x67] = {C_FKEY, C_FKEY},    /* F11 */
    [0x69] = {C_FKEY, C_FKEY},    /* F13 */
    [0x6B] = {C_FKEY, C_FKEY},    /* F14 */
    [0x6D] = {C_FKEY, C_FKEY},    /* F10 */
    [0x6F] = {C_FKEY, C_FKEY},    /* F12 */
    [0x71] = {C_FKEY, C_FKEY},    /* F15 */
    [0x72] = {C_HELP, C_HELP},    /* Help / Insert */
    [0x73] = {C_HOME, C_HOME},    /* Home */
    [0x74] = {C_PGUP, C_PGUP},    /* Page Up */
    [0x75] = {C_DEL, C_DEL},      /* forward Delete */
    [0x76] = {C_FKEY, C_FKEY},    /* F4 */
    [0x77] = {C_END, C_END},      /* End */
    [0x78] = {C_FKEY, C_FKEY},    /* F2 */
    [0x79] = {C_PGDN, C_PGDN},    /* Page Down */
    [0x7A] = {C_FKEY, C_FKEY},    /* F1 */
    [0x7B] = {C_LEFT, C_LEFT},    /* Left arrow */
    [0x7C] = {C_RIGHT, C_RIGHT},  /* Right arrow */
    [0x7D] = {C_DOWN, C_DOWN},    /* Down arrow */
    [0x7E] = {C_UP, C_UP},        /* Up arrow */
};

/* modword = KeyMap bytes 6-7 as the ROM passes it: $8000 Command (key $37),
 * $0001 Shift ($38), $0002 Caps Lock ($39), $0004 Option ($3A),
 * $0008 Control ($3B), $0010-$0040 right-hand Shift/Option/Control. */
int keymap_translate(int code, int modword)
{
    if (code < 0 || code > 0x7F) return 0;
    int shift = (modword & (0x0001 | 0x0010)) != 0;
    int caps = (modword & 0x0002) != 0;
    int c = shift ? keymap[code].shift : keymap[code].plain;
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    return c;
}
