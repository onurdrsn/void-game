// entity.cpp — VOID Horror FPS
// Player controller, AI state machine, A* pathfinding, procedural meshes
#include "entity.h"
#include "world.h"
#include "audio.h"
#include "platform.h"
#include "vmath.h"
#include <cstring>
#include <cstdio>
#include <cmath>

Entity g_entities[MAX_ENTITIES] = {};
int    g_entity_count = 0;
int    g_player_idx   = 0;

bool  g_nav_grid[GRID_SIZE][GRID_SIZE] = {};
Vec3  g_grid_origin    = {};
float g_grid_cell_size = 1.5f;

// ── Enemy stats table ─────────────────────────────────────────────────────────
struct EnemyStats { float hp, speed, atk_range, damage, det_range, atk_interval; };
static const EnemyStats STATS[] = {
    {100,5,0,0,0,0},          // player (unused)
    { 30,7, 1.5f,15,15, 0.8f}, // crawler
    { 50,4, 12,  20,20, 1.2f}, // shadow
    {200,2.5f,2.5f,35,12,1.5f},// brute
    { 40,6,  2,  25, 8, 1.0f}, // wraith
};

void entities_init() {
    for (int i = 0; i < MAX_ENTITIES; i++) g_entities[i] = {};
    g_entity_count = 0;
    g_player_idx   = -1;  // Initialize to invalid value to detect missing player
}

void entities_load_from_level() {
    entities_init();
    for(int i=0;i<g_level.spawn_count;i++){
        const SpawnPoint& sp = g_level.spawns[i];
        EntityType type = (EntityType)sp.entity_type;
        Entity* e = entity_spawn(type, sp.position);
        if(!e) continue;
        e->yaw = (float)sp.facing_deg;
        if(type == ENT_PLAYER) g_player_idx = (int)(e - g_entities);
    }
    
    // Validate that player was spawned
    if(g_player_idx < 0 || g_player_idx >= g_entity_count) {
        fprintf(stderr, "[VOID] WARNING: Player entity not spawned! g_player_idx=%d, entity_count=%d\n", 
                g_player_idx, g_entity_count);
    } else {
        fprintf(stderr, "[VOID] Player spawned at index=%d, total entities=%d\n", g_player_idx, g_entity_count);
    }
    
    nav_grid_build();
}

Entity* entity_spawn(EntityType type, Vec3 pos) {
    if(g_entity_count >= MAX_ENTITIES) return nullptr;
    Entity& e = g_entities[g_entity_count++];
    e = {};
    e.type      = type;
    e.state     = (type==ENT_WRAITH) ? STATE_INVISIBLE : STATE_IDLE;
    e.position  = pos;
    e.active    = true;
    e.alpha     = (type==ENT_WRAITH) ? 0.f : 1.f;

    const EnemyStats& s = STATS[type];
    e.health = e.max_health = s.hp;

    if(type==ENT_PLAYER){
        e.bounds      = AABB(Vec3(-0.3f,0,-0.3f),Vec3(0.3f,1.8f,0.3f));
        e.ammo_clip   = 12;
        e.ammo_pistol = 48;
        e.weapon      = WEAPON_FLASHLIGHT;
        e.flashlight_on = true;
    } else {
        float hs=0.4f;
        switch(type){
            case ENT_CRAWLER: e.bounds=AABB(Vec3(-0.6f,0,-0.4f),Vec3(0.6f,0.4f,0.4f)); break;
            case ENT_SHADOW:  e.bounds=AABB(Vec3(-0.3f,0,-0.3f),Vec3(0.3f,1.9f,0.3f)); break;
            case ENT_BRUTE:   e.bounds=AABB(Vec3(-0.7f,0,-0.5f),Vec3(0.7f,2.2f,0.5f)); hs=0.8f; break;
            case ENT_WRAITH:  e.bounds=AABB(Vec3(-0.4f,0,-0.4f),Vec3(0.4f,2.f,0.4f));  break;
            default: break;
        }
        (void)hs;
    }
    return &e;
}

void entity_kill(int idx) {
    g_entities[idx].state  = STATE_DEAD;
    g_entities[idx].state_timer = 3.f;
}

