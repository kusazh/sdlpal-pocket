/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "platform_video.h"

void PAL_PocketPresent(SDL_Surface *source, SDL_Window *window) {
    SDL_Surface *output = SDL_GetWindowSurface(window);
    if (!output) return;
    if (output->format->palette)
        SDL_SetPaletteColors(output->format->palette, source->format->palette->colors, 0, 256);
    /* Position native pixels within the full physical raster. Do not scale:
     * a 320x200 game surface gets 20 rows of border above and below it.
     * Map black after palette updates: AVI reserves index 255 for black,
     * and palette index zero need not be black in every game scene. */
    SDL_FillRect(output, NULL, SDL_MapRGB(output->format, 0, 0, 0));
    SDL_Rect dst = {(output->w - source->w) / 2,
                    (output->h - source->h) / 2, source->w, source->h};
    SDL_BlitSurface(source, NULL, output, &dst);
    SDL_UpdateWindowSurface(window);
}
