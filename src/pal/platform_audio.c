/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original DOS RIX/OPL2 music, rendered at build time and streamed through
 * a looping hardware PCM voice. No MIDI / SoundFont substitution. */
#define PAL_PLATFORM_IMPLEMENTATION
#include "audio.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"
#include "palcommon.h"
#include "of_mixer.h"
#include "of_codec.h"
#include "of_cache.h"
#include "platform_movie.h"
#ifndef OF_PC
#include "of_file.h"
#include "of_caps.h"
#endif

#undef fopen /* The PCM archive is a separate APF file, outside pal.pak. */

#define FM_RATE 44100
#define FM_HALF 262144
#define FM_SAMPLES (FM_HALF * 2)
#define SOUND_COUNT 512
#define SOUND_CACHE_LIMIT (4 * 1024 * 1024)
AUDIODEVICE gAudioDevice;
static struct {
    int16_t *pcm;
    uint32_t count, rate;
    of_mixer_handle_t voice;
#ifdef OF_PC
    uint64_t cursor;
    int playing;
#endif
} sounds[SOUND_COUNT];
static uint32_t sound_cache_bytes;
static struct { uint32_t offset, count, loop; } tracks[128];
static FILE *music_file;
static int16_t *music_pcm;
static unsigned track_count, read_sample;
static int current_music = -1, looping, last_half, last_position;
static uint64_t played;
static of_mixer_handle_t music_voice;
static int gain = 255, pending_music = -1, pending_loop;
static Uint32 fade_start, fade_duration;
static int fade_from, fade_to;
static int16_t *movie_pcm;
static uint64_t movie_played;
static unsigned movie_last_position;
static of_mixer_handle_t movie_voice = OF_MIXER_HANDLE_INVALID;
#ifdef OF_PC
static SDL_AudioDeviceID device;
static unsigned desktop_position;
static uint32_t movie_hash;
static void music_callback(void *unused, Uint8 *output, int bytes) {
    (void)unused;
    int16_t *out = (int16_t *)output;
    for (int i=0; i<bytes/2; i++) {
        if (movie_pcm) {
            int16_t sample=movie_pcm[movie_played % MOVIE_RING];
            movie_played++;
            out[i]=gAudioDevice.fMusicEnabled ? sample*gConfig.iMusicVolume/PAL_MAX_VOLUME : 0;
            movie_hash=(movie_hash ^ (uint16_t)out[i])*16777619u;
            continue;
        }
        int sample = current_music > 0 ? music_pcm[desktop_position] * gain / 255 : 0;
        if (current_music > 0) desktop_position = (desktop_position+1) % FM_SAMPLES;
        for (int j=1; j<SOUND_COUNT; j++) {
            if (!sounds[j].playing) continue;
            unsigned pos = sounds[j].cursor / FM_RATE;
            sample += sounds[j].pcm[pos] * gConfig.iSoundVolume / PAL_MAX_VOLUME;
            sounds[j].cursor += sounds[j].rate;
            if (sounds[j].cursor / FM_RATE >= sounds[j].count) sounds[j].playing = 0;
        }
        out[i] = sample < -32768 ? -32768 : sample > 32767 ? 32767 : sample;
    }
}
#define LOCK() do { if (device) SDL_LockAudioDevice(device); } while (0)
#define UNLOCK() do { if (device) SDL_UnlockAudioDevice(device); } while (0)
#else
#define LOCK() ((void)0)
#define UNLOCK() ((void)0)
#endif
static uint32_t read32(FILE *f) {
    unsigned char b[4];
    if (fread(b,1,4,f)!=4) return 0;
    return b[0] | (uint32_t)b[1]<<8 | (uint32_t)b[2]<<16 | (uint32_t)b[3]<<24;
}
static void set_gain(int value) {
    gain = gAudioDevice.fMusicEnabled ? value * gConfig.iMusicVolume / PAL_MAX_VOLUME : 0;
    of_mixer_set_vol_lr_h(music_voice,gain,gain);
}
static int load_music(void) {
#ifndef OF_PC
    if (!of_has_feature(OF_HW_MIXER)) return -1;
    of_file_slot_register(5,"pal-fm.pcm");
#endif
    music_file = fopen("pal-fm.pcm","rb");
    if (!music_file) return -1;
    char magic[8];
    if (fread(magic,1,8,music_file)!=8 || memcmp(magic,"PALFM01\0",8) || read32(music_file)!=FM_RATE) return -1;
    track_count=read32(music_file);
    if (!track_count || track_count>128 || fseek(music_file,0,SEEK_END)) return -1;
    long size=ftell(music_file);
    if (size<16+(long)track_count*12 || fseek(music_file,16,SEEK_SET)) return -1;
    for (unsigned i=0;i<track_count;i++) {
        tracks[i].offset=read32(music_file);
        tracks[i].count=read32(music_file);
        tracks[i].loop=read32(music_file);
        if (!tracks[i].count) continue;
        if (tracks[i].offset<16+track_count*12 || tracks[i].offset>(unsigned long)size ||
            tracks[i].count>((unsigned long)size-tracks[i].offset)/2 ||
            !tracks[i].loop || tracks[i].loop>=tracks[i].count) return -1;
    }
    if (track_count<2 || !tracks[1].count) return -1;
    music_pcm=malloc(FM_SAMPLES*sizeof *music_pcm);
    return music_pcm ? 0 : -1;
}
static void stop_music(void) {
    of_mixer_stop_h(music_voice);
    music_voice=OF_MIXER_HANDLE_INVALID;
    current_music=-1;
}
int PAL_MovieAudioStart(int16_t *pcm) {
    LOCK();
#ifdef OF_PC
    if (!device) { UNLOCK(); return 0; }
#else
    if (!of_has_feature(OF_HW_MIXER)) { UNLOCK(); return 0; }
#endif
    stop_music(); pending_music=-1; fade_duration=0;
    for (int i=0; i<SOUND_COUNT; i++) {
        of_mixer_stop_h(sounds[i].voice);
#ifdef OF_PC
        sounds[i].playing=0;
#endif
    }
    movie_pcm=pcm; movie_played=0; movie_last_position=0;
    of_cache_flush_range(pcm,MOVIE_RING*sizeof *pcm);
#ifdef OF_PC
    movie_hash=2166136261u;
#else
    movie_voice=of_mixer_alloc_for_group_h(OF_MIXER_GROUP_MUSIC,
        (const uint8_t *)pcm,MOVIE_RING,MOVIE_RATE,10,0);
    of_mixer_set_loop_h(movie_voice,0,MOVIE_RING);
    if (!of_mixer_handle_active(movie_voice)) { movie_pcm=NULL; UNLOCK(); return 0; }
    of_mixer_set_volume_h(movie_voice,gAudioDevice.fMusicEnabled ? gConfig.iMusicVolume*255/PAL_MAX_VOLUME : 0);
#endif
    UNLOCK();
    return 1;
}
int64_t PAL_MovieAudioClock(void) {
    LOCK();
    int64_t result=-1;
    if (movie_pcm) {
#ifndef OF_PC
        int pos=of_mixer_get_position_h(movie_voice);
        if (pos<0 || pos>=MOVIE_RING || !of_mixer_handle_active(movie_voice)) { UNLOCK(); return -1; }
        movie_played+=(pos+MOVIE_RING-movie_last_position)%MOVIE_RING;
        movie_last_position=pos;
#endif
        result=movie_played;
    }
    UNLOCK();
    return result;
}
void PAL_MovieAudioStop(void) {
    LOCK();
    of_mixer_stop_h(movie_voice); movie_voice=OF_MIXER_HANDLE_INVALID;
    movie_pcm=NULL;
    UNLOCK();
}
#ifdef OF_PC
void PAL_MovieAudioAdvance(unsigned samples) {
    SDL_PauseAudioDevice(device,1);
    int16_t output[1024];
    while (samples) {
        unsigned n=samples>1024 ? 1024 : samples;
        music_callback(NULL,(Uint8 *)output,n*2); samples-=n;
    }
}
uint32_t PAL_MovieAudioHash(void) { return movie_hash; }
#endif
static void fill_half(int half) {
    int16_t *dst=music_pcm+half*FM_HALF;
    unsigned left=FM_HALF;
    while (left) {
        unsigned end=looping ? tracks[current_music].count : tracks[current_music].loop;
        if (read_sample==end) {
            if (!looping) { memset(dst,0,left*sizeof *dst); break; }
            read_sample=tracks[current_music].loop;
        }
        unsigned n=end-read_sample;
        if (n>left) n=left;
        if (fseek(music_file,tracks[current_music].offset+read_sample*2,SEEK_SET) ||
            fread(dst,sizeof *dst,n,music_file)!=n) {
            memset(dst,0,left*sizeof *dst);
            printf("PAL: FM read error\n"); break;
        }
        dst+=n; left-=n; read_sample+=n;
    }
    of_cache_flush_range(music_pcm+half*FM_HALF,FM_HALF*sizeof *music_pcm);
}
static void start_music(int number, int loop) {
    stop_music();
    if (!music_pcm || number<=0 || (unsigned)number>=track_count || !tracks[number].count) return;
    current_music=number; looping=loop; read_sample=0; played=0; last_position=0; last_half=0;
    fill_half(0); fill_half(1);
#ifdef OF_PC
    desktop_position=0;
#else
    music_voice=of_mixer_alloc_for_group_h(OF_MIXER_GROUP_MUSIC,
        (const uint8_t *)music_pcm,FM_SAMPLES,FM_RATE,10,0);
    of_mixer_set_loop_h(music_voice,0,FM_SAMPLES);
    if (!of_mixer_handle_active(music_voice)) { stop_music(); printf("PAL: FM voice unavailable\n"); }
#endif
}
/* Called frequently by the game's event loop. Only refill the half that the
 * hardware has finished reading; each half allows almost six seconds of IO. */
