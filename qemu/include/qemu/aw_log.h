/*
 * Copyright (C) 2025 Steward <steward.fu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef QEMU_AW_LOG_H
#define QEMU_AW_LOG_H

#include "hw/gpio/f1c100s.h"

#define TRACE_LEVEL 3
#define DEBUG_LEVEL 2
#define ERROR_LEVEL 1
#define FATAL_LEVEL 0

extern int aw_debug_level;

#define trace(...) do {                         \
    if (aw_debug_level >= TRACE_LEVEL) {        \
        printf("[TRACE] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define debug(...) do {                         \
    if (aw_debug_level >= DEBUG_LEVEL) {        \
        printf("[DEBUG] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define error(...) do {                         \
    if (aw_debug_level >= ERROR_LEVEL) {        \
        printf("[ERROR] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define fatal(...) do {                         \
    printf("[FATAL] ");                         \
    printf(__VA_ARGS__);                        \
    exit(-1);                                   \
} while(0);

struct aw_shm_t {
    struct f1c100s_gpio_t pe;
};

extern struct aw_shm_t aw_shm;

#endif
