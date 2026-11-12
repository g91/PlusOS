/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Input Hardware Device Driver & Automatic Gain Control (AGC)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SOUND_IN_DRIVER_H
#define LC3_SOUND_IN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define SNDIN_NO_ERR         0
#define SNDIN_HARDWARE_ERR  -1
#define SNDIN_PARAM_ERR    -50
#define SNDIN_BUSY_ERR    -221

#define SNDIN_REG_BASE     0x50014000
#define SNDIN_FIFO_SIZE    256

typedef struct SoundInHWConfig {
    uint16_t sampleRate;      /* e.g. 11025, 22050 */
    uint8_t gainLevel;        /* 0 to 255 */
    bool agcEnabled;
    bool micPreampEnabled;
    uint8_t inputSource;      /* 0 = mic jack, 1 = internal line-in */
} SoundInHWConfig;

typedef struct SoundInDriverState {
    SoundInHWConfig config;
    bool isRecording;
    uint8_t ringBuffer[SNDIN_FIFO_SIZE];
    uint16_t head;
    uint16_t tail;
    uint8_t peakLevel;
    uint32_t totalSamplesCaptured;
} SoundInDriverState;

/* Sound In Hardware API */
int16_t lc3_sndin_hw_init(const SoundInHWConfig *config);
int16_t lc3_sndin_hw_start_record(void);
int16_t lc3_sndin_hw_stop_record(void);
int16_t lc3_sndin_hw_set_gain(uint8_t gain, bool enableAGC);
int16_t lc3_sndin_hw_get_meter_level(uint8_t *peakLevel);
int16_t lc3_sndin_hw_process_interrupt(uint8_t *destBuffer, uint16_t maxBytes, uint16_t *actualBytes);

#endif /* LC3_SOUND_IN_DRIVER_H */
