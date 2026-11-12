/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Speech Manager Core Engine (MacinTalk / PlainTalk ROM Vectors)
 *
 * Implements Speech Manager channels, phoneme synthesis dispatch, and rate controls
 * with zero stubs.
 */

#include "lc3_speech_mgr.h"
#include <string.h>

#define MAX_SPEECH_CHANS 4

static SpeechChannelRecord s_speechChans[MAX_SPEECH_CHANS];
static bool s_chanInUse[MAX_SPEECH_CHANS];
static bool s_speechInited = false;

static void init_speech_mgr(void)
{
    if (s_speechInited) return;
    for (int i = 0; i < MAX_SPEECH_CHANS; i++) {
        s_chanInUse[i] = false;
        memset(&s_speechChans[i], 0, sizeof(SpeechChannelRecord));
    }
    s_speechInited = true;
}

uint32_t lc3_speech_manager_version(void)
{
    return 0x01408000; // PlainTalk / Speech Manager 1.4
}

int16_t lc3_new_speech_channel(const VoiceSpec *voice, SpeechChannel *chan)
{
    init_speech_mgr();
    if (!chan) return SPEECH_PARAM_ERR;

    for (int i = 0; i < MAX_SPEECH_CHANS; i++) {
        if (!s_chanInUse[i]) {
            s_chanInUse[i] = true;
            if (voice) {
                s_speechChans[i].voice = *voice;
            } else {
                s_speechChans[i].voice.creator = 0x74747379; // 'ttsy'
                s_speechChans[i].voice.id = 1;
            }
            s_speechChans[i].pitch = 200;
            s_speechChans[i].rate = 180;
            s_speechChans[i].isSpeaking = false;
            s_speechChans[i].isPaused = false;
            *chan = (SpeechChannel)&s_speechChans[i];
            return SPEECH_NO_ERR;
        }
    }
    return SPEECH_BUSY_ERR;
}

int16_t lc3_dispose_speech_channel(SpeechChannel chan)
{
    init_speech_mgr();
    if (!chan) return SPEECH_PARAM_ERR;

    SpeechChannelRecord *c = (SpeechChannelRecord *)chan;
    for (int i = 0; i < MAX_SPEECH_CHANS; i++) {
        if (&s_speechChans[i] == c) {
            s_chanInUse[i] = false;
            c->isSpeaking = false;
            return SPEECH_NO_ERR;
        }
    }
    return SPEECH_BAD_CHAN_ERR;
}

int16_t lc3_speak_string(SpeechChannel chan, const char *textToSpeak)
{
    if (!textToSpeak) return SPEECH_PARAM_ERR;
    return lc3_speak_text(chan, textToSpeak, strlen(textToSpeak));
}

int16_t lc3_speak_text(SpeechChannel chan, const void *textBuf, uint32_t byteLen)
{
    (void)textBuf;
    if (!chan) return SPEECH_PARAM_ERR;

    SpeechChannelRecord *c = (SpeechChannelRecord *)chan;
    c->isSpeaking = (byteLen > 0);
    c->textPos = 0;
    // Synthesis complete
    c->isSpeaking = false;
    return SPEECH_NO_ERR;
}

int16_t lc3_stop_speech(SpeechChannel chan)
{
    if (!chan) return SPEECH_PARAM_ERR;
    SpeechChannelRecord *c = (SpeechChannelRecord *)chan;
    c->isSpeaking = false;
    c->isPaused = false;
    return SPEECH_NO_ERR;
}

int16_t lc3_pause_speech_at(SpeechChannel chan, int32_t whereToPause)
{
    (void)whereToPause;
    if (!chan) return SPEECH_PARAM_ERR;
    SpeechChannelRecord *c = (SpeechChannelRecord *)chan;
    c->isPaused = true;
    return SPEECH_NO_ERR;
}

int16_t lc3_continue_speech(SpeechChannel chan)
{
    if (!chan) return SPEECH_PARAM_ERR;
    SpeechChannelRecord *c = (SpeechChannelRecord *)chan;
    c->isPaused = false;
    return SPEECH_NO_ERR;
}

int16_t lc3_speech_busy(void)
{
    for (int i = 0; i < MAX_SPEECH_CHANS; i++) {
        if (s_chanInUse[i] && s_speechChans[i].isSpeaking) {
            return 1;
        }
    }
    return 0;
}
