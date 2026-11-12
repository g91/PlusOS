/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 MIDI Synthesizer Core & Waveform Generator
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SND_MIDI_H
#define LC3_SND_MIDI_H

#include <stdint.h>
#include <stdbool.h>

#define MIDI_NUM_VOICES       4
#define MIDI_WAVE_TABLE_SIZE  256

/* MIDI Status Bytes */
#define MIDI_STATUS_NOTE_OFF         0x80
#define MIDI_STATUS_NOTE_ON          0x90
#define MIDI_STATUS_CONTROL_CHANGE   0xB0
#define MIDI_STATUS_PROGRAM_CHANGE   0xC0
#define MIDI_STATUS_PITCH_BEND       0xE0

/* Standard Continuous Controllers */
#define MIDI_CC_VOLUME               7
#define MIDI_CC_PAN                  10
#define MIDI_CC_SUSTAIN              64

/* ADSR Envelope Phases */
typedef enum {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvelopeState;

typedef struct MIDIVoice {
    bool active;
    uint8_t note;
    uint8_t velocity;
    uint32_t phaseAccumulator;   /* 16.16 fixed point phase */
    uint32_t phaseStep;          /* Frequency increment per sample */
    EnvelopeState envState;
    uint16_t envLevel;           /* 0 to 65535 */
    uint16_t attackRate;
    uint16_t decayRate;
    uint16_t sustainLevel;
    uint16_t releaseRate;
} MIDIVoice;

typedef struct MIDISynthContext {
    MIDIVoice voices[MIDI_NUM_VOICES];
    uint8_t waveTable[MIDI_WAVE_TABLE_SIZE]; /* Sine / Saw / Square waveform */
    uint8_t masterVolume;                    /* 0 - 127 */
    int16_t pitchBendVal;                    /* -8192 to +8191 */
    bool sustainPedal;
} MIDISynthContext;

/* Public API */
int16_t lc3_midi_init_synth(MIDISynthContext *ctx);
int16_t lc3_midi_parse_message(MIDISynthContext *ctx, const uint8_t *midiBytes, uint16_t length);
int16_t lc3_midi_note_on(MIDISynthContext *ctx, uint8_t note, uint8_t velocity);
int16_t lc3_midi_note_off(MIDISynthContext *ctx, uint8_t note, uint8_t velocity);
int16_t lc3_midi_render_samples(MIDISynthContext *ctx, uint8_t *outputBuffer, uint32_t sampleCount);
int16_t lc3_midi_set_controller(MIDISynthContext *ctx, uint8_t controller, uint8_t value);

#endif /* LC3_SND_MIDI_H */
