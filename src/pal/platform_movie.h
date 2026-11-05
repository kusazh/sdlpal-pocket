/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PAL_PLATFORM_MOVIE_H
#define PAL_PLATFORM_MOVIE_H
#include <stdint.h>
#define MOVIE_WIDTH 288
#define MOVIE_HEIGHT 180
#define MOVIE_FPS 15
#define MOVIE_RATE 44100
#define MOVIE_SAMPLES (MOVIE_RATE / MOVIE_FPS)
#define MOVIE_PIXELS (MOVIE_WIDTH * MOVIE_HEIGHT)
#define MOVIE_VIDEO (768 + MOVIE_PIXELS)
#define MOVIE_RECORD (MOVIE_VIDEO + MOVIE_SAMPLES * 2)
#define MOVIE_QUEUE 32
#define MOVIE_RING 131072
int PAL_MovieAudioStart(int16_t *pcm);
int64_t PAL_MovieAudioClock(void);
void PAL_MovieAudioStop(void);
#ifdef OF_PC
void PAL_MovieAudioAdvance(unsigned samples);
uint32_t PAL_MovieAudioHash(void);
int PAL_PocketAVISmoke(void);
#endif
#endif
