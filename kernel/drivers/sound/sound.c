/* sound.c - PlusOS Compact Mac Hardware Sound Synthesizer & Volume Driver
 *
 * Hardware PWM sound & VIA volume control specifications:
 *
 *   - Compact Macs (Plus / SE) scan 370 words per video frame (370 * 60.15 Hz
 *     = 22,255 Hz) from the main sound buffer at SoundBase ($0266, located at
 *     MemTop - $0300). The high byte of each 16-bit word is the 8-bit unsigned
 *     PWM sample ($80 = center/silence), while the low byte is disk speed PWM.
 *   - VIA1 ($EFE1FE):
 *       vBufB (+$0000) bit 7 (vSndEnb = $80): 0 = sound enabled, 1 = muted
 *       vBufA (+$1E00) bits 0..2: speaker volume attenuation (0..7)
 *   - Low-memory SdVolume ($0260) stores the system volume (0..7). When volume
 *     is 0, sound_beep() silently flashes the menu bar (top 20 scanlines),
 *     matching standard Mac OS alert behavior.
 */
#include "kernel.h"
#include "proc.h"
#include <plusos/syscall.h>

#define VIA_BASE        ((volatile uint8_t *)0xEFE1FEUL)
#define VIA_BUFB        (VIA_BASE + 0x0000)
#define VIA_BUFA        (VIA_BASE + 0x1E00)
#define SND_SAMPLES     370
#define SND_SAMPLE_RATE 22255UL

static volatile int      snd_ticks_left;
static volatile uint32_t snd_phase;
static volatile uint32_t snd_step;

static void fill_sound_buffer(uint32_t step)
{
    uint16_t *sb = (uint16_t *)LM_LONG(0x266);      /* SoundBase */
    if (!sb) return;
    uint32_t ph = snd_phase;
    for (int i = 0; i < SND_SAMPLES; i++) {
        uint8_t sample;
        if (step == 0) {
            sample = 0x80;                          /* silence */
        } else {
            /* Rich square + harmonic waveform for clear classic Mac tone */
            uint8_t p = (uint8_t)(ph >> 8);
            sample = (p & 0x80) ? ((p & 0x40) ? 0xD8 : 0xC0)
                                : ((p & 0x40) ? 0x28 : 0x40);
            ph += step;
        }
        /* Preserve low byte (Sony floppy speed control) and write high byte */
        sb[i] = (uint16_t)(((uint16_t)sample << 8) | (sb[i] & 0x00FF));
    }
    snd_phase = ph;
}

void sound_init(void)
{
    uint8_t vol = LM_BYTE(0x260) & 7;               /* SdVolume */
    if (vol == 0) {
        vol = 5;
        LM_BYTE(0x260) = vol;
    }
    snd_ticks_left = 0;
    snd_step = 0;
    if (!g_hw.has_classic_sound) return;            /* ASC/SoundMgr Macs do not use VIA1 PWM buffer */
    *VIA_BUFA = (*VIA_BUFA & ~0x07) | vol;
    *VIA_BUFB |= 0x80;                              /* mute PWM until a tone plays */
    fill_sound_buffer(0);
}

int sound_get_volume(void)
{
    return LM_BYTE(0x260) & 7;
}

void sound_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 7) vol = 7;
    LM_BYTE(0x260) = (uint8_t)vol;
    if (vol > 0 && g_hw.has_classic_sound)
        *VIA_BUFA = (uint8_t)((*VIA_BUFA & ~0x07) | (vol & 7));
}

/* Called at 60 Hz from tick_c() in tick.c */
void sound_vbl_tick(void)
{
    if (!g_hw.has_classic_sound) return;
    if (snd_ticks_left > 0) {
        fill_sound_buffer(snd_step);
        if (--snd_ticks_left == 0) {
            snd_step = 0;
            fill_sound_buffer(0);
            *VIA_BUFB |= 0x80;                      /* disable sound generator */
        }
    }
}

static void flash_menubar(void)
{
    uint32_t *scr = (uint32_t *)video_front();
    if (!scr) scr = (uint32_t *)LM_LONG(0x824);
    if (!scr) return;
    int longs = (20 * video_rowbytes()) / 4;
    if (longs <= 0) longs = 320;
    for (int i = 0; i < longs; i++) scr[i] ^= 0xFFFFFFFFUL;
    extern volatile int sched_active;
    if (sched_active) proc_sleep_ticks(4);
    else { for (volatile int d = 0; d < 25000; d++) {} }
    for (int i = 0; i < longs; i++) scr[i] ^= 0xFFFFFFFFUL;
}

void sound_tone(int freq_hz, int duration_ticks)
{
    if (duration_ticks <= 0) return;
    if (sound_get_volume() == 0) {
        flash_menubar();
        return;
    }
    if (!g_hw.has_classic_sound) {
        if (freq_hz > 20) SysBeep((INTEGER)duration_ticks);
        extern volatile int sched_active;
        if (sched_active) proc_sleep_ticks(duration_ticks);
        return;
    }
    if (freq_hz <= 20) {
        /* Rest (silence) for duration_ticks */
        uint16_t sr = int_disable();
        snd_step = 0;
        snd_ticks_left = duration_ticks;
        fill_sound_buffer(0);
        *VIA_BUFB |= 0x80;
        int_restore(sr);
    } else {
        uint32_t step = ((uint32_t)freq_hz << 16) / SND_SAMPLE_RATE;
        uint16_t sr = int_disable();
        snd_step = step;
        snd_ticks_left = duration_ticks;
        fill_sound_buffer(step);
        *VIA_BUFB &= (uint8_t)~0x80;                /* enable VIA1 sound output */
        int_restore(sr);
    }
    extern volatile int sched_active;
    if (sched_active)
        proc_sleep_ticks(duration_ticks);
}

void sound_beep(int duration_ticks)
{
    if (duration_ticks <= 0) duration_ticks = 10;
    if (sound_get_volume() == 0) {
        flash_menubar();
        return;
    }
    sound_tone(880, duration_ticks);
}

void sound_play_notes(const uint16_t *notes, int count)
{
    if (!notes || count <= 0) return;
    for (int i = 0; i < count; i++) {
        uint16_t freq = notes[i * 2];
        uint16_t dur  = notes[i * 2 + 1];
        if (dur > 0) sound_tone(freq, dur);
    }
}

long sound_syscall(struct plusos_sound_args *a)
{
    if (!a) return -1;
    switch (a->op) {
    case PLUSOS_SND_BEEP:
        sound_beep(a->duration_ticks);
        return 0;
    case PLUSOS_SND_TONE:
        sound_tone(a->freq_hz, a->duration_ticks);
        return 0;
    case PLUSOS_SND_VOL_GET:
        return sound_get_volume();
    case PLUSOS_SND_VOL_SET:
        sound_set_volume(a->volume);
        return sound_get_volume();
    case PLUSOS_SND_NOTES:
        sound_play_notes(a->notes, a->count);
        return 0;
    }
    return -1;
}
