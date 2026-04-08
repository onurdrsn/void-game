// game.h — VOID Horror FPS
#pragma once
#include "vmath.h"
#include "renderer.h"
#include "entity.h"
#include "world.h"

enum GameState {
    GAME_MENU = 0,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_DEAD,
    GAME_LEVEL_TRANSITION,
    GAME_WIN,
};

struct GameData {
    GameState state;
    int       current_level;
    float     time_elapsed;
    float     transition_timer;
    float     death_timer;
    float     heartbeat_phase;
    float     heartbeat_pulse;
    bool      boss_dead;
    int       kills;
    float     menu_cursor;
};

extern GameData g_game;

void game_init();
void game_update(float dt);
void game_render();
void game_load_level(int index);
void game_player_died();
void game_win();
