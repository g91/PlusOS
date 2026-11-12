/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Mixer & Double Buffer Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SOUND_MIXER_H
#define LC3_SOUND_MIXER_H

#include <stdint.h>
#include <stdbool.h>

struct SndChannel;

#define SND_NO_ERR              0
#define SND_BAD_CHANNEL_ERR  -205
#define SND_BUFF_TOO_SMALL   -210
#define SND_NOT_DOUBLE_BUF   -212

typedef struct SndDoubleBuffer {
    uint32_t dbNumFrames;
    uint32_t dbFlags;
    uint32_t dbUserInfo[2];
    uint8_t  dbSoundData[1];
} SndDoubleBuffer, *SndDoubleBufferPtr;

typedef struct SndDoubleBufferHeader {
    int16_t  dbhNumChannels;
    int16_t  dbhSampleSize;
    int16_t  dbhCompressionID;
    int16_t  dbhPacketSize;
    uint32_t dbhSampleRate;
    SndDoubleBufferPtr dbhBufferPtr[2];
    void (*dbhDoubleBack)(struct SndChannel *chan, SndDoubleBufferPtr doubleBuffer);
} SndDoubleBufferHeader, *SndDoubleBufferHeaderPtr;

typedef struct SCStatus {
    uint32_t scFlags;
    uint32_t scReserved;
    uint32_t scChannelBusy;
    uint32_t scChannelPaused;
    uint32_t scSampleCount;
} SCStatus;

/* Sound Manager 3.0 API */
int16_t lc3_snd_play_double_buffer(void *chan, SndDoubleBufferHeaderPtr header);
int16_t lc3_sound_component_dispatch(int32_t selector, void *params);
int16_t lc3_snd_get_sys_beep_state(int16_t *sysBeepState);
int16_t lc3_snd_set_sys_beep_state(int16_t sysBeepState);
int16_t lc3_snd_channel_status(void *chan, int16_t theLength, SCStatus *theStatus);
uint32_t lc3_snd_sound_manager_version(void);

#endif /* LC3_SOUND_MIXER_H */
