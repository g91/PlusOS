/* sound.c - PlusOS Compact Mac Hardware Sound Synthesizer & Piano Utility
 *
 * Usage:
 *   sound                   Interactive keyboard synthesizer (A..K = piano keys)
 *   sound vol [0..7]        Get or set hardware speaker volume (0 = menu flash)
 *   sound tone <hz> [ticks] Play a synthesized frequency
 *   sound chime             Play classic Mac startup chord
 *   sound song              Play Ode to Joy melody
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void play_tone(int hz, int ticks)
{
    struct plusos_sound_args a = { PLUSOS_SND_TONE, hz, ticks, 0, 0, 0 };
    plusos_syscall(SYS_sound, &a);
}

static int get_vol(void)
{
    struct plusos_sound_args a = { PLUSOS_SND_VOL_GET, 0, 0, 0, 0, 0 };
    return (int)plusos_syscall(SYS_sound, &a);
}

static int set_vol(int v)
{
    struct plusos_sound_args a = { PLUSOS_SND_VOL_SET, 0, 0, v, 0, 0 };
    return (int)plusos_syscall(SYS_sound, &a);
}

static void play_notes(const uint16_t *notes, int count)
{
    struct plusos_sound_args a = { PLUSOS_SND_NOTES, 0, 0, 0, notes, count };
    plusos_syscall(SYS_sound, &a);
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        if (!strcasecmp(argv[1], "vol") || !strcasecmp(argv[1], "volume")) {
            if (argc >= 3) {
                int v = set_vol(atoi(argv[2]));
                printf("Speaker volume set to %d/7%s\n", v,
                       v == 0 ? " (silent menu-bar flash)" : "");
            } else {
                int v = get_vol();
                printf("Speaker volume: %d/7 (use 'sound vol 0..7' to change)\n", v);
            }
            return 0;
        }
        if (!strcasecmp(argv[1], "tone") && argc >= 3) {
            int hz = atoi(argv[2]);
            int ticks = argc >= 4 ? atoi(argv[3]) : 20;
            play_tone(hz, ticks);
            return 0;
        }
        if (!strcasecmp(argv[1], "beep")) {
            struct plusos_sound_args a = { PLUSOS_SND_BEEP, 0, 12, 0, 0, 0 };
            plusos_syscall(SYS_sound, &a);
            return 0;
        }
        if (!strcasecmp(argv[1], "chime")) {
            static const uint16_t chime[] = {
                523, 8,  659, 8,  784, 8,  1047, 24
            };
            play_notes(chime, 4);
            return 0;
        }
        if (!strcasecmp(argv[1], "song")) {
            static const uint16_t ode[] = {
                330, 12, 330, 12, 349, 12, 392, 12,
                392, 12, 349, 12, 330, 12, 294, 12,
                262, 12, 262, 12, 294, 12, 330, 12,
                330, 18, 294,  6, 294, 24
            };
            play_notes(ode, 15);
            return 0;
        }
    }

    printf("PlusOS 22.25 kHz PWM Sound Synthesizer (Volume: %d/7)\n", get_vol());
    printf("  Keys A S D F G H J K = C4 D4 E4 F4 G4 A4 B4 C5\n");
    printf("  Keys 0..7 = Set Volume   M = Play Melody   Q = Quit\n");
    for (;;) {
        int c = plusos->key_get();
        if (c == 'q' || c == 'Q' || c == 27) break;
        if (c >= '0' && c <= '7') {
            int v = set_vol(c - '0');
            printf("  Volume: %d/7\n", v);
            continue;
        }
        switch (c) {
        case 'a': case 'A': play_tone(262, 8); break;
        case 's': case 'S': play_tone(294, 8); break;
        case 'd': case 'D': play_tone(330, 8); break;
        case 'f': case 'F': play_tone(349, 8); break;
        case 'g': case 'G': play_tone(392, 8); break;
        case 'h': case 'H': play_tone(440, 8); break;
        case 'j': case 'J': play_tone(494, 8); break;
        case 'k': case 'K': play_tone(523, 8); break;
        case 'm': case 'M': {
            static const uint16_t arp[] = { 262, 6, 330, 6, 392, 6, 523, 12 };
            play_notes(arp, 4);
            break;
        }
        }
    }
    return 0;
}