void entity_damage(int idx, float dmg, Vec3 /*hit_dir*/) {
    Entity& e = g_entities[idx];
    if(e.state==STATE_DEAD) return;
    e.health -= dmg;
    if(idx==g_player_idx){
        e.hurt_flash = 0.4f;
        audio_play(SND_PLAYER_HURT, 0.7f, 1.f);
    }
    if(e.health <= 0.f) entity_kill(idx);
}

// ── Line of sight (ray vs all solid brushes) ─────────────────────────────────
static bool has_los(Vec3 from, Vec3 to) {
    Vec3 dir = to - from;
    float maxd = dir.length();
    if(maxd < EPSILON) return true;
    Ray r(from, dir*(1.f/maxd));
    for(int i=0;i<g_level.brush_count;i++){
        const Brush& b = g_level.brushes[i];
        if(!(b.flags & BRUSH_SOLID)) continue;
        if(b.flags & BRUSH_NOCOLLIDE) continue;
        float t0,t1;
        if(r.intersectsAABB(b.bounds,t0,t1))
            if(t0>=0.f && t0<maxd) return false;
    }
    return true;
}

// ── Navigation grid ───────────────────────────────────────────────────────────
void nav_grid_build() {
    // Build a 32×32 walkable grid centered at level origin
    g_grid_origin    = Vec3(-24.f, 0.f, -24.f);
    g_grid_cell_size = 1.5f;
    for(int gz=0;gz<GRID_SIZE;gz++) for(int gx=0;gx<GRID_SIZE;gx++){
        float wx = g_grid_origin.x + gx*g_grid_cell_size + g_grid_cell_size*0.5f;
        float wz = g_grid_origin.z + gz*g_grid_cell_size + g_grid_cell_size*0.5f;
        AABB test(Vec3(wx-0.3f,-0.5f,wz-0.3f),Vec3(wx+0.3f,1.8f,wz+0.3f));
        g_nav_grid[gz][gx] = true;
        for(int i=0;i<g_level.brush_count;i++){
            const Brush& b = g_level.brushes[i];
            if(!(b.flags & BRUSH_SOLID)) continue;
            if(b.flags & BRUSH_NOCOLLIDE) continue;
            if(test.intersects(b.bounds)){ g_nav_grid[gz][gx]=false; break; }
        }
    }
}

// Simple greedy pathfinding towards player (not full A*, sufficient for gameplay)
bool nav_find_path(Vec3 from, Vec3 to, Vec3* out_wp, int max_wp, int* out_count) {
    *out_count = 0;
    if(max_wp < 1) return false;

    // Direct path if LOS
    if(has_los(from,to)){
        out_wp[0] = to;
        *out_count = 1;
        return true;
    }

    // Steer toward target in grid space
    int gx0 = (int)((from.x - g_grid_origin.x) / g_grid_cell_size);
    int gz0 = (int)((from.z - g_grid_origin.z) / g_grid_cell_size);
    int gx1 = (int)((to.x   - g_grid_origin.x) / g_grid_cell_size);
    int gz1 = (int)((to.z   - g_grid_origin.z) / g_grid_cell_size);

    gx0=math_maxi(0,math_mini(GRID_SIZE-1,gx0));
    gz0=math_maxi(0,math_mini(GRID_SIZE-1,gz0));
    gx1=math_maxi(0,math_mini(GRID_SIZE-1,gx1));
    gz1=math_maxi(0,math_mini(GRID_SIZE-1,gz1));

    // Walk toward target cell, pick free neighbor
    int cx=gx0, cz=gz0;
    for(int step=0;step<max_wp&&(cx!=gx1||cz!=gz1);step++){
        int dx=(gx1>cx)?1:(gx1<cx?-1:0);
        int dz=(gz1>cz)?1:(gz1<cz?-1:0);
        // Try direct, then X, then Z
        int ncx=cx+dx, ncz=cz+dz;
        if(ncx>=0&&ncx<GRID_SIZE&&ncz>=0&&ncz<GRID_SIZE&&g_nav_grid[ncz][ncx]){
            cx=ncx; cz=ncz;
        } else if(dx!=0&&cx+dx>=0&&cx+dx<GRID_SIZE&&g_nav_grid[cz][cx+dx]){
            cx+=dx;
        } else if(dz!=0&&cz+dz>=0&&cz+dz<GRID_SIZE&&g_nav_grid[cz+dz][cx]){
            cz+=dz;
        } else break;

        float wx = g_grid_origin.x + cx*g_grid_cell_size + g_grid_cell_size*0.5f;
        float wz = g_grid_origin.z + cz*g_grid_cell_size + g_grid_cell_size*0.5f;
        out_wp[*out_count] = Vec3(wx, from.y, wz);
        (*out_count)++;
    }
    if(*out_count == 0){ out_wp[0]=to; *out_count=1; }
    return true;
}

