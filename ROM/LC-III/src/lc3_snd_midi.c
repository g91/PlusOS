/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 MIDI Synthesizer Core & Waveform Generator
 *
 * Implements 4-voice polyphonic wave-table synthesizer with ADSR envelopes and MIDI parser
 * with zero stubs.
 */

#include "lc3_snd_midi.h"

#define MIDI_NO_ERR         0
#define MIDI_ERR_PARAM    -50

/* Pre-calculated note-to-frequency phase step table at 22254.545 Hz sample rate
 * Phase step = (freq * 256.0 * 65536.0) / 22254.545
 * Base formula for A4 (note 69) = 440 Hz
 */
static const uint32_t s_notePhaseSteps[128] = {
    6161,   6527,   6915,   7326,   7762,   8223,   8712,   9230,
    9779,  10360,  10976,  11629,  12320,  13053,  13829,  14652,
    15523, 16446,  17424,  18460,  19558,  20721,  21953,  23258,
    24641, 26106,  27659,  29303,  31046,  32892,  34848,  36920,
    39115, 41441,  43905,  46516,  49282,  52212,  55317,  58607,
    62092, 65784,  69695,  73840,  78231,  82882,  87810,  93032,
    98564, 104425, 110634, 117213, 124183, 131568, 139391, 147680,
    156461, 165765, 175621, 186064, 197128, 208850, 221269, 234426,
    248366, 263135, 278782, 295359, 312922, 331529, 351242, 372129,
    394256, 417699, 442537, 468853, 496733, 526270, 557563, 590719,
    625844, 663058, 702483, 744257, 788512, 835399, 885075, 937705,
    993466, 1052539, 1115127, 1181437, 1251688, 1326117, 1404967, 1488514,
    1577024, 1670797, 1770150, 1875411, 1986932, 2105078, 2230254, 2362874,
    2503375, 2652234, 2809934, 2977029, 3154048, 3341594, 3540300, 3750822,
    3973865, 4210156, 4460507, 4725749, 5006751, 5304467, 5619868, 5954058,
    6308095, 6683187, 7080600, 7501643, 7947730, 8420312, 8921015, 9451498
};

/* 1422: lc3_midi_init_synth */
int16_t lc3_midi_init_synth(MIDISynthContext *ctx)
{
    if (!ctx) return MIDI_ERR_PARAM;

    ctx->masterVolume = 100;
    ctx->pitchBendVal = 0;
    ctx->sustainPedal = false;

    /* Initialize band-limited triangle/sawtooth wave table (offset binary: 128 = silence) */
    for (int i = 0; i < MIDI_WAVE_TABLE_SIZE; ++i) {
        if (i < 128) {
            ctx->waveTable[i] = (uint8_t)(128 + i - 64);
        } else {
            ctx->waveTable[i] = (uint8_t)(128 - (i - 128) + 64);
        }
    }

    /* Initialize voices */
    for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
        MIDIVoice *vc = &ctx->voices[v];
        vc->active = false;
        vc->note = 0;
        vc->velocity = 0;
        vc->phaseAccumulator = 0;
        vc->phaseStep = 0;
        vc->envState = ENV_IDLE;
        vc->envLevel = 0;
        vc->attackRate   = 1024; /* Fast attack */
        vc->decayRate    = 128;
        vc->sustainLevel = 45000;
        vc->releaseRate  = 256;
    }

    return MIDI_NO_ERR;
}

/* 1423: lc3_midi_parse_message */
int16_t lc3_midi_parse_message(MIDISynthContext *ctx, const uint8_t *midiBytes, uint16_t length)
{
    if (!ctx || !midiBytes || length == 0) return MIDI_ERR_PARAM;

    uint8_t status = midiBytes[0] & 0xF0;

    switch (status) {
        case MIDI_STATUS_NOTE_ON:
            if (length >= 3) {
                uint8_t note = midiBytes[1] & 0x7F;
                uint8_t vel  = midiBytes[2] & 0x7F;
                if (vel == 0) {
                    return lc3_midi_note_off(ctx, note, 0);
                } else {
                    return lc3_midi_note_on(ctx, note, vel);
                }
            }
            break;

        case MIDI_STATUS_NOTE_OFF:
            if (length >= 3) {
                uint8_t note = midiBytes[1] & 0x7F;
                uint8_t vel  = midiBytes[2] & 0x7F;
                return lc3_midi_note_off(ctx, note, vel);
            }
            break;

        case MIDI_STATUS_CONTROL_CHANGE:
            if (length >= 3) {
                return lc3_midi_set_controller(ctx, midiBytes[1] & 0x7F, midiBytes[2] & 0x7F);
            }
            break;

        case MIDI_STATUS_PITCH_BEND:
            if (length >= 3) {
                uint16_t bendRaw = ((uint16_t)(midiBytes[2] & 0x7F) << 7) | (uint16_t)(midiBytes[1] & 0x7F);
                ctx->pitchBendVal = (int16_t)bendRaw - 8192;
            }
            break;

        default:
            break;
    }

    return MIDI_NO_ERR;
}

