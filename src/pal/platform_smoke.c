/* Desktop integration check: real resources, scene rendering, save roundtrip.
 * Run from a temporary working directory containing only a pal.pak symlink. */
#include "main.h"
#ifdef OF_PC
#include <unistd.h>
#include "platform_video.h"
extern SDL_Window *gpWindow;

/* Report decoded text for comparison with the user's UTF-16 originals. No
 * proprietary strings or reference fixtures are built into the executable. */
static int check_windows_text(void) {
    if (!gConfig.fIsWIN95) return 1;
    uint32_t hash=2166136261u;
    int blocks=0;
    for (int i=0; i<g_TextLib.nWords; i++) {
        LPCWSTR word=PAL_GetWord(i);
        while (*word) hash=(hash ^ (uint32_t)*word++)*16777619u;
        hash*=16777619u;
    }
    for (int i=0; i<gpGlobals->g.nScriptEntry; i++) {
        LPSCRIPTENTRY script=gpGlobals->g.lprgScriptEntry;
        if (script[i].wOperation!=0xffff) continue;
        int first=script[i].rgwOperand[0], last=first, end=i+1;
        while (end<gpGlobals->g.nScriptEntry) {
            if (script[end].wOperation==0xffff && script[end].rgwOperand[0]==last+1)
                last=script[end].rgwOperand[0];
            else if (script[end].wOperation!=0x008e) break;
            end++;
        }
        int order=0, message;
        while ((message=PAL_GetMsgNum(first,last-first,order))>=0) {
            if (++order>end-i) return 0;
            if (!message) hash=(hash ^ 0xffffffffu)*16777619u;
            else {
                LPCWSTR text=PAL_GetMsg(message);
                while (*text) hash=(hash ^ (uint32_t)*text++)*16777619u;
                hash*=16777619u;
            }
        }
        if (order!=end-i) return 0;
        blocks++;
    }
    printf("PASS: PAL98 text words=%d, script entries=%d, text hash=%08x\n",g_TextLib.nWords,blocks,hash);
    return 1;
}

static int check_display(SDL_Surface *display) {
    if (display->w != 320 || display->h != 240) {
        fprintf(stderr, "Display is not physical 320x240\n"); return 0;
    }
    for (int y = 0; y < 240; y++) for (int x = 0; x < 320; x++) {
        Uint8 *p = (Uint8 *)display->pixels + y * display->pitch + x * display->format->BytesPerPixel;
        Uint32 pixel = 0;
        memcpy(&pixel, p, display->format->BytesPerPixel);
        Uint8 r, g, b;
        SDL_GetRGB(pixel, display->format, &r, &g, &b);
        SDL_Color expected = {0, 0, 0, 255};
        if (y >= 20 && y < 220) {
            Uint8 index = *((Uint8 *)gpScreen->pixels + (y - 20) * gpScreen->pitch + x);
            expected = gpScreen->format->palette->colors[index];
        }
        if (r != expected.r || g != expected.g || b != expected.b) {
            fprintf(stderr, "Native pixel mismatch at %d,%d\n", x, y); return 0;
        }
    }
    return 1;
}

int PAL_PocketSmokeTest(void) {
    if (access(gConfig.fIsWIN95 ? "pal98-1.sav" : "pal-1.sav", F_OK) == 0) {
        fprintf(stderr, "Refusing to overwrite an existing smoke-test save\n");
        return 1;
    }
    extern int PAL_PocketAudioSmoke(void);
    if (PAL_PocketAudioSmoke()) { fprintf(stderr,"FM audio check failed\n"); return 1; }
    PAL_InitGameData(0);
    if (!check_windows_text()) { fprintf(stderr,"PAL98 dialogue mapping failed\n"); return 1; }
    gpGlobals->fInMainGame = TRUE;
    PAL_SetLoadFlags(kLoadScene | kLoadPlayerSprite);
    PAL_LoadResources();
    PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    PAL_MakeScene();
    VIDEO_UpdateScreen(NULL);
    /* Initial scene's (0,0) viewport is outside its map until the opening
     * script runs. Render a deterministic title resource for visual QA. */
    uint8_t title[64000];
    int title_chunk = gConfig.fIsWIN95 ? 0x04 : 0x27;
    if (PAL_MKFDecompressChunk(title, sizeof title, title_chunk, gpGlobals->f.fpFBP) != sizeof title) {
        fprintf(stderr, "Title decompression failed\n"); return 1;
    }
    PAL_SetPalette(1, FALSE);
    PAL_FBPBlitToSurface(title, gpScreen);
    PAL_DrawText(PAL_GetWord(7), PAL_XY(120, 145), 0x4f, TRUE, FALSE, FALSE);
    PAL_DrawText(PAL_GetWord(8), PAL_XY(120, 165), 0x4f, TRUE, FALSE, FALSE);
    VIDEO_UpdateScreen(NULL);
    if (SDL_SaveBMP(gpScreen, "title.bmp") != 0) {
        fprintf(stderr, "Screenshot failed: %s\n", SDL_GetError()); return 1;
    }
    for (int i = 0; i < 12; i++) { SDL_Delay(100); VIDEO_UpdateScreen(NULL); }
    SDL_Surface *display = SDL_GetWindowSurface(gpWindow);
    if (!check_display(display) || SDL_SaveBMP(display, "display.bmp")) return 1;
    printf("PASS: native 320x200 scene centered with equal 20-row black borders\n");
    /* Exercise every row and both image edges: title borders alone would
     * not detect a clipped or shifted black scanline. */
    SDL_BlitSurface(gpScreen, NULL, gpScreenBak, NULL);
    for (int y = 0; y < 200; y++) for (int x = 0; x < 320; x++)
        *((Uint8 *)gpScreen->pixels + y * gpScreen->pitch + x) = (x + y) % 256;
    VIDEO_UpdateScreen(NULL);
    if (!check_display(SDL_GetWindowSurface(gpWindow))) return 1;
    SDL_BlitSurface(gpScreenBak, NULL, gpScreen, NULL);
    VIDEO_UpdateScreen(NULL);
    printf("PASS: all 200 scanlines and four edges preserved without scaling\n");
    DWORD cash = 12345;
    WORD scene = gpGlobals->wNumScene;
    gpGlobals->dwCash = cash;
    PAL_SaveGame(1, 1);
    gpGlobals->dwCash = 0;
    PAL_InitGameData(1);
    if (gpGlobals->dwCash != cash || gpGlobals->wNumScene != scene) {
        fprintf(stderr, "Save roundtrip failed\n"); return 1;
    }
    printf("PASS: DOS=%d, scene=%u, save roundtrip cash=%lu, screenshot=title.bmp\n",
        !gConfig.fIsWIN95, scene, (unsigned long)gpGlobals->dwCash);
    SDL_Quit();
    return 0;
}
#endif