// ── Player update ─────────────────────────────────────────────────────────────
void player_update(float dt) {
    // Validate player entity exists
    if(g_player_idx < 0 || g_player_idx >= g_entity_count) {
        fprintf(stderr, "[VOID] ERROR: player_update called with invalid g_player_idx=%d\n", g_player_idx);
        return;
    }
    
    Entity& p = g_entities[g_player_idx];
    if(!p.active || p.state==STATE_DEAD) return;

    const float sensitivity = 0.12f;
    p.yaw   += g_platform.mouse_dx * sensitivity;
    p.pitch -= g_platform.mouse_dy * sensitivity;
    p.pitch  = math_clamp(p.pitch, -89.f, 89.f);

    float yr  = p.yaw   * DEG2RAD;
    float pr  = p.pitch * DEG2RAD;
    Vec3 forward(sinf(yr)*cosf(pr), sinf(pr), cosf(yr)*cosf(pr));
    Vec3 fwd_flat(sinf(yr),0,cosf(yr));
    Vec3 right = fwd_flat.cross(Vec3(0,1,0)).normalized();
    p.facing = forward;

    float speed = key_down(KEY_SHIFT) ? 8.f : 5.f;
    if(p.in_water) speed *= 0.4f;

    Vec3 wish(0,0,0);
    if(key_down(KEY_W)) wish += fwd_flat;
    if(key_down(KEY_S)) wish -= fwd_flat;
    if(key_down(KEY_A)) wish -= right;
    if(key_down(KEY_D)) wish += right;
    if(wish.lengthSq()>0) wish = wish.normalized();

    p.velocity.x = wish.x * speed;
    p.velocity.z = wish.z * speed;

    // Friction
    if(p.on_ground){ p.velocity.x*=0.82f; p.velocity.z*=0.82f; }

    // Jump / swim
    if(key_just_pressed(KEY_SPACE)){
        if(p.on_ground) p.velocity.y = 6.f;
        else if(p.in_water) p.velocity.y = 2.5f;
    }

    // Gravity
    if(!p.on_ground&&!p.in_water) p.velocity.y -= 18.f*dt;
    if(p.in_water && p.velocity.y < -1.f) p.velocity.y = -1.f;

    // Move and collide
    p.position += p.velocity * dt;
    p.on_ground = false;
    Vec3 old_vel = p.velocity;
    bool collided = level_collide(p.bounds, p.position, p.velocity);
    // If Y velocity was zeroed and we were falling, we're on ground
    if(collided && old_vel.y < 0 && p.velocity.y == 0) p.on_ground = true;

    // Water check
    p.in_water = false;
    AABB wb(p.position+p.bounds.min, p.position+p.bounds.max);
    for(int i=0;i<g_level.brush_count;i++){
        const Brush& b=g_level.brushes[i];
        if((b.flags&BRUSH_WATER) && wb.intersects(b.bounds)) { p.in_water=true; break; }
    }

    // Flashlight
    if(key_just_pressed(KEY_F)) p.flashlight_on = !p.flashlight_on;

    // Weapons
    if(key_just_pressed(KEY_1)) p.weapon = WEAPON_FLASHLIGHT;
    if(key_just_pressed(KEY_2)) p.weapon = WEAPON_PISTOL;

    // Shoot
    if(g_platform.mouse_buttons[0] && p.weapon==WEAPON_PISTOL && !p.reloading)
        player_shoot();

    // Reload
    if(!p.reloading && p.weapon==WEAPON_PISTOL){
        if(key_just_pressed(KEY_R) && p.ammo_clip<12 && p.ammo_pistol>0)
            player_reload();
        if(p.ammo_clip==0 && p.ammo_pistol>0) player_reload();
    }

    if(p.reloading){
        p.reload_timer -= dt;
        if(p.reload_timer<=0){
            int need = 12 - p.ammo_clip;
            int take = math_mini(need, p.ammo_pistol);
            p.ammo_clip   += take;
            p.ammo_pistol -= take;
            p.reloading    = false;
        }
    }

    // Weapon bob
    float mv = wish.lengthSq() > 0 ? 1.f : 0.f;
    p.bob_phase += mv * dt * 8.f;

    // Hurt flash decay
    if(p.hurt_flash>0) p.hurt_flash -= dt*2.f;
    if(p.hurt_flash<0) p.hurt_flash=0;

    // Footstep sounds
    float fspeed = wish.lengthSq()>0 ? speed : 0.f;
    p.footstep_timer += dt * (fspeed / 5.f);
    if(p.footstep_timer > 0.5f && p.on_ground && fspeed>0){
        float pitch = 0.9f + noise_hash(p.footstep_timer)*0.2f;
        audio_play(SND_FOOTSTEP, 0.4f, pitch);
        p.footstep_timer = 0;
    }
}