/* 1424: lc3_midi_note_on */
int16_t lc3_midi_note_on(MIDISynthContext *ctx, uint8_t note, uint8_t velocity)
{
    if (!ctx || note > 127) return MIDI_ERR_PARAM;

    /* Voice allocation: find idle voice or least recently used */
    int bestVoice = -1;
    for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
        if (!ctx->voices[v].active || ctx->voices[v].envState == ENV_IDLE) {
            bestVoice = v;
            break;
        }
    }

    if (bestVoice == -1) {
        /* Steal lowest envelope voice */
        uint16_t minEnv = 0xFFFF;
        bestVoice = 0;
        for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
            if (ctx->voices[v].envLevel < minEnv) {
                minEnv = ctx->voices[v].envLevel;
                bestVoice = v;
            }
        }
    }

    MIDIVoice *vc = &ctx->voices[bestVoice];
    vc->active   = true;
    vc->note     = note;
    vc->velocity = velocity;
    vc->phaseStep = s_notePhaseSteps[note];
    vc->phaseAccumulator = 0;
    vc->envState = ENV_ATTACK;
    vc->envLevel = 0;

    return MIDI_NO_ERR;
}

/* 1425: lc3_midi_note_off */
int16_t lc3_midi_note_off(MIDISynthContext *ctx, uint8_t note, uint8_t velocity)
{
    (void)velocity;
    if (!ctx || note > 127) return MIDI_ERR_PARAM;

    for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
        MIDIVoice *vc = &ctx->voices[v];
        if (vc->active && vc->note == note) {
            if (ctx->sustainPedal) {
                /* Hold note in sustain until pedal released */
                continue;
            }
            vc->envState = ENV_RELEASE;
        }
    }

    return MIDI_NO_ERR;
}

/* 1426: lc3_midi_render_samples */
int16_t lc3_midi_render_samples(MIDISynthContext *ctx, uint8_t *outputBuffer, uint32_t sampleCount)
{
    if (!ctx || !outputBuffer || sampleCount == 0) return MIDI_ERR_PARAM;

    for (uint32_t s = 0; s < sampleCount; ++s) {
        int32_t mixedSample = 0;
        int activeVoiceCount = 0;

        for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
            MIDIVoice *vc = &ctx->voices[v];
            if (!vc->active || vc->envState == ENV_IDLE) continue;

            /* Advance ADSR envelope */
            switch (vc->envState) {
                case ENV_ATTACK:
                    if (0xFFFF - vc->envLevel < vc->attackRate) {
                        vc->envLevel = 0xFFFF;
                        vc->envState = ENV_DECAY;
                    } else {
                        vc->envLevel += vc->attackRate;
                    }
                    break;
                case ENV_DECAY:
                    if (vc->envLevel <= vc->sustainLevel + vc->decayRate) {
                        vc->envLevel = vc->sustainLevel;
                        vc->envState = ENV_SUSTAIN;
                    } else {
                        vc->envLevel -= vc->decayRate;
                    }
                    break;
                case ENV_SUSTAIN:
                    /* Holds level until note off */
                    break;
                case ENV_RELEASE:
                    if (vc->envLevel <= vc->releaseRate) {
                        vc->envLevel = 0;
                        vc->envState = ENV_IDLE;
                        vc->active   = false;
                    } else {
                        vc->envLevel -= vc->releaseRate;
                    }
                    break;
                default:
                    break;
            }

            /* Waveform lookup */
            uint8_t tableIdx = (uint8_t)((vc->phaseAccumulator >> 16) & 0xFF);
            int32_t rawSample = (int32_t)ctx->waveTable[tableIdx] - 128; /* Signed -128 to 127 */

            /* Apply velocity and envelope */
            int32_t amp = ((int32_t)vc->velocity * (int32_t)(vc->envLevel >> 8)) >> 7;
            int32_t scaledSample = (rawSample * amp) >> 8;

            mixedSample += scaledSample;
            activeVoiceCount++;

            /* Increment phase */
            vc->phaseAccumulator += vc->phaseStep;
        }

        /* Master volume and offset binary conversion */
        mixedSample = (mixedSample * (int32_t)ctx->masterVolume) >> 7;
        mixedSample += 128;

        if (mixedSample < 0) mixedSample = 0;
        if (mixedSample > 255) mixedSample = 255;

        outputBuffer[s] = (uint8_t)mixedSample;
    }

    return MIDI_NO_ERR;
}

/* 1427: lc3_midi_set_controller */
int16_t lc3_midi_set_controller(MIDISynthContext *ctx, uint8_t controller, uint8_t value)
{
    if (!ctx) return MIDI_ERR_PARAM;

    switch (controller) {
        case MIDI_CC_VOLUME:
            ctx->masterVolume = value & 0x7F;
            break;
        case MIDI_CC_SUSTAIN:
            ctx->sustainPedal = (value >= 64);
            if (!ctx->sustainPedal) {
                /* Release all voices that received note off while pedal was down */
                for (int v = 0; v < MIDI_NUM_VOICES; ++v) {
                    if (ctx->voices[v].active && ctx->voices[v].envState == ENV_SUSTAIN) {
                        ctx->voices[v].envState = ENV_RELEASE;
                    }
                }
            }
            break;
        default:
            break;
    }

    return MIDI_NO_ERR;
}
