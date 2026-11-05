/* SPDX-License-Identifier: GPL-3.0-or-later
 * PALAVI1 contains fixed-size paletted frames interleaved with mono PCM.
 * Bounded read-ahead keeps SD reads off the audio callback; audio is the clock.
 */
#define PAL_PLATFORM_IMPLEMENTATION
#include "aviplay.h"
#include "audio.h"
#include "input.h"
#include "video.h"
#include "global.h"
#include "palcfg.h"
#include "platform_movie.h"
#include "of_cache.h"
#ifndef OF_PC
#include "of_file.h"
#endif
#undef fopen /* Movie data uses its own APF slot, outside pal.pak. */

static struct {
    FILE *file;
    uint8_t *frames, *saved;
    int16_t *pcm;
    SDL_Color palette[256];
    uint32_t count, loaded, shown;
    int skipped;
} movie;
#ifdef OF_PC
static int test_mode;
static uint32_t video_hash;
static int test_failed;
extern SDL_Window *gpWindow;
static int check_movie_output(void) {
    SDL_Surface *output=SDL_GetWindowSurface(gpWindow);
    if (!output || output->w!=320 || output->h!=240) return 0;
    for (int y=0; y<240; y++) for (int x=0; x<320; x++) {
        SDL_Color expected={0,0,0,255};
        if (y>=20 && y<220) {
            unsigned index=*((uint8_t *)gpScreen->pixels+(y-20)*gpScreen->pitch+x);
            expected=gpScreen->format->palette->colors[index];
        }
        uint32_t pixel=0;
        memcpy(&pixel,(uint8_t *)output->pixels+y*output->pitch+x*output->format->BytesPerPixel,output->format->BytesPerPixel);
        uint8_t r,g,b;
        SDL_GetRGB(pixel,output->format,&r,&g,&b);
        if (r!=expected.r || g!=expected.g || b!=expected.b) return 0;
        if ((x<16 || x>=304 || y<30 || y>=210) && (r || g || b)) return 0;
    }
    return 1;
}
#endif

static uint32_t read32(FILE *file) {
    uint8_t b[4];
    if (fread(b,1,4,file)!=4) return 0;
    return b[0] | (uint32_t)b[1]<<8 | (uint32_t)b[2]<<16 | (uint32_t)b[3]<<24;
}

static int open_movie(int number) {
#ifndef OF_PC
    of_file_slot_register(7,"pal-avi.pav");
#endif
    movie.file=fopen("pal-avi.pav","rb");
    if (!movie.file) return 0;
    if (fseek(movie.file,0,SEEK_END)) return 0;
    long size=ftell(movie.file);
    if (size<124 || size>0x7fffffff || fseek(movie.file,0,SEEK_SET)) return 0;
    char magic[8];
    if (fread(magic,1,8,movie.file)!=8 || memcmp(magic,"PALAVI1\0",8) ||
        read32(movie.file)!=MOVIE_WIDTH || read32(movie.file)!=MOVIE_HEIGHT ||
        read32(movie.file)!=MOVIE_FPS || read32(movie.file)!=MOVIE_RATE || read32(movie.file)!=6) return 0;
    uint64_t previous=124;
    uint32_t offset=0;
    for (int i=1; i<=6; i++) {
        uint32_t id=read32(movie.file), start=read32(movie.file);
        uint32_t count=read32(movie.file), samples=read32(movie.file);
        uint64_t end=(uint64_t)start+(uint64_t)count*MOVIE_RECORD;
        if (id!=(unsigned)i || start<previous || !count || end>(uint64_t)size ||
            (uint64_t)count*MOVIE_SAMPLES!=samples) return 0;
        previous=end;
        if (i==number) { offset=start; movie.count=count; }
    }
    return !fseek(movie.file,offset,SEEK_SET);
}