void player_shoot() {
    Entity& p = g_entities[g_player_idx];
    if(p.ammo_clip<=0||p.reloading) return;
    p.ammo_clip--;
    audio_play(SND_GUNSHOT, 0.8f, 1.f);

    // Raycast against entities
    Vec3 eye = p.position + Vec3(0,1.65f,0);
    float yr=p.yaw*DEG2RAD, pr=p.pitch*DEG2RAD;
    Vec3 dir(sinf(yr)*cosf(pr), sinf(pr), cosf(yr)*cosf(pr));
    Ray ray(eye, dir);

    float best = 100.f;
    int   hit_idx = -1;
    for(int i=0;i<g_entity_count;i++){
        if(i==g_player_idx) continue;
        Entity& e=g_entities[i];
        if(!e.active||e.state==STATE_DEAD) continue;
        AABB wb(e.position+e.bounds.min, e.position+e.bounds.max);
        float t0,t1;
        if(ray.intersectsAABB(wb,t0,t1)&&t0>0&&t0<best){
            best=t0; hit_idx=i;
        }
    }
    if(hit_idx>=0) entity_damage(hit_idx, 25.f, dir);
}

void player_reload() {
    Entity& p = g_entities[g_player_idx];
    if(p.reloading||p.ammo_pistol<=0||p.ammo_clip>=12) return;
    p.reloading    = true;
    p.reload_timer = 1.5f;
    audio_play(SND_RELOAD, 0.6f, 1.f);
}

