/* keytrans.S - key translation procedure for the ROM keyboard driver.
 *
 * The Plus ROM's keyboard driver asks Key1Trans (main
 * keyboard) / Key2Trans (keypad) to turn a key code into a character, then
 * posts the keyDown/autoKey event. The ROM clears both pointers at start-up
 * and normally the System file installs them; PlusOS installs this instead.
 *   in:  D2 = virtual key code (keypad codes are +$40), D1 = modifier word
 *        (KeyMap bytes 6-7: $8000 Command, $0001 Shift, $0002 Caps, $0004 Option)
 *   out: D0 = character (0 = none). Called from the keyboard interrupt, so
 *        everything except D0 is preserved.
 */
        .text
        .globl  plusos_key_trans
plusos_key_trans:
        movem.l %d1-%d7/%a0-%a6, -(%sp)
        moveq   #0, %d0
        move.w  %d1, %d0
        move.l  %d0, -(%sp)                 /* mods */
        moveq   #0x7F, %d0
        and.l   %d2, %d0
        move.l  %d0, -(%sp)                 /* key code */
        jsr     kbd_translate
        addq.l  #8, %sp
        movem.l (%sp)+, %d1-%d7/%a0-%a6
        rts
