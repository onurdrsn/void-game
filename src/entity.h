// entity.h — VOID Horror FPS
#pragma once
#include "vmath.h"
#include "renderer.h"

enum EntityType  { ENT_PLAYER=0, ENT_CRAWLER, ENT_SHADOW, ENT_BRUTE, ENT_WRAITH };
enum EntityState { STATE_IDLE=0, STATE_ALERT, STATE_CHASE, STATE_ATTACK, STATE_DEAD,
                   STATE_TELEPORTING, STATE_INVISIBLE };
enum WeaponType  { WEAPON_FLASHLIGHT=0, WEAPON_PISTOL, WEAPON_COUNT };

static const int MAX_ENTITIES = 64;

struct Entity {
    EntityType  type;
    EntityState state;
    Vec3        position;
    Vec3        velocity;
    Vec3        facing;
    float       yaw;        // degrees
    float       pitch;      // degrees, player only
    float       health;
    float       max_health;
    AABB        bounds;
    bool        active;
    bool        on_ground;
    bool        in_water;
    float       state_timer;
    float       attack_timer;
    float       alert_timer;
    Vec3        last_known_player_pos;
    int         path_target;
    int         path_count;
    Vec3        path[8];
    WeaponType  weapon;
    int         ammo_pistol;
    int         ammo_clip;
    bool        reloading;
    float       reload_timer;
    bool        flashlight_on;
    float       footstep_timer;
    float       bob_phase;
    float       hurt_flash;
    float       visible_timer;
    float       teleport_timer;
    bool        boss;
    float       alpha;      // for wraith fade
};

extern Entity g_entities[MAX_ENTITIES];
extern int    g_entity_count;
extern int    g_player_idx;

static const int GRID_SIZE = 32;
extern bool  g_nav_grid[GRID_SIZE][GRID_SIZE];
extern Vec3  g_grid_origin;
extern float g_grid_cell_size;

void     entities_init();
void     entities_load_from_level();
void     entities_update(float dt);
void     player_update(float dt);
void     enemy_update(int idx, float dt);
void     player_shoot();
void     player_reload();
Entity*  entity_spawn(EntityType type, Vec3 pos);
void     entity_kill(int idx);
void     entity_damage(int idx, float dmg, Vec3 hit_dir);
void     nav_grid_build();
bool     nav_find_path(Vec3 from, Vec3 to, Vec3* out_wp, int max_wp, int* out_count);
Mesh     entity_build_mesh(EntityType type);
