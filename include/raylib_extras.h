#pragma once
#include <raylib.h>

#if defined(__EMSCRIPTEN__) && !defined(WASM)
// If your old code checks WASM, define it automatically under Emscripten.
#define WASM 1
#endif

#ifdef WASM
#include <emscripten/emscripten.h>
static inline void start_main_loop(void (*callback)(void)) {
    // Schedules your callback as the browser main loop.
    emscripten_set_main_loop(callback, 0, 1);
}
#else
static inline void start_main_loop(void (*callback)(void)) {
    while (!WindowShouldClose()) {
        callback();
    }
}
#endif