static int read_frame(void) {
    uint8_t *frame=movie.frames+(movie.loaded % MOVIE_QUEUE)*MOVIE_VIDEO;
    uint8_t audio[MOVIE_SAMPLES*2];
    if (fread(frame,1,MOVIE_VIDEO,movie.file)!=MOVIE_VIDEO ||
        fread(audio,1,sizeof audio,movie.file)!=sizeof audio) return 0;
    unsigned pos=((uint64_t)movie.loaded*MOVIE_SAMPLES)%MOVIE_RING;
    for (unsigned i=0; i<MOVIE_SAMPLES; i++)
        movie.pcm[(pos+i)%MOVIE_RING]=(int16_t)((uint16_t)audio[i*2] | (uint16_t)audio[i*2+1]<<8);
    unsigned first=MOVIE_RING-pos;
    if (first>MOVIE_SAMPLES) first=MOVIE_SAMPLES;
    of_cache_flush_range(movie.pcm+pos,first*sizeof(int16_t));
    if (first<MOVIE_SAMPLES) of_cache_flush_range(movie.pcm,(MOVIE_SAMPLES-first)*sizeof(int16_t));
    movie.loaded++;
    return 1;
}

static void present_frame(unsigned number) {
    const uint8_t *frame=movie.frames+(number % MOVIE_QUEUE)*MOVIE_VIDEO;
    SDL_Color colors[256];
    for (int i=0; i<256; i++) {
        colors[i].r=frame[i*3]; colors[i].g=frame[i*3+1]; colors[i].b=frame[i*3+2]; colors[i].a=255;
    }
    colors[255].r=colors[255].g=colors[255].b=0;
    SDL_FillRect(gpScreen,NULL,255);
    for (int y=0; y<MOVIE_HEIGHT; y++)
        memcpy((uint8_t *)gpScreen->pixels+(y+10)*gpScreen->pitch+16,
            frame+768+y*MOVIE_WIDTH,MOVIE_WIDTH);
    VIDEO_SetPalette(colors);
    movie.shown=number+1;
#ifdef OF_PC
    if (test_mode==1) {
        for (int i=0; i<768; i++) video_hash=(video_hash ^ frame[i])*16777619u;
        for (int y=0; y<MOVIE_HEIGHT; y++) for (int x=0; x<MOVIE_WIDTH; x++)
            video_hash=(video_hash ^ *((uint8_t *)gpScreen->pixels+(y+10)*gpScreen->pitch+x+16))*16777619u;
        if ((!number || number==movie.count/2 || number+1==movie.count) && !check_movie_output()) test_failed=1;
    }
#endif
}

static void close_movie(void) {
    PAL_MovieAudioStop();
    if (movie.saved) {
        for (int y=0; y<200; y++) memcpy((uint8_t *)gpScreen->pixels+y*gpScreen->pitch,movie.saved+y*320,320);
        VIDEO_SetPalette(movie.palette);
    }
    if (movie.file) fclose(movie.file);
    free(movie.frames); free(movie.pcm); free(movie.saved);
    memset(&movie,0,sizeof movie);
}

void PAL_AVIInit(void) {}
void PAL_AVIShutdown(void) { close_movie(); }
void AVI_FillAudioBuffer(void *u, uint8_t *s, int n) { (void)u; (void)s; (void)n; }
void *AVI_GetPlayState(void) { return NULL; }

