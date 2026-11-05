/* Desktop integration check: real resources, scene rendering, save roundtrip.
 * Run from a temporary working directory containing only a pal.pak symlink. */
#include "main.h"
#ifdef OF_PC
#include <unistd.h>
#include "platform_video.h"
extern SDL_Window *gpWindow;

static int check_display(SDL_Surface *display) {
    if (display->w != 320 || display->h != 200) {
        fprintf(stderr, "Display is not native 320x200\n"); return 0;
    }
    for (int y = 0; y < 200; y++) for (int x = 0; x < 320; x++) {
        Uint8 index = *((Uint8 *)gpScreen->pixels + y * gpScreen->pitch + x);
        Uint8 *p = (Uint8 *)display->pixels + y * display->pitch + x * display->format->BytesPerPixel;
        Uint32 pixel = 0;
        memcpy(&pixel, p, display->format->BytesPerPixel);
        Uint8 r, g, b;
        SDL_GetRGB(pixel, display->format, &r, &g, &b);
        SDL_Color expected = gpScreen->format->palette->colors[index];
        if (r != expected.r || g != expected.g || b != expected.b) {
            fprintf(stderr, "Native pixel mismatch at %d,%d\n", x, y); return 0;
        }
    }
    return 1;
}

int PAL_PocketSmokeTest(void) {
    if (access("pal-1.sav", F_OK) == 0) {
        fprintf(stderr, "Refusing to overwrite an existing smoke-test save\n");
        return 1;
    }
    extern int PAL_PocketAudioSmoke(void);
    if (PAL_PocketAudioSmoke()) { fprintf(stderr,"FM audio check failed\n"); return 1; }
    PAL_InitGameData(0);
    gpGlobals->fInMainGame = TRUE;
    PAL_SetLoadFlags(kLoadScene | kLoadPlayerSprite);
    PAL_LoadResources();
    PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    PAL_MakeScene();
    VIDEO_UpdateScreen(NULL);
    /* Initial scene's (0,0) viewport is outside its map until the opening
     * script runs. Render a deterministic title resource for visual QA. */
    uint8_t title[64000];
    if (PAL_MKFDecompressChunk(title, sizeof title, 0x27, gpGlobals->f.fpFBP) != sizeof title) {
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
    printf("PASS: native 320x200 pixels, no debug overlay\n");
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
