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
#include "qemu/aw_log.h"

#define DEV_MODE_W          640
#define DEV_MODE_H          480
#define TINY200_FILE        "/opt/qemu/dev-mode/tiny200.png"
#define LED_RED_FILE        "/opt/qemu/dev-mode/led_red.png"
#define LED_BLUE_FILE       "/opt/qemu/dev-mode/led_blue.png"
#define LED_GREEN_FILE      "/opt/qemu/dev-mode/led_green.png"
#define LED_ORANGE_FILE     "/opt/qemu/dev-mode/led_orange.png"
#define LED_WHITE_FILE      "/opt/qemu/dev-mode/led_white.png"
#define BTN_PRESS_FILE      "/opt/qemu/dev-mode/btn_press.png"
#define BTN_RELEASE_FILE    "/opt/qemu/dev-mode/btn_release.png"
#define FONT_FILE           "/opt/qemu/dev-mode/font.ttf"
#define FONT_SIZE           24

#define TRACE_LEVEL 3
#define DEBUG_LEVEL 2
#define ERROR_LEVEL 1
#define FATAL_LEVEL 0

static struct sdl1_console *sdl1_console = NULL;

static void sdl1_update(DisplayChangeListener *dcl, int x, int y, int w, int h)
{
    struct sdl1_console *scon = container_of(dcl, struct sdl1_console, dcl);

    trace("call %s()\n", __func__);

    if (scon->dev_mode == 0) {
        SDL_BlitSurface(scon->guest_screen, NULL, scon->real_screen, NULL);
    }
    SDL_Flip(scon->real_screen);
}

static int sym_to_qcode(SDL_Event ev)
{
    switch (ev.key.keysym.sym) {
    case SDLK_0:    return Q_KEY_CODE_0;
    case SDLK_1:    return Q_KEY_CODE_1;
    case SDLK_2:    return Q_KEY_CODE_2;
    case SDLK_3:    return Q_KEY_CODE_3;
    case SDLK_4:    return Q_KEY_CODE_4;
    case SDLK_5:    return Q_KEY_CODE_5;
    case SDLK_6:    return Q_KEY_CODE_6;
    case SDLK_7:    return Q_KEY_CODE_7;
    case SDLK_8:    return Q_KEY_CODE_8;
    case SDLK_9:    return Q_KEY_CODE_9;
    case SDLK_a:    return Q_KEY_CODE_A;
    case SDLK_b:    return Q_KEY_CODE_B;
    case SDLK_c:    return Q_KEY_CODE_C;
    case SDLK_d:    return Q_KEY_CODE_D;
    case SDLK_e:    return Q_KEY_CODE_E;
    case SDLK_f:    return Q_KEY_CODE_F;
    case SDLK_g:    return Q_KEY_CODE_G;
    case SDLK_h:    return Q_KEY_CODE_H;
    case SDLK_i:    return Q_KEY_CODE_I;
    case SDLK_j:    return Q_KEY_CODE_J;
    case SDLK_k:    return Q_KEY_CODE_K;
    case SDLK_l:    return Q_KEY_CODE_L;
    case SDLK_m:    return Q_KEY_CODE_M;
    case SDLK_n:    return Q_KEY_CODE_N;
    case SDLK_o:    return Q_KEY_CODE_O;
    case SDLK_p:    return Q_KEY_CODE_P;
    case SDLK_q:    return Q_KEY_CODE_Q;
    case SDLK_r:    return Q_KEY_CODE_R;
    case SDLK_s:    return Q_KEY_CODE_S;
    case SDLK_t:    return Q_KEY_CODE_T;
    case SDLK_u:    return Q_KEY_CODE_U;
    case SDLK_v:    return Q_KEY_CODE_V;
    case SDLK_w:    return Q_KEY_CODE_W;
    case SDLK_x:    return Q_KEY_CODE_X;
    case SDLK_y:    return Q_KEY_CODE_Y;
    case SDLK_z:    return Q_KEY_CODE_Z;
    default:        return Q_KEY_CODE_0;
    }

    return Q_KEY_CODE_0;
}

