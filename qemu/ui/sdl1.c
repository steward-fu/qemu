/*
 * Copyright (c) 2025 Steward <steward.fu@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/cutils.h"
#include "ui/console.h"
#include "ui/input.h"
#include "ui/sdl1.h"
#include "ui/keymaps.h"
#include "sysemu/runstate.h"
#include "sysemu/runstate-action.h"
#include "sysemu/sysemu.h"
#include "ui/win32-kbd-hook.h"
#include "qemu/log.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#define TRACE_LEVEL 3
#define DEBUG_LEVEL 2
#define ERROR_LEVEL 1
#define FATAL_LEVEL 0

static int nds_debug_level = FATAL_LEVEL;

#define trace(...) do {                         \
    if (nds_debug_level >= TRACE_LEVEL) {       \
        printf("[TRACE] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define debug(...) do {                         \
    if (nds_debug_level >= DEBUG_LEVEL) {       \
        printf("[DEBUG] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define error(...) do {                         \
    if (nds_debug_level >= ERROR_LEVEL) {       \
        printf("[ERROR] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define fatal(...) do {                         \
    printf("[FATAL] ");                         \
    printf(__VA_ARGS__);                        \
    exit(-1);                                   \
} while(0);

static struct sdl1_console *sdl1_console = NULL;

static void sdl1_update(DisplayChangeListener *dcl, int x, int y, int w, int h)
{
    struct sdl1_console *scon = container_of(dcl, struct sdl1_console, dcl);

    trace("call %s()\n", __func__);
    SDL_BlitSurface(scon->guest_screen, NULL, scon->real_screen, NULL);
    SDL_Flip(scon->real_screen);
}

static void sdl1_refresh(DisplayChangeListener *dcl)
{
    uint8_t keycode = 0;
    SDL_Event ev = { 0 };

    trace("call %s()\n", __func__);
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_KEYDOWN:
            //kbd_put_keysym(ev.key.keysym.sym);
            //keycode = keysym2scancode(kbd_layout, ev.key.keysym.sym & 0xffff, ev->type == SDL_KEYDOWN);
            //qemu_input_event_send_key_number(dcl->con, keycode, ev->type == SDL_KEYDOWN);
            break;
        case SDL_KEYUP:
            //qemu_input_event_send_key_number(dcl->con, keycode, ev->type == SDL_KEYDOWN);
            break;
        case SDL_QUIT:
            qemu_system_shutdown_request(SHUTDOWN_CAUSE_HOST_UI);
            break;
        }
    }
}

static void sdl1_window_resize(struct sdl1_console *scon)
{
    int w = surface_width(scon->surface);
    int h = surface_height(scon->surface);

    trace("call %s(w=%d, h=%d)\n", __func__, w, h);
    scon->real_screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE);
}

static void sdl1_window_create(struct sdl1_console *scon)
{
    int w = surface_width(scon->surface);
    int h = surface_height(scon->surface);

    trace("call %s(w=%d, h=%d)\n", __func__, w, h);
    scon->real_screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE);
}

static void sdl1_switch(DisplayChangeListener *dcl, DisplaySurface *new_surface)
{
    int w = 0;
    int h  =0;
    PixelFormat pf = { 0 };
    struct sdl1_console *scon = container_of(dcl, struct sdl1_console, dcl);
    DisplaySurface *old_surface = scon->surface;

    scon->surface = new_surface;
    w = surface_width(new_surface);
    h = surface_height(new_surface);
    pf = qemu_pixelformat_from_pixman(new_surface->format);

    trace("call %s(w=%d, h=%d, new_surface=%p)\n", __func__, w, h, new_surface);

    if (!scon->real_screen) {
        sdl1_window_create(scon);
    } else if (old_surface &&
        ((surface_width(old_surface)  != surface_width(new_surface)) ||
        (surface_height(old_surface) != surface_height(new_surface))))
    {
        sdl1_window_resize(scon);
    }

    if (scon->guest_screen != NULL) {
        SDL_FreeSurface(scon->guest_screen);
    }

    scon->guest_screen = SDL_CreateRGBSurfaceFrom(
        surface_data(new_surface),
        surface_width(new_surface),
        surface_height(new_surface),
        surface_bits_per_pixel(new_surface),
        surface_stride(new_surface),
        pf.rmask,
        pf.gmask,
        pf.bmask,
        pf.amask
    );
}

static void sdl1_grab_start(struct sdl1_console *scon)
{
    trace("call %s()\n", __func__);
}

static void sdl1_cleanup(void)
{
    trace("call %s()\n", __func__);

    if (sdl1_console->guest_screen != NULL) {
        SDL_FreeSurface(sdl1_console->guest_screen);
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static bool sdl1_check_format(DisplayChangeListener *dcl, pixman_format_code_t format)
{
    trace("call %s()\n", __func__);

    return format == PIXMAN_r5g6b5;
}

static void sdl1_mouse_mode_change(Notifier *notify, void *data)
{
    trace("call %s()\n", __func__);
}

static void sdl1_mouse_warp(DisplayChangeListener *dcl, int x, int y, int on)
{
    trace("call %s()\n", __func__);
}

static void sdl1_mouse_define(DisplayChangeListener *dcl, QEMUCursor *c)
{
    trace("call %s()\n", __func__);
}

static const DisplayChangeListenerOps dcl_ops = {
    .dpy_name             = "sdl1",
    .dpy_gfx_update       = sdl1_update,
    .dpy_gfx_switch       = sdl1_switch,
    .dpy_gfx_check_format = sdl1_check_format,
    .dpy_refresh          = sdl1_refresh,
    .dpy_mouse_set        = sdl1_mouse_warp,
    .dpy_cursor_define    = sdl1_mouse_define,
};

static void sdl1_display_init(DisplayState *ds, DisplayOptions *o)
{
    trace("call %s()\n", __func__);

    if (SDL_Init(SDL_INIT_VIDEO)) {
        fatal("failed to initialize SDL(%s) - exiting\n", SDL_GetError());
        exit(1);
    }

    QemuConsole *con = qemu_console_lookup_by_index(0);
    sdl1_console = g_new0(struct sdl1_console, 1);
    sdl1_console->opts = o;
    sdl1_console->dcl.con = con;
    sdl1_console->dcl.ops = &dcl_ops;
    register_displaychangelistener(&sdl1_console->dcl);

    atexit(sdl1_cleanup);
}

static void sdl1_display_early_init(DisplayOptions *o)
{
    trace("call %s()\n", __func__);
}

static QemuDisplay qemu_display_sdl1 = {
    .type = DISPLAY_TYPE_SDL1,
    .init = sdl1_display_init,
    .early_init = sdl1_display_early_init,
};

static void register_sdl1(void)
{
    trace("call %s()\n", __func__);

    qemu_display_register(&qemu_display_sdl1);
}

type_init(register_sdl1);
