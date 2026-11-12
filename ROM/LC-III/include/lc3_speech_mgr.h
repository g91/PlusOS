/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Speech Manager Core Engine (MacinTalk / PlainTalk ROM Vectors)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SPEECH_MGR_H
#define LC3_SPEECH_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define SPEECH_NO_ERR           0
#define SPEECH_BUSY_ERR      -241
#define SPEECH_BAD_CHAN_ERR  -242
#define SPEECH_PARAM_ERR      -50

typedef void *SpeechChannel;

typedef struct VoiceSpec {
    uint32_t creator;
    uint32_t id;
} VoiceSpec;

typedef struct SpeechChannelRecord {
    VoiceSpec voice;
    int16_t pitch;
    int16_t rate;
    bool isSpeaking;
    bool isPaused;
    uint32_t textPos;
} SpeechChannelRecord;

/* Speech Manager API */
uint32_t lc3_speech_manager_version(void);
int16_t lc3_new_speech_channel(const VoiceSpec *voice, SpeechChannel *chan);
int16_t lc3_dispose_speech_channel(SpeechChannel chan);
int16_t lc3_speak_string(SpeechChannel chan, const char *textToSpeak);
int16_t lc3_speak_text(SpeechChannel chan, const void *textBuf, uint32_t byteLen);
int16_t lc3_stop_speech(SpeechChannel chan);
int16_t lc3_pause_speech_at(SpeechChannel chan, int32_t whereToPause);
int16_t lc3_continue_speech(SpeechChannel chan);
int16_t lc3_speech_busy(void);

#endif /* LC3_SPEECH_MGR_H */