static int draw_string(struct sdl1_console *scon, int x, int y, const char *text)
{
    SDL_Rect rt = { 0 };
    SDL_Color col = { 0, 0, 255 };

    rt.x = x;
    rt.y = y;
    SDL_Surface *msg = TTF_RenderUTF8_Solid(scon->font, text, col);
    SDL_BlitSurface(msg, NULL, scon->real_screen, &rt);
    SDL_FreeSurface(msg);

    return 0;
}

static int draw_dev_item(struct sdl1_console *scon)
{
    int i = 0;
    SDL_Rect srt = { 0 };
    SDL_Rect drt = { 0 };

    SDL_FillRect(
        scon->real_screen,
        &scon->real_screen->clip_rect,
        SDL_MapRGB(scon->real_screen->format, 0xff, 0xff, 0xff)
    );

    for (i = 0; i < 3; i++) {
        drt.x = (i * 100);
        drt.y = 45;
        SDL_BlitSurface(
            (aw_shm.pe & (1 << i)) ? scon->led_red : scon->led_white,
            NULL,
            scon->real_screen,
            &drt
        );

        drt.x = 325 + (i * 100);
        drt.y = 65;
        SDL_BlitSurface(
            (aw_shm.pe & (1 << (3 + i))) ? scon->btn_press : scon->btn_release,
            NULL,
            scon->real_screen,
            &drt
        );
    }

    SDL_Surface *t = SDL_ConvertSurface(scon->tiny200, scon->real_screen->format, 0);

    srt.x = 0;
    srt.y = 0;
    srt.w = t->w;
    srt.h = t->h;
    drt.w = t->w >> 2;
    drt.h = t->h >> 2;
    drt.x = (DEV_MODE_W - drt.w) >> 1;
    drt.y = 250;
    SDL_SoftStretch(t, &srt, scon->real_screen, &drt);
    SDL_FreeSurface(t);

    draw_string(scon, 40, 170, "PE0");
    draw_string(scon, 140, 170, "PE1");
    draw_string(scon, 240, 170, "PE2");
    draw_string(scon, 353, 170, "PE3");
    draw_string(scon, 453, 170, "PE4");
    draw_string(scon, 553, 170, "PE5");

    int w = 0;
    int h = 0;
    const char *model = "Allwinner F1C100S";
    TTF_SizeUTF8(scon->font, model, &w, &h);
    draw_string(scon, (DEV_MODE_W - w) >> 1, 395, model);

    return 0;
}

static void sdl1_refresh(DisplayChangeListener *dcl)
{
    int x = 0;
    int y = 0;
    int r = 35;
    int qcode = 0;
    int pressed = 0;
    SDL_Event ev = { 0 };
    struct sdl1_console *scon = container_of(dcl, struct sdl1_console, dcl);
    QemuConsole *con = scon->dcl.con;

    trace("call %s()\n", __func__);

    if (scon->dev_mode) {
        draw_dev_item(scon);
    }

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            qcode = sym_to_qcode(ev);
            qkbd_state_key_event(scon->kbd, qcode, ev.type == SDL_KEYDOWN);
            break;
        case SDL_MOUSEMOTION:
            SDL_GetMouseState(&x, &y);
            trace("%d, %d\n", x, y);
            break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            pressed = (ev.type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
            SDL_GetMouseState(&x, &y);
            if (((x >= 355) && (x <= (355 + r))) && (y >= 95) && (y <= 95 + r)) {
                aw_shm.pe &= ~(1 << 3);
                trace("BTN 1 (X=%d, Y=%d, PRESS=%d)\n", x, y, pressed);
            }
            else if (((x >= 455) && (x <= (455 + r))) && (y >= 95) && (y <= 95 + r)) {
                aw_shm.pe &= ~(1 << 4);
                trace("BTN 2 (X=%d, Y=%d, PRESS=%d)\n", x, y, pressed);
            }
            else if (((x >= 555) && (x <= (555 + r))) && (y >= 95) && (y <= 95 + r)) {
                aw_shm.pe &= ~(1 << 5);
                trace("BTN 3 (X=%d, Y=%d, PRESS=%d)\n", x, y, pressed);
            }
            break;
        case SDL_QUIT:
            trace("shutdown from SDL\n");
            qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
            qemu_system_shutdown_request(SHUTDOWN_CAUSE_HOST_UI);
            break;
        }
    }

    if (!qemu_console_is_graphic(con)) {
        bool ctrl = qkbd_state_modifier_get(scon->kbd, QKBD_MOD_CTRL);
        if (ev.type == SDL_KEYDOWN) {
            switch (qcode) {
            case Q_KEY_CODE_RET:
                kbd_put_keysym_console(con, '\n');
                break;
            default:
                kbd_put_qcode_console(con, qcode, ctrl);
                break;
            }
        }
    }

    graphic_hw_update(dcl->con);
}

