/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickTime Movie Toolbox & Image Compression ROM Vectors
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_QUICKTIME_H
#define LC3_QUICKTIME_H

#include <stdint.h>
#include <stdbool.h>

#define QT_NO_ERR              0
#define QT_INVALID_MOVIE_ERR -2000
#define QT_PARAM_ERR           -50

typedef void *Movie;
typedef void *Track;
typedef int32_t TimeValue;
typedef int32_t TimeScale;
typedef int32_t Fixed;

typedef struct MovieRecord {
    TimeValue currentTime;
    TimeValue duration;
    TimeScale timeScale;
    Fixed rate;
    bool isActive;
    bool isPlaying;
    uint32_t trackCount;
    void *gWorld;
} MovieRecord;

/* Movie Toolbox API */
int16_t lc3_enter_movies(void);
void lc3_exit_movies(void);
int16_t lc3_movies_task(Movie theMovie, uint32_t maxMilliSecToUse);
TimeValue lc3_get_movie_duration(Movie theMovie);
TimeValue lc3_get_movie_time(Movie theMovie, void *timeRecord);
void lc3_set_movie_time_value(Movie theMovie, TimeValue newTime);
void lc3_start_movie(Movie theMovie);
void lc3_stop_movie(Movie theMovie);
Fixed lc3_get_movie_rate(Movie theMovie);
void lc3_set_movie_rate(Movie theMovie, Fixed rate);
bool lc3_is_movie_done(Movie theMovie);

#endif /* LC3_QUICKTIME_H */