BOOL PAL_PlayAVI(const char *path) {
    if (!gConfig.fEnableAviPlay || !gConfig.fIsWIN95 || movie.file) return FALSE;
    if (strlen(path)!=5 || path[0]<'1' || path[0]>'6' || strcasecmp(path+1,".avi")) return FALSE;
    int number=path[0]-'0', success=0;
    if (!open_movie(number)) goto done;
    movie.frames=malloc(MOVIE_QUEUE*MOVIE_VIDEO);
    movie.pcm=calloc(MOVIE_RING,sizeof(int16_t));
    movie.saved=malloc(320*200);
    if (!movie.frames || !movie.pcm || !movie.saved) {
        free(movie.saved); movie.saved=NULL; goto done;
    }
    memcpy(movie.palette,VIDEO_GetPalette(),sizeof movie.palette);
    for (int y=0; y<200; y++) memcpy(movie.saved+y*320,(uint8_t *)gpScreen->pixels+y*gpScreen->pitch,320);
    while (movie.loaded<movie.count && movie.loaded<MOVIE_QUEUE) if (!read_frame()) goto done;
    PAL_ClearKeyState();
    if (!PAL_MovieAudioStart(movie.pcm)) goto done;
    Uint32 last_tick=SDL_GetTicks();
    uint64_t last_sample=0;
    while (1) {
        PAL_ProcessEvent();
#ifdef OF_PC
        if (test_mode==2 && movie.shown>=2) g_InputState.dwKeyPress|=kKeyMenu;
#endif
        if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch)) { movie.skipped=1; success=1; break; }
        if (SDL_GetTicks()-last_tick>2000) { printf("PAL: movie IO or audio clock stalled\n"); goto done; }
        int64_t clock=PAL_MovieAudioClock();
        if (clock<0) goto done;
        uint64_t sample=clock;
        if (sample>last_sample) { last_tick=SDL_GetTicks(); last_sample=sample; }
        unsigned frame=sample/MOVIE_SAMPLES;
        if (frame>=movie.count) { success=1; break; }
        if (frame>=movie.loaded) { printf("PAL: movie stream underrun\n"); goto done; }
        if (frame>=movie.shown) present_frame(frame);
#ifdef OF_PC
        if (test_failed) goto done;
#endif
        while (movie.loaded<movie.count && movie.loaded<frame+MOVIE_QUEUE)
            if (!read_frame()) goto done;
#ifdef OF_PC
        if (test_mode && test_mode!=4) PAL_MovieAudioAdvance(MOVIE_SAMPLES*(test_mode==3 ? 3 : 1));
        else
#endif
            SDL_Delay(1);
    }
done:
#ifdef OF_PC
    if (test_mode && success) printf("PASS: AVI %d frames=%u shown=%u skip=%d video=%08x audio=%08x\n",
        number,movie.count,movie.shown,movie.skipped,video_hash,PAL_MovieAudioHash());
#endif
    if (!success) printf("PAL: AVI %d unavailable or invalid; using engine fallback\n",number);
    close_movie();
    PAL_ClearKeyState();
    return success ? TRUE : FALSE;
}

#ifdef OF_PC
int PAL_PocketAVIRealtime(void) {
    test_mode=4;
    int result=PAL_PlayAVI("1.avi");
    test_mode=0;
    return result ? 0 : 1;
}
int PAL_PocketAVISmoke(void) {
    PAL_MovieAudioAdvance(0);
    int volume=gConfig.iMusicVolume;
    gConfig.iMusicVolume=PAL_MAX_VOLUME;
    SDL_Color palette[256];
    memcpy(palette,VIDEO_GetPalette(),sizeof palette);
    uint8_t saved[64000];
    for (int y=0; y<200; y++) memcpy(saved+y*320,(uint8_t *)gpScreen->pixels+y*gpScreen->pitch,320);
    test_mode=1;
    for (int i=1; i<=6; i++) {
        char path[8]; snprintf(path,sizeof path,"%d.avi",i);
        video_hash=2166136261u;
        if (!PAL_PlayAVI(path)) { test_mode=0; return 1; }
        if (memcmp(palette,VIDEO_GetPalette(),sizeof palette)) return 1;
        for (int y=0; y<200; y++) if (memcmp(saved+y*320,(uint8_t *)gpScreen->pixels+y*gpScreen->pitch,320)) return 1;
        if (PAL_MovieAudioClock()!=-1) return 1;
    }
    test_mode=2;
    if (!PAL_PlayAVI("1.avi")) return 1;
    test_mode=3;
    AUDIO_EnableMusic(FALSE);
    if (!PAL_PlayAVI("1.avi")) return 1;
    AUDIO_EnableMusic(TRUE);
    gConfig.fEnableAviPlay=FALSE;
    if (PAL_PlayAVI("1.avi")) return 1;
    gConfig.fEnableAviPlay=TRUE;
    if (PAL_PlayAVI("7.avi")) return 1;
    test_mode=0;
    gConfig.iMusicVolume=volume;
    puts("PASS: AVI completion, skip, muted clock, dropped frames, fallback, palette/screen restoration and audio cleanup");
    return 0;
}
#endif
