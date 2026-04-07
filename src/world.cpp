// world.cpp — VOID Horror FPS
// Level geometry, collision, triggers, mesh generation
#include "world.h"
#include "vmath.h"
#include "renderer.h"
#include <cstring>
#include <cstdio>

Level g_level = {};

// ── Mesh vertex pool (static, no heap) ───────────────────────────────────────
// Max 512 brushes × 6 faces × 6 verts × 8 floats
static const int MAX_WORLD_VERTS = 512 * 6 * 6;
static float s_world_verts[MAX_WORLD_VERTS * 8];
static int   s_world_vert_count = 0;

// ─────────────────────────────────────────────────────────────────────────────
// BRUSH HELPER MACROS
// ─────────────────────────────────────────────────────────────────────────────
static int s_bc = 0;   // brush count during load
static int s_tc = 0;   // trigger count
static int s_sc = 0;   // spawn count

static void add_brush(float x0,float y0,float z0, float x1,float y1,float z1,
                      TextureID tex, int flags, int trigger_id=-1) {
    if(s_bc>=MAX_BRUSHES) return;
    Brush& b = g_level.brushes[s_bc++];
    b.bounds    = AABB(Vec3(x0,y0,z0), Vec3(x1,y1,z1));
    b.tex_id    = tex;
    b.flags     = flags;
    b.door_open = 0.f;
    b.trigger_id= trigger_id;
}

static void add_trigger(float x0,float y0,float z0, float x1,float y1,float z1,
                         TriggerAction action, int param, bool rep=false) {
    if(s_tc>=MAX_TRIGGERS) return;
    Trigger& t   = g_level.triggers[s_tc++];
    t.bounds     = AABB(Vec3(x0,y0,z0), Vec3(x1,y1,z1));
    t.action     = action;
    t.param      = param;
    t.fired      = false;
    t.repeatable = rep;
}

static void add_spawn(float x,float y,float z, int etype, int face) {
    if(s_sc>=MAX_SPAWNS) return;
    SpawnPoint& sp = g_level.spawns[s_sc++];
    sp.position    = Vec3(x,y,z);
    sp.entity_type = etype;
    sp.facing_deg  = face;
}

// ─────────────────────────────────────────────────────────────────────────────
// LEVEL 0 — "The Lab"
// ─────────────────────────────────────────────────────────────────────────────
static void load_level0() {
    s_bc=s_tc=s_sc=0;

    // Outer shell
    add_brush(-20,-0.5f,-20, 20,0,20,       TEX_FLOOR,    BRUSH_SOLID);           // floor
    add_brush(-20, 6,-20,    20,6.5f,20,     TEX_CONCRETE, BRUSH_SOLID);           // ceiling
    add_brush(-20, 0,-20,    20,6,-19.5f,    TEX_CONCRETE, BRUSH_SOLID);           // wall N
    add_brush(-20, 0,19.5f,  20,6,20,        TEX_CONCRETE, BRUSH_SOLID);           // wall S
    add_brush(19.5f,0,-20,   20,6,20,        TEX_CONCRETE, BRUSH_SOLID);           // wall E
    add_brush(-20, 0,-20,   -19.5f,6,20,     TEX_CONCRETE, BRUSH_SOLID);           // wall W

    // Central corridor dividers
    add_brush(-5,0,-20,   -4.5f,6,-2,       TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-5,0, 2,    -4.5f,6,20,       TEX_CONCRETE, BRUSH_SOLID);
    add_brush( 4.5f,0,-20, 5,6,-2,          TEX_CONCRETE, BRUSH_SOLID);
    add_brush( 4.5f,0, 2,  5,6,20,          TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-5,3,-2,     5,6,2,           TEX_CONCRETE, BRUSH_SOLID);  // door lintel

    // East research room
    add_brush(5,0,-12,  19.5f,6,-11.5f,     TEX_METAL, BRUSH_SOLID);
    add_brush(5,0,-5,   19.5f,6,-4.5f,      TEX_METAL, BRUSH_SOLID);
    add_brush(17,0,-11.5f, 19.5f,1.5f,-5,   TEX_PANEL, BRUSH_NOCOLLIDE);

    // West research room
    add_brush(-19.5f,0,2, -5,6,2.5f,        TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-19.5f,0,12,-5,6,12.5f,       TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-15,0.01f,-8,-10,0.02f,-5,    TEX_BLOOD, BRUSH_NOCOLLIDE);

    // Exit door (index s_bc) — DOOR brush, opens via trigger
    int door_idx = s_bc;
    add_brush(-0.5f,0,18,  0.5f,3,19.5f,   TEX_METAL, BRUSH_SOLID|BRUSH_DOOR);

    // Triggers
    add_trigger(5,0,-12, 10,3,-5,  TRIGGER_SPAWN_ENEMY, 1);           // spawn crawler
    add_trigger(-3,0,15,  3,3,18,  TRIGGER_OPEN_DOOR,   door_idx);    // open exit
    add_trigger(-1,0,19,  1,3,20,  TRIGGER_LOAD_LEVEL,  1);           // next level

    // Spawn points: 0=player,1=crawler,2=shadow,3=brute,4=wraith
    add_spawn(0,1,-15,   0,   0);
    add_spawn(12,0.3f,-8,1, 180);
    add_spawn(-12,0.5f,8,2,  90);
    add_spawn(0,0.8f,10, 3,   0);

    g_level.ambient_light = Vec3(0.04f,0.04f,0.05f);
    g_level.fog_start = 5.f;
    g_level.fog_end   = 30.f;
    g_level.index     = 0;
    g_level.brush_count   = s_bc;
    g_level.trigger_count = s_tc;
    g_level.spawn_count   = s_sc;
}