// ── Enemy AI ──────────────────────────────────────────────────────────────────
void enemy_update(int idx, float dt) {
    Entity& e  = g_entities[idx];
    Entity& pl = g_entities[g_player_idx];
    if(!e.active) return;

    if(e.state==STATE_DEAD){
        e.state_timer -= dt;
        e.position.y  -= dt * 0.3f;
        if(e.state_timer<=0) e.active=false;
        return;
    }

    const EnemyStats& s = STATS[e.type];
    Vec3 to_player = pl.position - e.position;
    float dist_player = to_player.length();
    bool  los = has_los(e.position + Vec3(0,1,0), pl.position + Vec3(0,1,0));

    // Wraith: invisibility logic
    if(e.type==ENT_WRAITH && e.state==STATE_INVISIBLE){
        bool reveal = (dist_player < 4.f) ||
                      (pl.flashlight_on && dist_player < 15.f && los &&
                       to_player.normalized().dot(pl.facing) < -0.7f);
        if(reveal){ e.state=STATE_ALERT; e.alert_timer=0; }
        e.alpha = 0.f;
        // still move slowly
        Vec3 dir2 =(pl.position-e.position); dir2.y=0;
        float d2=dir2.length();
        if(d2>0.5f){ dir2=dir2*(1.f/d2); e.position+=dir2*s.speed*0.3f*dt; }
        return;
    }
    // Wraith alpha fade-in
    if(e.type==ENT_WRAITH && e.alpha<1.f) e.alpha=math_clamp(e.alpha+dt*2.f,0,1);

    // Shadow: teleport logic
    if(e.type==ENT_SHADOW && e.state==STATE_CHASE){
        e.teleport_timer -= dt;
        if(e.teleport_timer<=0 && dist_player>8.f){
            e.teleport_timer = 3.f;
            e.state = STATE_TELEPORTING;
            e.state_timer = 0.5f;
            Vec3 dir2 = (pl.position-e.position).normalized();
            e.position = pl.position - dir2*4.f;
            e.position.y = pl.position.y;
            audio_play(SND_ENEMY_GROWL, 0.5f, 1.5f);
        }
    }
    if(e.state==STATE_TELEPORTING){
        e.state_timer-=dt;
        if(e.state_timer<=0) e.state=STATE_CHASE;
        return;
    }

    // FSM
    switch(e.state){
    case STATE_IDLE:
        e.state_timer -= dt;
        if(e.state_timer<=0){
            e.yaw += 45.f;
            e.state_timer = 2.f + noise_hash(e.position.x)*3.f;
        }
        if(dist_player < s.det_range && los){ e.state=STATE_ALERT; e.alert_timer=0; }
        break;

    case STATE_ALERT:
        e.alert_timer += dt;
        { Vec3 d=to_player; d.y=0; if(d.lengthSq()>EPSILON) e.yaw=math_atan2(d.x,d.z)*RAD2DEG; }
        if(e.alert_timer > 0.5f){ e.state=STATE_CHASE; audio_play(SND_ENEMY_GROWL,0.6f,1.f); }
        if(dist_player>s.det_range+3.f){ e.state=STATE_IDLE; e.state_timer=2.f; }
        break;

    case STATE_CHASE: {
        // Update path every 0.5s
        e.state_timer -= dt;
        if(e.state_timer<=0){
            e.state_timer=0.5f;
            e.last_known_player_pos=pl.position;
            nav_find_path(e.position, pl.position, e.path, 8, &e.path_count);
            e.path_target=0;
        }
        if(e.path_count>0 && e.path_target<e.path_count){
            Vec3 target = e.path[e.path_target]; target.y=e.position.y;
            Vec3 dir2 = target - e.position; dir2.y=0;
            float d2=dir2.length();
            if(d2 < g_grid_cell_size*0.6f) e.path_target++;
            else {
                dir2=dir2*(1.f/d2);
                e.position += dir2*s.speed*dt;
                e.yaw=math_atan2(dir2.x,dir2.z)*RAD2DEG;
            }
        }
        // Gravity
        e.velocity.y -= 18.f*dt;
        e.position.y += e.velocity.y*dt;
        Vec3 vel2(0,e.velocity.y,0);
        level_collide(e.bounds,e.position,vel2);
        e.velocity.y=vel2.y;

        if(dist_player < s.atk_range) e.state=STATE_ATTACK;
        if(!los){ e.alert_timer+=dt; if(e.alert_timer>2.f){e.state=STATE_IDLE;e.state_timer=2.f;} }
        else e.alert_timer=0;
        break;
    }
    case STATE_ATTACK:
        e.attack_timer += dt;
        if(e.attack_timer >= s.atk_interval){
            if(dist_player < s.atk_range){
                entity_damage(g_player_idx, s.damage,
                              (pl.position-e.position).normalized());
                audio_play_3d(SND_ENEMY_ATTACK,0.7f,1.f,
                              e.position.x,e.position.y,e.position.z,
                              pl.position.x,pl.position.y,pl.position.z);
            }
            e.attack_timer=0;
        }
        if(dist_player > s.atk_range*1.5f) e.state=STATE_CHASE;
        break;

    default: break;
    }
}

