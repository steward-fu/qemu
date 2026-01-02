#ifndef SDL1_H
#define SDL1_H

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
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
    int led_status[3];
    int btn_status[3];

    SDL_Surface *tiny200;
    SDL_Surface *led_red;
    SDL_Surface *led_blue;
    SDL_Surface *led_green;
    SDL_Surface *led_white;
    SDL_Surface *led_orange;
    SDL_Surface *btn_press;
    SDL_Surface *btn_release;

    TTF_Font *font;
};

#endif
