/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "platform_video.h"

void PAL_PocketPresent(SDL_Surface *source, SDL_Window *window) {
    SDL_Surface *output = SDL_GetWindowSurface(window);
    if (!output) return;
    if (output->format->palette)
        SDL_SetPaletteColors(output->format->palette, source->format->palette->colors, 0, 256);
    /* Copy native pixels. The Pocket scaler handles the 4:3 display aspect
     * directly (5x horizontal, 6x vertical on the handheld LCD). */
    SDL_BlitSurface(source, NULL, output, NULL);
    SDL_UpdateWindowSurface(window);
}