void PAL_PocketAudioPump(void) {
    LOCK();
    if (movie_pcm) {
        of_mixer_set_volume_h(movie_voice,gAudioDevice.fMusicEnabled ? gConfig.iMusicVolume*255/PAL_MAX_VOLUME : 0);
        UNLOCK(); return;
    }
    Uint32 now=SDL_GetTicks();
    int level=255;
    if (fade_duration) {
        Uint32 elapsed=now-fade_start;
        if (elapsed>=fade_duration) {
            Uint32 next_duration=fade_duration;
            level=fade_to; fade_duration=0;
            if (pending_music!=-1) {
                int next=pending_music; pending_music=-1;
                start_music(next,pending_loop);
                fade_start=now; fade_duration=next_duration;
                fade_from=0; fade_to=255;
            }
        } else level=fade_from+(fade_to-fade_from)*(int64_t)elapsed/fade_duration;
    }
    set_gain(level);
    if (current_music>0) {
#ifdef OF_PC
        int pos=desktop_position;
#else
        int pos=of_mixer_get_position_h(music_voice);
#endif
        if (pos>=0 && pos<FM_SAMPLES) {
            played+=(pos+FM_SAMPLES-last_position)%FM_SAMPLES;
            last_position=pos;
            if (!looping && played>=tracks[current_music].loop) stop_music();
            else if (pos/FM_HALF!=last_half) {
                fill_half(last_half); last_half=pos/FM_HALF;
            }
        }
    }
    UNLOCK();
}
INT AUDIO_OpenDevice(void) {
    memset(&gAudioDevice,0,sizeof gAudioDevice);
    gAudioDevice.spec.freq=FM_RATE; gAudioDevice.spec.channels=1;
    gAudioDevice.spec.format=AUDIO_S16SYS;
    gAudioDevice.fMusicEnabled=gAudioDevice.fSoundEnabled=TRUE;
    gAudioDevice.fOpened=TRUE;
    music_voice=OF_MIXER_HANDLE_INVALID;
    of_mixer_init(32,48000);
    if (load_music()) {
        printf("PAL: missing or invalid pal-fm.pcm; FM music unavailable\n");
        if (music_file) fclose(music_file);
        music_file=NULL; free(music_pcm); music_pcm=NULL;
    }
#ifdef OF_PC
    SDL_AudioSpec want=gAudioDevice.spec;
    want.samples=1024; want.callback=music_callback;
    device=SDL_OpenAudioDevice(NULL,0,&want,NULL,0);
    if (device) SDL_PauseAudioDevice(device,0);
#endif
    return 0;
}
VOID AUDIO_CloseDevice(void) {
    PAL_MovieAudioStop();
#ifdef OF_PC
    if (device) SDL_CloseAudioDevice(device);
    device=0;
#endif
    stop_music(); free(music_pcm); music_pcm=NULL;
    if (music_file) fclose(music_file);
    music_file=NULL;
    for (unsigned i=0;i<SOUND_COUNT;i++) {
        of_mixer_stop_h(sounds[i].voice); free(sounds[i].pcm);
        memset(&sounds[i],0,sizeof sounds[i]);
    }
    sound_cache_bytes=0;
    gAudioDevice.fOpened=FALSE;
}
SDL_AudioSpec *AUDIO_GetDeviceSpec(void) { return &gAudioDevice.spec; }
VOID AUDIO_PlayMusic(INT number, BOOL loop, FLOAT fade) {
    LOCK();
    if (number==current_music && current_music>0) { UNLOCK(); return; }
    if (number<0) number=0;
    pending_music=-1;
    if (current_music>0 && fade>0) {
        pending_music=number; pending_loop=loop;
        fade_start=SDL_GetTicks(); fade_duration=(Uint32)(fade*500);
        if (!fade_duration) fade_duration=1;
        fade_from=255; fade_to=0;
    } else {
        start_music(number,loop);
        fade_start=SDL_GetTicks(); fade_duration=fade>0 ? (Uint32)(fade*500):0;
        fade_from=0; fade_to=255; set_gain(fade_duration ? 0:255);
    }
    UNLOCK();
}
static int sound_active(int number) {
#ifdef OF_PC
    return sounds[number].playing;
#else
    return of_mixer_handle_active(sounds[number].voice);
#endif
}
static int load_sound(int number) {
    if (!sounds[number].pcm) {
        FILE *f = UTIL_OpenFile(gConfig.fIsWIN95 ? "sounds.mkf" : "voc.mkf");
        if (!f) return 0;
        int size = PAL_MKFGetChunkSize(number, f);
        uint8_t *data = size > 0 ? malloc(size) : NULL;
        of_codec_result_t decoded = {0};
        Uint8 *wav = NULL;
        if (data && PAL_MKFReadChunk(data, size, number, f) == size) {
            if (gConfig.fIsWIN95) {
                SDL_AudioSpec spec;
                Uint32 len;
                if (SDL_LoadWAV_RW(SDL_RWFromConstMem(data, size), 1, &spec, &wav, &len) &&
                    spec.channels == 1 && (spec.format == AUDIO_U8 || spec.format == AUDIO_S16LSB)) {
                    decoded.pcm = wav; decoded.pcm_len = len;
                    decoded.sample_rate = spec.freq;
                    decoded.channels = 1;
                    decoded.bits_per_sample = spec.format == AUDIO_U8 ? 8 : 16;
                }
            } else of_codec_parse_voc(data, size, &decoded);
        }
        if (decoded.pcm && decoded.channels == 1 && decoded.sample_rate &&
            (decoded.bits_per_sample == 8 || decoded.bits_per_sample == 16)) {
            unsigned width = decoded.bits_per_sample / 8;
            unsigned count = decoded.pcm_len / width;
            unsigned bytes = count * sizeof(int16_t);
            for (int i=1; i<SOUND_COUNT && sound_cache_bytes + bytes > SOUND_CACHE_LIMIT; i++) {
                if (!sounds[i].pcm || sound_active(i)) continue;
                sound_cache_bytes -= sounds[i].count * sizeof(int16_t);
                free(sounds[i].pcm); memset(&sounds[i], 0, sizeof sounds[i]);
            }
            int16_t *pcm = count && sound_cache_bytes + bytes <= SOUND_CACHE_LIMIT ? malloc(bytes) : NULL;
            if (pcm) {
                for (unsigned i = 0; i < count; i++)
                    pcm[i] = width == 1 ? ((int)decoded.pcm[i] - 128) * 256 :
                        (int16_t)((uint16_t)decoded.pcm[i*2] | (uint16_t)decoded.pcm[i*2+1] << 8);
                of_cache_flush_range(pcm, bytes);
                sounds[number].pcm = pcm;
                sounds[number].count = count;
                sounds[number].rate = decoded.sample_rate;
                sound_cache_bytes += bytes;
            }
        }
        SDL_FreeWAV(wav);
        free(data); fclose(f);
    }
    return sounds[number].pcm != NULL;
}
VOID AUDIO_PlaySound(INT number) {
    if (!gAudioDevice.fSoundEnabled || number <= -SOUND_COUNT || number >= SOUND_COUNT || !number) return;
    if (number < 0) number = -number;
    LOCK();
    load_sound(number);
    if (sounds[number].pcm) {
#ifdef OF_PC
        sounds[number].cursor = 0;
        sounds[number].playing = 1;
#else
        of_mixer_stop_h(sounds[number].voice);
        sounds[number].voice = of_mixer_alloc_for_group_h(OF_MIXER_GROUP_SFX,
            (const uint8_t *)sounds[number].pcm, sounds[number].count,
            sounds[number].rate, 1, gConfig.iSoundVolume * 255 / PAL_MAX_VOLUME);
#endif
    }
    UNLOCK();
}
BOOL AUDIO_CD_Available(void) { return FALSE; }
BOOL AUDIO_PlayCDTrack(INT track) { (void)track; return FALSE; }
VOID AUDIO_EnableMusic(BOOL enabled) {
    gAudioDevice.fMusicEnabled = enabled;
    PAL_PocketAudioPump();
}
BOOL AUDIO_MusicEnabled(void) { return gAudioDevice.fMusicEnabled; }
VOID AUDIO_EnableSound(BOOL enabled) {
    LOCK();
    gAudioDevice.fSoundEnabled = enabled;
    if (!enabled) for (int i = 0; i < SOUND_COUNT; i++) {
#ifdef OF_PC
        sounds[i].playing = 0;
#else
        of_mixer_stop_h(sounds[i].voice);
#endif
    }
    UNLOCK();
}
BOOL AUDIO_SoundEnabled(void) { return gAudioDevice.fSoundEnabled; }
static void change_volume(int delta) {
    int v = gConfig.iMusicVolume + delta;
    gConfig.iMusicVolume = v < 0 ? 0 : v > PAL_MAX_VOLUME ? PAL_MAX_VOLUME : v;
    PAL_PocketAudioPump();
}
VOID AUDIO_IncreaseVolume(void) { change_volume(5); }
VOID AUDIO_DecreaseVolume(void) { change_volume(-5); }
void AUDIO_Lock(void) {}
void AUDIO_Unlock(void) {}

