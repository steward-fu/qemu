#ifndef SDL1_H
#define SDL1_H

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#ifdef CONFIG_SDL_IMAGE
# include <SDL/SDL_image.h>
#endif

#include "ui/kbd-state.h"

struct sdl1_console {
    DisplayGLCtx dgc;
    DisplayOptions *opts;
    DisplaySurface *surface;
    DisplayChangeListener dcl;
    SDL_Surface *real_screen;
    SDL_Surface *guest_screen;
    QKbdState *kbd;

    int dev_mode;
};

#endif