// ─────────────────────────────────────────────────────────────────────────────
// LEVEL 1 — "The Tunnels"
// ─────────────────────────────────────────────────────────────────────────────
static void load_level1() {
    s_bc=s_tc=s_sc=0;

    // Main horizontal tunnel
    add_brush(-30,-0.5f,-4,  10,0,4,        TEX_FLOOR,    BRUSH_SOLID);
    add_brush(-30, 3.5f,-4,  10,4,4,        TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-30, 0,-4,     10,3.5f,-3.5f, TEX_CONCRETE, BRUSH_SOLID); // wall N
    add_brush(-30, 0, 3.5f,  10,3.5f,4,     TEX_CONCRETE, BRUSH_SOLID); // wall S

    // Vertical branch tunnel
    add_brush(-4,-0.5f,-30,  4,0,-4,        TEX_FLOOR,    BRUSH_SOLID);
    add_brush(-4, 3.5f,-30,  4,4,-4,        TEX_CONCRETE, BRUSH_SOLID);
    add_brush(-4, 0,-30,    -3.5f,3.5f,-4,  TEX_CONCRETE, BRUSH_SOLID); // wall W
    add_brush( 3.5f,0,-30,   4,3.5f,-4,     TEX_CONCRETE, BRUSH_SOLID); // wall E

    // Water pool
    add_brush(-20,-0.5f,-4, -12,0.3f,4,     TEX_FLOOR, BRUSH_SOLID|BRUSH_WATER);

    // Debris
    add_brush(-8,0,-2, -6,1.5f,2,           TEX_CONCRETE, BRUSH_SOLID);
    add_brush( 2,0,-3,  4,2,3,              TEX_METAL,    BRUSH_SOLID);

    // Pillars
    add_brush(-24,0,-0.4f,-23.6f,3.5f,0.4f, TEX_METAL, BRUSH_SOLID);
    add_brush(-18,0,-0.4f,-17.6f,3.5f,0.4f, TEX_METAL, BRUSH_SOLID);
    add_brush(-12,0,-0.4f,-11.6f,3.5f,0.4f, TEX_METAL, BRUSH_SOLID);
    add_brush(-6, 0,-0.4f,-5.6f, 3.5f,0.4f, TEX_METAL, BRUSH_SOLID);

    // Cap walls for horizontal tunnel ends
    add_brush(-30,0,-4,-29.5f,3.5f,4,       TEX_CONCRETE, BRUSH_SOLID);
    add_brush(9.5f,0,-4,10,3.5f,4,          TEX_CONCRETE, BRUSH_SOLID);
    // Cap for vertical tunnel
    add_brush(-4,0,-30,4,3.5f,-29.5f,       TEX_CONCRETE, BRUSH_SOLID);

    // Exit door
    int door_idx = s_bc;
    add_brush(-0.5f,0,-29.5f, 0.5f,3,-28,  TEX_METAL, BRUSH_SOLID|BRUSH_DOOR);

    add_trigger(-2,0,-28, 2,3,-26, TRIGGER_LOAD_LEVEL, 2);

    add_spawn(8,1,0,     0, 180);
    add_spawn(-16,0.3f,2,1, 90);
    add_spawn(-22,0.3f,-1,1,180);
    add_spawn(-3,0.5f,-15,4, 0);
    add_spawn(-2,0.8f,-25,3, 0);

    (void)door_idx;
    g_level.ambient_light = Vec3(0.03f,0.03f,0.04f);
    g_level.fog_start = 3.f;
    g_level.fog_end   = 20.f;
    g_level.index     = 1;
    g_level.brush_count   = s_bc;
    g_level.trigger_count = s_tc;
    g_level.spawn_count   = s_sc;
}