#ifdef OF_PC
static uint32_t sound_hash(uint32_t hash, uint32_t value) {
    return (hash ^ value) * 16777619u;
}
static int sound_smoke(void) {
    if (!gConfig.fIsWIN95) return 0;
    FILE *f=UTIL_OpenFile("sounds.mkf");
    if (!f) return 1;
    int count=PAL_MKFGetChunkCount(f), clips=0, high=0;
    uint32_t hash=2166136261u;
    if (count>SOUND_COUNT) { fclose(f); return 1; }
    for (int i=1; i<count; i++) {
        if (PAL_MKFGetChunkSize(i,f)<=0) continue;
        if (!load_sound(i) || sound_cache_bytes>SOUND_CACHE_LIMIT) { fclose(f); return 1; }
        clips++;
        hash=sound_hash(hash,i);
        hash=sound_hash(hash,sounds[i].rate);
        hash=sound_hash(hash,sounds[i].count);
        for (unsigned j=0; j<sounds[i].count; j++) hash=sound_hash(hash,(uint16_t)sounds[i].pcm[j]);
        if (i>275) high=i;
    }
    fclose(f);
    if (!high) return 1;
    int volume=gConfig.iSoundVolume;
    gConfig.iSoundVolume=PAL_MAX_VOLUME;
    AUDIO_PlaySound(high);
    int16_t actual[1024];
    music_callback(NULL,(Uint8 *)actual,sizeof actual);
    for (unsigned i=0; i<1024; i++) {
        unsigned pos=(uint64_t)i*sounds[high].rate/FM_RATE;
        int16_t expected=pos<sounds[high].count ? sounds[high].pcm[pos] : 0;
        if (actual[i]!=expected) return 1;
    }
    AUDIO_EnableSound(FALSE);
    music_callback(NULL,(Uint8 *)actual,sizeof actual);
    for (unsigned i=0; i<1024; i++) if (actual[i]) return 1;
    AUDIO_EnableSound(TRUE);
    gConfig.iSoundVolume=volume;
    printf("PASS: PAL98 WAV clips=%d, PCM hash=%08x, high sound IDs, playback, mute, bounded cache\n",clips,hash);
    return 0;
}
/* Exercise the actual playback callback and ring refill across song repeats,
 * comparing every output sample to the rendered source, without wall time. */
