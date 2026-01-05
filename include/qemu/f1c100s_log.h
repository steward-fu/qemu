// LGPL-2.1 License
// (C) 2025 Steward Fu <steward.fu@gmail.com>

#ifndef __AW_LOG_H__
#define __AW_LOG_H__

#define TRACE_LEVEL  3
#define DEBUG_LEVEL 2
#define ERROR_LEVEL 1
#define FATAL_LEVEL 0

extern int f1c100s_debug_level;

#define trace(...) do {                         \
    if (f1c100s_debug_level >= TRACE_LEVEL) {   \
        printf("[TRACE] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define debug(...) do {                         \
    if (f1c100s_debug_level >= DEBUG_LEVEL) {   \
        printf("[DEBUG] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define error(...) do {                         \
    if (f1c100s_debug_level >= ERROR_LEVEL) {   \
        printf("[ERROR] ");                     \
        printf(__VA_ARGS__);                    \
    }                                           \
} while(0);

#define fatal(...) do {                         \
    printf("[FATAL] ");                         \
    printf(__VA_ARGS__);                        \
    exit(-1);                                   \
} while(0);

#endif