// ─────────────────────────────────────────────────────────────────────────────
// LEVEL 2 — "The Reactor"
// ─────────────────────────────────────────────────────────────────────────────
static void load_level2() {
    s_bc=s_tc=s_sc=0;

    add_brush(-25,-0.5f,-25, 25,0,25,        TEX_METAL, BRUSH_SOLID);  // floor
    add_brush(-25,10,-25,    25,10.5f,25,     TEX_METAL, BRUSH_SOLID);  // ceiling

    // 8 walls (octagonal approx)
    add_brush(-18,0,-25, 18,10,-24.5f,        TEX_METAL, BRUSH_SOLID);
    add_brush(-18,0,24.5f,18,10,25,           TEX_METAL, BRUSH_SOLID);
    add_brush(-25,0,-18,-24.5f,10,18,         TEX_METAL, BRUSH_SOLID);
    add_brush(24.5f,0,-18,25,10,18,           TEX_METAL, BRUSH_SOLID);
    add_brush(-25,0,-25,-17,10,-17,           TEX_METAL, BRUSH_SOLID);  // NW corner
    add_brush(17,0,-25, 25,10,-17,            TEX_METAL, BRUSH_SOLID);  // NE corner
    add_brush(-25,0,17,-17,10,25,             TEX_METAL, BRUSH_SOLID);  // SW corner
    add_brush(17,0,17, 25,10,25,             TEX_METAL, BRUSH_SOLID);  // SE corner

    // Reactor core platform
    add_brush(-4,0,-4, 4,0.8f,4,             TEX_PANEL, BRUSH_SOLID);

    // Cover pillars
    add_brush(-10,0,-10,-8,5,-8,              TEX_METAL, BRUSH_SOLID);
    add_brush(8,0,-10,  10,5,-8,             TEX_METAL, BRUSH_SOLID);
    add_brush(-10,0,8,  -8,5,10,             TEX_METAL, BRUSH_SOLID);
    add_brush(8,0,8,    10,5,10,             TEX_METAL, BRUSH_SOLID);

    // Exit door (unlocked when boss dies, via game.cpp logic)
    int door_idx = s_bc;
    add_brush(-1.5f,0,-25, 1.5f,4,-24,       TEX_METAL, BRUSH_SOLID|BRUSH_DOOR);
    (void)door_idx;

    add_spawn(0,1,22,     0, 0);
    add_spawn(0,0.8f,0,   3, 0);   // boss brute
    add_spawn(-15,0.5f,0, 2, 90);
    add_spawn(15,0.5f,0,  2, 270);
    add_spawn(0,0.3f,-15, 1, 0);
    add_spawn(-10,0.3f,10,1, 45);

    g_level.ambient_light = Vec3(0.08f,0.04f,0.02f);
    g_level.fog_start = 8.f;
    g_level.fog_end   = 40.f;
    g_level.index     = 2;
    g_level.brush_count   = s_bc;
    g_level.trigger_count = s_tc;
    g_level.spawn_count   = s_sc;
}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLIC API
// ─────────────────────────────────────────────────────────────────────────────
void level_load(int index) {
    memset(&g_level, 0, sizeof(g_level));
    switch(index) {
        case 0: load_level0(); break;
        case 1: load_level1(); break;
        case 2: load_level2(); break;
        default: load_level0(); break;
    }
}

void level_update(float dt) {
    // Animate open doors
    for(int i=0;i<g_level.brush_count;i++){
        Brush& b = g_level.brushes[i];
        if(b.flags & BRUSH_DOOR){
            if(b.door_open > 0.f && b.door_open < 1.f)
                b.door_open = math_clamp(b.door_open + dt * 0.5f, 0.f, 1.f);
        }
    }
}

// AABB vs solid brush sweep collision (simple displacement method)
bool level_collide(const AABB& mover, Vec3& position, Vec3& velocity) {
    bool hit = false;
    AABB world_bounds;
    world_bounds.min = position + mover.min;
    world_bounds.max = position + mover.max;

    for(int i=0;i<g_level.brush_count;i++){
        const Brush& b = g_level.brushes[i];
        if(!(b.flags & BRUSH_SOLID)) continue;
        if(b.flags & BRUSH_NOCOLLIDE) continue;
        // Skip open doors
        if((b.flags & BRUSH_DOOR) && b.door_open >= 0.9f) continue;

        if(!world_bounds.intersects(b.bounds)) continue;

        // Find smallest overlap axis and push out
        float ox0 = world_bounds.max.x - b.bounds.min.x;
        float ox1 = b.bounds.max.x - world_bounds.min.x;
        float oy0 = world_bounds.max.y - b.bounds.min.y;
        float oy1 = b.bounds.max.y - world_bounds.min.y;
        float oz0 = world_bounds.max.z - b.bounds.min.z;
        float oz1 = b.bounds.max.z - world_bounds.min.z;

        float ox = math_minf(ox0,ox1);
        float oy = math_minf(oy0,oy1);
        float oz = math_minf(oz0,oz1);

        if(ox<=oy && ox<=oz){
            float dir = (ox0<ox1) ? -1.f : 1.f;
            position.x += dir * ox;
            velocity.x = 0;
        } else if(oy<=ox && oy<=oz){
            float dir = (oy0<oy1) ? -1.f : 1.f;
            position.y += dir * oy;
            velocity.y = 0;
        } else {
            float dir = (oz0<oz1) ? -1.f : 1.f;
            position.z += dir * oz;
            velocity.z = 0;
        }

        // Recompute bounds after push
        world_bounds.min = position + mover.min;
        world_bounds.max = position + mover.max;
        hit = true;
    }
    return hit;
}

