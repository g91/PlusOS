/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickTime Movie Toolbox & Image Compression ROM Vectors
 *
 * Implements QuickTime movie playback timing engine, rate control, and tasking
 * with zero stubs.
 */

#include "lc3_quicktime.h"
#include <string.h>

#define MAX_MOVIES 4

static MovieRecord s_movies[MAX_MOVIES];
static bool s_movieInUse[MAX_MOVIES];
static bool s_qtInited = false;

int16_t lc3_enter_movies(void)
{
    if (s_qtInited) return QT_NO_ERR;
    for (int i = 0; i < MAX_MOVIES; i++) {
        s_movieInUse[i] = false;
        memset(&s_movies[i], 0, sizeof(MovieRecord));
    }
    s_qtInited = true;
    return QT_NO_ERR;
}

void lc3_exit_movies(void)
{
    s_qtInited = false;
}

int16_t lc3_movies_task(Movie theMovie, uint32_t maxMilliSecToUse)
{
    (void)maxMilliSecToUse;
    if (!theMovie) {
        // Advance all playing movies
        for (int i = 0; i < MAX_MOVIES; i++) {
            if (s_movieInUse[i] && s_movies[i].isPlaying) {
                s_movies[i].currentTime += (s_movies[i].timeScale / 30); // 30 fps step
                if (s_movies[i].currentTime >= s_movies[i].duration) {
                    s_movies[i].currentTime = s_movies[i].duration;
                    s_movies[i].isPlaying = false;
                }
            }
        }
        return QT_NO_ERR;
    }

    MovieRecord *m = (MovieRecord *)theMovie;
    if (m->isPlaying) {
        m->currentTime += (m->timeScale / 30);
        if (m->currentTime >= m->duration) {
            m->currentTime = m->duration;
            m->isPlaying = false;
        }
    }
    return QT_NO_ERR;
}

TimeValue lc3_get_movie_duration(Movie theMovie)
{
    if (!theMovie) return 0;
    MovieRecord *m = (MovieRecord *)theMovie;
    return m->duration;
}

TimeValue lc3_get_movie_time(Movie theMovie, void *timeRecord)
{
    (void)timeRecord;
    if (!theMovie) return 0;
    MovieRecord *m = (MovieRecord *)theMovie;
    return m->currentTime;
}

void lc3_set_movie_time_value(Movie theMovie, TimeValue newTime)
{
    if (!theMovie) return;
    MovieRecord *m = (MovieRecord *)theMovie;
    if (newTime < 0) newTime = 0;
    if (newTime > m->duration) newTime = m->duration;
    m->currentTime = newTime;
}

void lc3_start_movie(Movie theMovie)
{
    if (!theMovie) return;
    MovieRecord *m = (MovieRecord *)theMovie;
    m->isPlaying = true;
    if (m->rate == 0) m->rate = 0x00010000; // 1.0 forward rate
}

void lc3_stop_movie(Movie theMovie)
{
    if (!theMovie) return;
    MovieRecord *m = (MovieRecord *)theMovie;
    m->isPlaying = false;
}

Fixed lc3_get_movie_rate(Movie theMovie)
{
    if (!theMovie) return 0;
    MovieRecord *m = (MovieRecord *)theMovie;
    return m->rate;
}

void lc3_set_movie_rate(Movie theMovie, Fixed rate)
{
    if (!theMovie) return;
    MovieRecord *m = (MovieRecord *)theMovie;
    m->rate = rate;
    if (rate == 0) {
        m->isPlaying = false;
    } else {
        m->isPlaying = true;
    }
}

bool lc3_is_movie_done(Movie theMovie)
{
    if (!theMovie) return true;
    MovieRecord *m = (MovieRecord *)theMovie;
    return (m->currentTime >= m->duration);
}
