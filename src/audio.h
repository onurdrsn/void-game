// audio.h — VOID Horror FPS
#pragma once

enum SoundID {
    SND_FOOTSTEP = 0,
    SND_GUNSHOT,
    SND_RELOAD,
    SND_ENEMY_GROWL,
    SND_ENEMY_ATTACK,
    SND_AMBIENT_HUM,
    SND_HEARTBEAT,
    SND_PLAYER_HURT,
    SND_PLAYER_DIE,
    SND_LEVEL_TRANSITION,
    SND_COUNT
};

bool  audio_init();
void  audio_shutdown();
void  audio_play(SoundID id, float volume, float pitch);
void  audio_play_3d(SoundID id, float volume, float pitch,
                    float src_x, float src_y, float src_z,
                    float listener_x, float listener_y, float listener_z);
void  audio_set_ambient(bool on);
void  audio_update(float dt);
