// SPDX-License-Identifier: GPL-3.0-or-later
// Build-time DOS RIX -> mono OPL2 PCM, using vendored SDLPAL / DOSBox.
#include "adplug/rix.h"
#include "adplug/dosbox_opls.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>
class OPL : public CEmuopl {
public: OPL() : CEmuopl(new DBINTOPL2(44100), TYPE_OPL2) {}
};
static void u32(FILE *f, uint32_t v) {
    unsigned char b[] = {(unsigned char)v, (unsigned char)(v>>8), (unsigned char)(v>>16), (unsigned char)(v>>24)};
    if (fwrite(b, 1, 4, f) != 4) std::exit(3);
}
static uint32_t read32(FILE *f) {
    unsigned char b[4]; if (fread(b,1,4,f)!=4) std::exit(3);
    return b[0] | uint32_t(b[1])<<8 | uint32_t(b[2])<<16 | uint32_t(b[3])<<24;
}
struct Entry { uint32_t offset=0, count=0, loop=0; };
int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr,"usage: render-rix MUS.MKF pal-fm.pcm\n"); return 1; }
    FILE *src=fopen(argv[1],"rb"); if (!src) return 2;
    uint32_t first=read32(src), count=first/4-1;
    if (first%4 || count>128 || count<1) return 2;
    rewind(src); std::vector<uint32_t> offsets(count+1);
    for (auto &v:offsets) v=read32(src);
    fclose(src);
    FILE *out=fopen(argv[2],"wb"); if (!out) return 2;
    std::vector<Entry> entries(count);
    fwrite("PALFM01\0",1,8,out); u32(out,44100); u32(out,count);
    for (uint32_t i=0;i<count*3;i++) u32(out,0);
    for (uint32_t i=1;i<count;i++) {
        if (offsets[i+1]==offsets[i]) continue;
        // Independent instance for each song, so output never depends on order.
        OPL opl; CrixPlayer player(&opl);
        if (!player.load(argv[1],CProvider_Filesystem())) return 2;
        player.rewind(i);
        Entry &e=entries[i]; e.offset=ftell(out);
        short pcm[630]; uint32_t peak=0;
        // First pass is the intro. A second pass preserves OPL envelopes at
        // the repeat boundary; only that second pass loops during playback.
        for (int pass=0;pass<2;pass++) {
            if (pass) { e.loop=e.count; player.rewindReInit(i,false); }
            uint32_t ticks=0;
            while (player.update()) {
                if (++ticks > 70*600) { fprintf(stderr,"track %u exceeds 10 min\n",i); return 4; }
                opl.update(pcm,630);
                unsigned char bytes[1260];
                for (int j=0;j<630;j++) {
                    short s=pcm[j];
                    unsigned a=s<0 ? -int(s):s; if (a>peak) peak=a;
                    bytes[j*2]=(unsigned char)s; bytes[j*2+1]=(unsigned char)(uint16_t(s)>>8);
                }
                if (fwrite(bytes,1,sizeof bytes,out)!=sizeof bytes) return 3;
                e.count+=630;
            }
        }
        if (!e.loop || e.loop==e.count || !peak) { fprintf(stderr,"empty track %u\n",i); return 4; }
        printf("RIX %u: %.2fs, peak %u\n",i,e.loop/44100.,peak);
    }
    fseek(out,16,SEEK_SET);
    for (const auto &e:entries) { u32(out,e.offset); u32(out,e.count); u32(out,e.loop); }
    if (fclose(out)) return 3;
}