static void sdl1_window_resize(struct sdl1_console *scon)
{
    int w = surface_width(scon->surface);
    int h = surface_height(scon->surface);

    trace("call %s(w=%d, h=%d)\n", __func__, w, h);

    if (scon->dev_mode) {
        w = DEV_MODE_W;
        h = DEV_MODE_H;
    }
    scon->real_screen = SDL_SetVideoMode(w, h, 16, SDL_HWSURFACE);
}

static void sdl1_window_create(struct sdl1_console *scon)
{
    char *dir = NULL;
    SDL_Surface *icon = NULL;
    int w = surface_width(scon->surface);
    int h = surface_height(scon->surface);

    trace("call %s(w=%d, h=%d)\n", __func__, w, h);

    if (scon->dev_mode) {
        w = DEV_MODE_W;
        h = DEV_MODE_H;

        scon->tiny200 = IMG_Load(TINY200_FILE);
        scon->led_red = IMG_Load(LED_RED_FILE);
        scon->led_blue = IMG_Load(LED_BLUE_FILE);
        scon->led_green = IMG_Load(LED_GREEN_FILE);
        scon->led_orange = IMG_Load(LED_ORANGE_FILE);
        scon->led_white = IMG_Load(LED_WHITE_FILE);
        scon->btn_press = IMG_Load(BTN_PRESS_FILE);
        scon->btn_release = IMG_Load(BTN_RELEASE_FILE);
    }
    scon->real_screen = SDL_SetVideoMode(w, h, 16, SDL_HWSURFACE);

    if (scon->dev_mode) {
        SDL_WM_SetCaption("QEMU Development Mode", "qemu");
    }
    else {
        SDL_WM_SetCaption("QEMU", "qemu");
    }

    dir = get_relocated_path(CONFIG_QEMU_ICONDIR "/hicolor/128x128/apps/qemu.png");
    icon = IMG_Load(dir);
    g_free(dir);

    if (icon) {
        SDL_WM_SetIcon(icon, NULL);
    }

    TTF_Init();
    scon->font = TTF_OpenFont(FONT_FILE, FONT_SIZE);
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

    if (scon->dev_mode) {
        SDL_FillRect(
            scon->real_screen,
            &scon->real_screen->clip_rect,
            SDL_MapRGB(scon->real_screen->format, 0xff, 0xff, 0xff)
        );
    }
}

static void sdl1_cleanup(void)
{
    struct sdl1_console *scon = sdl1_console;

    trace("call %s()\n", __func__);

    if (scon->guest_screen) {
        SDL_FreeSurface(scon->guest_screen);
    }
    if (scon->tiny200) {
        SDL_FreeSurface(scon->tiny200);
    }
    if (scon->led_red) {
        SDL_FreeSurface(scon->led_red);
    }
    if (scon->led_blue) {
        SDL_FreeSurface(scon->led_blue);
    }
    if (scon->led_green) {
        SDL_FreeSurface(scon->led_green);
    }
    if (scon->led_orange) {
        SDL_FreeSurface(scon->led_orange);
    }
    if (scon->led_white) {
        SDL_FreeSurface(scon->led_white);
    }
    if (scon->btn_press) {
        SDL_FreeSurface(scon->btn_press);
    }
    if (scon->btn_release) {
        SDL_FreeSurface(scon->btn_release);
    }
    if (scon->font) {
        TTF_CloseFont(scon->font);
    }
    TTF_Quit();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static bool sdl1_check_format(DisplayChangeListener *dcl, pixman_format_code_t format)
{
    trace("call %s()\n", __func__);

    return format == PIXMAN_r5g6b5;
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
    const char *env = getenv("QEMU_DEV_MODE");

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
    sdl1_console->kbd = qkbd_state_init(con);

    if (env && !strcmp(env, "1")) {
        trace("dev mode\n");
        sdl1_console->dev_mode = 1;
    }
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