void entities_update(float dt) {
    player_update(dt);
    for(int i=0;i<g_entity_count;i++){
        if(i==g_player_idx) continue;
        enemy_update(i, dt);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCEDURAL ENEMY MESHES
// pos(3)+normal(3)+uv(2) = 8 floats per vertex
// Helper: emit a box primitive into a float array
// ─────────────────────────────────────────────────────────────────────────────
static int s_mesh_idx=0;
static float s_mesh_buf[4096];

static void box(float x0,float y0,float z0, float x1,float y1,float z1){
    float cx=(x0+x1)*.5f,cy=(y0+y1)*.5f,cz=(z0+z1)*.5f;
    struct Face{ Vec3 n; int vi[4]; };
    Vec3 v[8]={
        {x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},
        {x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1}
    };
    (void)cx;(void)cy;(void)cz;
    static const int faces[6][4]={
        {4,5,6,7},{3,2,1,0},{0,1,5,4},{2,3,7,6},{1,2,6,5},{3,0,4,7}
    };
    static const Vec3 norms[6]={
        {0,1,0},{0,-1,0},{0,0,-1},{0,0,1},{1,0,0},{-1,0,0}
    };
    static const float uvs[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(int f=0;f<6;f++){
        int tri[2][3]={{0,1,2},{0,2,3}};
        for(int t=0;t<2;t++) for(int k=0;k<3;k++){
            if(s_mesh_idx+8>4096) return;
            Vec3& vv=v[faces[f][tri[t][k]]];
            s_mesh_buf[s_mesh_idx++]=vv.x; s_mesh_buf[s_mesh_idx++]=vv.y; s_mesh_buf[s_mesh_idx++]=vv.z;
            s_mesh_buf[s_mesh_idx++]=norms[f].x;s_mesh_buf[s_mesh_idx++]=norms[f].y;s_mesh_buf[s_mesh_idx++]=norms[f].z;
            s_mesh_buf[s_mesh_idx++]=uvs[tri[t][k]][0];s_mesh_buf[s_mesh_idx++]=uvs[tri[t][k]][1];
        }
    }
}

Mesh entity_build_mesh(EntityType type){
    s_mesh_idx=0;
    switch(type){
    case ENT_CRAWLER:
        box(-0.6f,0,-0.4f, 0.6f,0.4f, 0.4f); // body
        box(-0.05f,0.3f,-0.2f, 0.05f,0.55f,0.2f); // head
        box(-0.7f,0.05f,-0.05f,-0.6f,0.15f,0.05f); // leg L1
        box( 0.6f,0.05f,-0.05f, 0.7f,0.15f,0.05f); // leg R1
        box(-0.65f,0.05f,-0.25f,-0.55f,0.15f,-0.15f); // leg L2
        box( 0.55f,0.05f,-0.25f, 0.65f,0.15f,-0.15f); // leg R2
        box(-0.65f,0.05f,0.15f,-0.55f,0.15f,0.25f); // leg L3
        box( 0.55f,0.05f,0.15f, 0.65f,0.15f,0.25f); // leg R3
        break;
    case ENT_SHADOW:
        box(-0.2f,0.8f,-0.15f,0.2f,1.7f,0.15f);  // torso
        box(-0.05f,0,-0.05f,0.05f,0.85f,0.05f);   // neck+head stub
        box(-0.05f,1.65f,-0.1f,0.05f,1.9f,0.1f);  // head
        box(-0.45f,0.85f,-0.08f,-0.2f,1.5f,0.08f);// arm L
        box(0.2f,0.85f,-0.08f,0.45f,1.5f,0.08f);  // arm R
        box(-0.12f,0,-0.08f,-0.04f,0.85f,0.08f);  // leg L
        box(0.04f,0,-0.08f,0.12f,0.85f,0.08f);    // leg R
        break;
    case ENT_BRUTE:
        box(-0.55f,0.7f,-0.35f,0.55f,1.8f,0.35f); // torso (wide)
        box(-0.15f,1.75f,-0.2f,0.15f,2.15f,0.2f); // head
        box(-0.9f,0.75f,-0.15f,-0.55f,1.6f,0.15f);// arm L
        box(0.55f,0.75f,-0.15f,0.9f,1.6f,0.15f);  // arm R
        box(-0.25f,0,-0.15f,-0.08f,0.75f,0.15f);  // leg L
        box(0.08f,0,-0.15f,0.25f,0.75f,0.15f);    // leg R
        break;
    case ENT_WRAITH:
        box(-0.3f,0.8f,-0.2f,0.3f,1.8f,0.2f);    // body
        box(-0.1f,1.75f,-0.12f,0.1f,2.f,0.12f);  // head
        box(-0.55f,0.9f,-0.07f,-0.3f,1.6f,0.07f);// arm L
        box(0.3f,0.9f,-0.07f,0.55f,1.6f,0.07f);  // arm R
        // wispy bottom (spread)
        box(-0.4f,0,-0.05f,-0.25f,0.85f,0.05f);
        box(-0.1f,0,-0.05f,0.1f,0.85f,0.05f);
        box(0.25f,0,-0.05f,0.4f,0.85f,0.05f);
        break;
    case ENT_PLAYER: // weapon (pistol)
        box(-0.04f,-0.06f,0.f,0.04f,0.04f,0.25f); // grip
        box(-0.03f,0.03f,-0.04f,0.03f,0.07f,0.3f); // slide/barrel
        box(-0.025f,-0.07f,0.05f,0.025f,-0.04f,0.15f); // trigger guard
        break;
    default: break;
    }
    return mesh_create(s_mesh_buf, s_mesh_idx/8, 8, nullptr, 0);
}
