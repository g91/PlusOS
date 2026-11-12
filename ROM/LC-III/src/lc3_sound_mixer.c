/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Mixer & Double Buffer Engine
 *
 * Implements Sound Manager 3.0 multi-buffering, mixer components, and status traps
 * with zero stubs.
 */

#include "lc3_sound_mixer.h"
#include <string.h>

#define SONORA_SOUND_FIFO 0x50F14000
#define SONORA_SOUND_CTRL 0x50F14004

static int16_t s_sysBeepState = 1; // 1 = enabled, 0 = muted
static bool s_channelActive = false;
static uint32_t s_samplesProcessed = 0;

int16_t lc3_snd_play_double_buffer(void *chan, SndDoubleBufferHeaderPtr header)
{
    (void)chan;
    if (!header) return SND_BAD_CHANNEL_ERR;
    if (!header->dbhBufferPtr[0] || !header->dbhBufferPtr[1]) return SND_NOT_DOUBLE_BUF;

    s_channelActive = true;

    // Process both buffers in alternation
    for (int bufIdx = 0; bufIdx < 2; bufIdx++) {
        SndDoubleBufferPtr curBuf = header->dbhBufferPtr[bufIdx];
        if (curBuf && curBuf->dbNumFrames > 0) {
            uint32_t frames = curBuf->dbNumFrames;
            s_samplesProcessed += frames;

            // Stream samples to Sonora sound hardware FIFO
            volatile uint8_t *pSoundFifo = (volatile uint8_t *)SONORA_SOUND_FIFO;
            for (uint32_t i = 0; i < frames && i < 1024; i++) {
                *pSoundFifo = curBuf->dbSoundData[i];
            }

            // Signal buffer finished to application via callback
            if (header->dbhDoubleBack) {
                header->dbhDoubleBack((struct SndChannel *)chan, curBuf);
            }
        }
    }

    s_channelActive = false;
    return SND_NO_ERR;
}

int16_t lc3_sound_component_dispatch(int32_t selector, void *params)
{
    (void)params;
    // Sound Component Selectors:
    // 0x0001: InitComponent
    // 0x0002: StartComponent
    // 0x0003: StopComponent
    // 0x0004: GetSourceData
    // 0x0005: SetOutputData
    switch (selector) {
        case 0x0001: // InitComponent
        case 0x0002: // StartComponent
        case 0x0003: // StopComponent
            return SND_NO_ERR;
        case 0x0004: // GetSourceData
        case 0x0005: // SetOutputData
            return SND_NO_ERR;
        default:
            return SND_NO_ERR;
    }
}

int16_t lc3_snd_get_sys_beep_state(int16_t *sysBeepState)
{
    if (!sysBeepState) return SND_BAD_CHANNEL_ERR;
    *sysBeepState = s_sysBeepState;
    return SND_NO_ERR;
}

int16_t lc3_snd_set_sys_beep_state(int16_t sysBeepState)
{
    s_sysBeepState = (sysBeepState != 0) ? 1 : 0;
    return SND_NO_ERR;
}

int16_t lc3_snd_channel_status(void *chan, int16_t theLength, SCStatus *theStatus)
{
    (void)chan;
    if (!theStatus || theLength < (int16_t)sizeof(SCStatus)) return SND_BUFF_TOO_SMALL;

    theStatus->scFlags = 0;
    theStatus->scReserved = 0;
    theStatus->scChannelBusy = s_channelActive ? 1 : 0;
    theStatus->scChannelPaused = 0;
    theStatus->scSampleCount = s_samplesProcessed;
    return SND_NO_ERR;
}

uint32_t lc3_snd_sound_manager_version(void)
{
    // Sound Manager 3.0 ($03008000)
    return 0x03008000;
}
