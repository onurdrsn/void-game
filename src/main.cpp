// main.cpp — VOID Horror FPS
// Entry point + Arena allocator
#include "platform.h"
#include "renderer.h"
#include "audio.h"
#include "game.h"

// ── Arena Allocator (16 MB static pool) ──────────────────────────────────────
static const int   ARENA_SIZE   = 16 * 1024 * 1024;
static unsigned char g_arena_mem[ARENA_SIZE];
static int           g_arena_off = 0;

void* arena_alloc(int bytes) {
    int aligned = (bytes + 15) & ~15;
    if(g_arena_off + aligned > ARENA_SIZE) {
        platform_fatal("Arena allocator out of memory");
    }
    void* ptr = g_arena_mem + g_arena_off;
    g_arena_off += aligned;
    return ptr;
}

void arena_reset() { g_arena_off = 0; }

// ── Entry Point ───────────────────────────────────────────────────────────────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
  int main(int /*argc*/, char** /*argv*/) {
#endif

    if(!platform_init("VOID", 1280, 720))
        platform_fatal("Failed to initialize platform");

    game_init();
    platform_capture_mouse(false); // Start with menu (no capture)

    double prev_time = platform_get_time();
    const double FIXED_DT   = 1.0 / 60.0;
    double       accumulator = 0.0;

    while(g_platform.running) {
        double now      = platform_get_time();
        double frame_dt = now - prev_time;
        prev_time = now;

        // Cap to prevent spiral of death
        if(frame_dt > 0.05) frame_dt = 0.05;
        accumulator += frame_dt;

        platform_poll_events();

        // Fixed timestep simulation
        while(accumulator >= FIXED_DT) {
            game_update((float)FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // Render at uncapped rate
        game_render();
        platform_swap_buffers();
    }

    audio_shutdown();
    renderer_shutdown();
    platform_shutdown();
    return 0;
}
