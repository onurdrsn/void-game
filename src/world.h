// world.h — VOID Horror FPS
#pragma once
#include "vmath.h"
#include "renderer.h"

static const int MAX_BRUSHES   = 512;
static const int MAX_TRIGGERS  = 32;
static const int MAX_SPAWNS    = 32;
static const int LEVEL_COUNT   = 3;

enum BrushFlags {
    BRUSH_SOLID     = 1 << 0,
    BRUSH_TRIGGER   = 1 << 1,
    BRUSH_WATER     = 1 << 2,
    BRUSH_DOOR      = 1 << 3,
    BRUSH_NOCOLLIDE = 1 << 4,
};

struct Brush {
    AABB      bounds;
    TextureID tex_id;
    int       flags;
    float     door_open;   // 0.0=closed, 1.0=open
    int       trigger_id;  // -1=none
};

enum TriggerAction {
    TRIGGER_NONE = 0,
    TRIGGER_OPEN_DOOR,
    TRIGGER_LOAD_LEVEL,
    TRIGGER_SPAWN_ENEMY,
    TRIGGER_PLAY_SOUND,
};

struct Trigger {
    AABB         bounds;
    TriggerAction action;
    int          param;
    bool         fired;
    bool         repeatable;
};

struct SpawnPoint {
    Vec3 position;
    int  entity_type;   // 0=player, 1-4=enemy types
    int  facing_deg;
};

struct Level {
    Brush      brushes[MAX_BRUSHES];
    int        brush_count;
    Trigger    triggers[MAX_TRIGGERS];
    int        trigger_count;
    SpawnPoint spawns[MAX_SPAWNS];
    int        spawn_count;
    Vec3       ambient_light;
    float      fog_start;
    float      fog_end;
    int        index;
};

extern Level g_level;

void level_load(int index);
void level_update(float dt);
bool level_collide(const AABB& mover, Vec3& position, Vec3& velocity);
void level_check_triggers(const Vec3& player_pos, int* action_out, int* param_out);
Mesh level_build_mesh();