void level_check_triggers(const Vec3& player_pos, int* action_out, int* param_out) {
    *action_out = TRIGGER_NONE;
    *param_out  = -1;
    for(int i=0;i<g_level.trigger_count;i++){
        Trigger& t = g_level.triggers[i];
        if(t.fired && !t.repeatable) continue;
        if(t.bounds.containsPoint(player_pos)){
            t.fired = true;
            *action_out = (int)t.action;
            *param_out  = t.param;
            return;
        }
    }
}

// ── Mesh generation ───────────────────────────────────────────────────────────
// For each non-NOCOLLIDE brush, emit 6 faces × 2 triangles × 3 verts
// Vertex format: pos(3) + normal(3) + uv(2) = 8 floats
static void emit_face(float* out, int& idx,
                       Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3,
                       Vec3 normal, float u_scale, float v_scale) {
    // Triangle 1: v0 v1 v2
    // Triangle 2: v0 v2 v3
    Vec3 verts[6] = {v0,v1,v2, v0,v2,v3};
    // Simple planar UV: project onto 2 non-normal axes
    int ax=0,ay=1; // default: x,y plane
    float nx=math_abs(normal.x), ny=math_abs(normal.y), nz=math_abs(normal.z);
    if(nx>=ny && nx>=nz) { ax=2; ay=1; }       // YZ plane
    else if(ny>=nx && ny>=nz) { ax=0; ay=2; }  // XZ plane
    // else XY plane (default)

    for(int i=0;i<6;i++){
        Vec3& v = verts[i];
        float uv[2];
        if(ax==0&&ay==1) { uv[0]=v.x/u_scale; uv[1]=v.y/v_scale; }
        else if(ax==2&&ay==1) { uv[0]=v.z/u_scale; uv[1]=v.y/v_scale; }
        else { uv[0]=v.x/u_scale; uv[1]=v.z/v_scale; }

        out[idx++] = v.x;      out[idx++] = v.y;      out[idx++] = v.z;
        out[idx++] = normal.x; out[idx++] = normal.y; out[idx++] = normal.z;
        out[idx++] = uv[0];    out[idx++] = uv[1];
    }
}

Mesh level_build_mesh() {
    int idx = 0;
    for(int i=0;i<g_level.brush_count;i++){
        const Brush& b = g_level.brushes[i];
        if(b.flags & BRUSH_NOCOLLIDE) continue;

        const Vec3& mn = b.bounds.min;
        const Vec3& mx = b.bounds.max;

        // 8 corners
        Vec3 c[8] = {
            Vec3(mn.x,mn.y,mn.z), Vec3(mx.x,mn.y,mn.z),
            Vec3(mx.x,mn.y,mx.z), Vec3(mn.x,mn.y,mx.z),
            Vec3(mn.x,mx.y,mn.z), Vec3(mx.x,mx.y,mn.z),
            Vec3(mx.x,mx.y,mx.z), Vec3(mn.x,mx.y,mx.z),
        };
        float us=2.f, vs=2.f;

        if(idx/8 + 36 > MAX_WORLD_VERTS) break;

        emit_face(s_world_verts,idx, c[4],c[5],c[6],c[7], Vec3(0,1,0),  us,vs); // top
        emit_face(s_world_verts,idx, c[3],c[2],c[1],c[0], Vec3(0,-1,0), us,vs); // bottom
        emit_face(s_world_verts,idx, c[0],c[1],c[5],c[4], Vec3(0,0,-1), us,vs); // front (N)
        emit_face(s_world_verts,idx, c[2],c[3],c[7],c[6], Vec3(0,0, 1), us,vs); // back  (S)
        emit_face(s_world_verts,idx, c[1],c[2],c[6],c[5], Vec3(1,0,0),  us,vs); // right (E)
        emit_face(s_world_verts,idx, c[3],c[0],c[4],c[7], Vec3(-1,0,0), us,vs); // left  (W)
    }

    s_world_vert_count = idx / 8;
    return mesh_create(s_world_verts, s_world_vert_count, 8, nullptr, 0);
}