int PAL_PocketAudioSmoke(void) {
    if (!music_pcm || !device) return 1;
    SDL_PauseAudioDevice(device,1);
    int old_volume=gConfig.iMusicVolume;
    gConfig.iMusicVolume=PAL_MAX_VOLUME;
    AUDIO_PlayMusic(1,TRUE,0);
    FILE *ref=fopen("pal-fm.pcm","rb");
    if (!ref) return 1;
    unsigned expected=0;
    int16_t actual[1024];
    uint32_t total=tracks[1].count*2;
    for (uint32_t i=0;i<total;) {
        unsigned n=total-i>1024 ? 1024:total-i;
        music_callback(NULL,(Uint8 *)actual,n*2);
        for (unsigned j=0;j<n;j++) {
            if (expected==tracks[1].count) expected=tracks[1].loop;
            fseek(ref,tracks[1].offset+expected*2,SEEK_SET);
            int16_t sample=(int16_t)(uint16_t)fgetc(ref);
            sample=(int16_t)((uint16_t)sample | (uint16_t)fgetc(ref)<<8);
            if (actual[j]!=sample) { fprintf(stderr,"FM mismatch at %u\n",i+j); fclose(ref); return 1; }
            expected++;
        }
        i+=n; PAL_PocketAudioPump();
    }
    fclose(ref);
    AUDIO_EnableMusic(FALSE);
    music_callback(NULL,(Uint8 *)actual,sizeof actual);
    for (unsigned i=0;i<1024;i++) if (actual[i]) return 1;
    AUDIO_EnableMusic(TRUE);
    AUDIO_PlayMusic(2,FALSE,0);
    for (unsigned i=0;i<tracks[2].loop+2048;i+=1024) {
        music_callback(NULL,(Uint8 *)actual,sizeof actual); PAL_PocketAudioPump();
    }
    if (current_music!=-1) return 1;
    AUDIO_PlayMusic(1,TRUE,0);
    AUDIO_PlayMusic(2,TRUE,1);
    fade_start-=fade_duration+1; PAL_PocketAudioPump();
    if (current_music!=2 || gain!=0) return 1;
    fade_start-=fade_duration+1; PAL_PocketAudioPump();
    if (gain!=255) return 1;
    AUDIO_PlayMusic(0,FALSE,0);
    if (current_music!=-1) return 1;
    gConfig.iMusicVolume=old_volume;
    puts("PASS: FM PCM exact samples over multiple ring/song loops, one-shot, mute, fade, stop");
    return sound_smoke();
}
#endif
