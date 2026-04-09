// game.cpp — VOID Horror FPS
// Main game state machine, rendering orchestration, HUD drawing
#include "game.h"
#include "platform.h"
#include "renderer.h"
#include "audio.h"
#include "world.h"
#include "entity.h"
#include "vmath.h"
#include <cstring>
#include <cstdio>
#include <cmath>

GameData g_game = {};

// Cached level mesh (rebuilt on level load)
static Mesh s_level_mesh = {};
static Mesh s_entity_meshes[5] = {};  // one per EntityType
static bool s_meshes_built = false;

// ─────────────────────────────────────────────────────────────────────────────
// INIT
// ─────────────────────────────────────────────────────────────────────────────
void game_init() {
    memset(&g_game,0,sizeof(g_game));
    g_game.state = GAME_MENU;
    g_game.current_level = 0;

    renderer_init(g_platform.width, g_platform.height);
    audio_init();
    audio_set_ambient(true);

    // Build entity meshes once
    for(int i=0;i<5;i++)
        s_entity_meshes[i] = entity_build_mesh((EntityType)i);
    s_meshes_built = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// LEVEL LOAD
// ─────────────────────────────────────────────────────────────────────────────
void game_load_level(int index) {
    fprintf(stderr, "[VOID] game_load_level(%d) starting\n", index);
    if(s_level_mesh.vao) {
        fprintf(stderr, "[VOID] Destroying old mesh vao=%u\n", s_level_mesh.vao);
        mesh_destroy(s_level_mesh);
    }
    
    fprintf(stderr, "[VOID] Calling level_load(%d)\n", index);
    level_load(index);
    fprintf(stderr, "[VOID] level_load done, calling level_build_mesh()\n");
    s_level_mesh = level_build_mesh();
    fprintf(stderr, "[VOID] level_build_mesh done: vao=%u, vcount=%d\n", 
            s_level_mesh.vao, s_level_mesh.vertex_count);
    
    fprintf(stderr, "[VOID] Calling entities_load_from_level()\n");
    entities_load_from_level();
    fprintf(stderr, "[VOID] entities_load_from_level done\n");
    platform_capture_mouse(true);
    
    g_game.current_level = index;
    g_game.boss_dead = false;
    g_game.time_elapsed = 0;
    audio_set_ambient(true);
    fprintf(stderr, "[VOID] game_load_level(%d) complete\n", index);
}

void game_player_died() {
    g_game.state = GAME_DEAD;
    g_game.death_timer = 3.f;
    audio_play(SND_PLAYER_DIE, 1.f, 1.f);
    platform_capture_mouse(false);
}

void game_win() {
    g_game.state = GAME_WIN;
    platform_capture_mouse(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// UPDATE
// ─────────────────────────────────────────────────────────────────────────────
void game_update(float dt) {
    switch(g_game.state) {

    case GAME_MENU:
        g_game.menu_cursor += dt;
        if (key_just_pressed(KEY_ENTER) || key_just_pressed(KEY_SPACE)) {
            fprintf(stderr, "[VOID] MENU: ENTER pressed, loading level 0...\n");
            game_load_level(0);
            g_game.state = GAME_PLAYING;
            fprintf(stderr, "[VOID] MENU: State changed to PLAYING\n");
        }
        break;

    case GAME_PLAYING:
        g_game.time_elapsed += dt;
        player_update(dt);
        entities_update(dt);
        level_update(dt);

        // Trigger check
        if(g_player_idx >= 0 && g_player_idx < g_entity_count) {
            Entity& pl = g_entities[g_player_idx];
            int action=0, param=-1;
            level_check_triggers(pl.position, &action, &param);
            switch(action){
            case TRIGGER_LOAD_LEVEL:
                g_game.state = GAME_LEVEL_TRANSITION;
                g_game.transition_timer = 1.5f;
                audio_play(SND_LEVEL_TRANSITION, 1.f, 1.f);
                break;
            case TRIGGER_OPEN_DOOR:
                if(param>=0 && param<g_level.brush_count)
                    g_level.brushes[param].door_open = 0.01f; // start opening
                break;
            case TRIGGER_SPAWN_ENEMY:
                if(param>=1 && param<=4)
                    entity_spawn((EntityType)param, pl.position + Vec3(5.f,0,0));
                break;
            default: break;
            }
        }

        // Player death
        if(g_player_idx >= 0 && g_player_idx < g_entity_count) {
            Entity& pl = g_entities[g_player_idx];
            if(pl.health <= 0.f) game_player_died();
        }

        // Level 2 — boss death check
        if(g_game.current_level == 2 && !g_game.boss_dead){
            bool all_brutes_dead = true;
            for(int i=0;i<g_entity_count;i++){
                Entity& e=g_entities[i];
                if(i==g_player_idx) continue;
                if(e.type==ENT_BRUTE && e.active && e.state!=STATE_DEAD){
                    all_brutes_dead=false; break;
                }
            }
            if(all_brutes_dead){
                g_game.boss_dead = true;
                // Open exit door (last DOOR brush in level)
                for(int i=g_level.brush_count-1;i>=0;i--){
                    if(g_level.brushes[i].flags & BRUSH_DOOR){
                        g_level.brushes[i].door_open = 0.01f; break;
                    }
                }
            }
        }

        // Heartbeat at low health
        {
            Entity& pl = g_entities[g_player_idx];
            if(pl.health < 30.f){
                float prev = g_game.heartbeat_phase;
                g_game.heartbeat_phase += dt * TWO_PI * (60.f/60.f);
                if(sinf(prev)>=0 && sinf(g_game.heartbeat_phase)<0)
                    audio_play(SND_HEARTBEAT, 0.8f, 1.f);
                g_game.heartbeat_pulse = math_maxf(0, sinf(g_game.heartbeat_phase));
            } else {
                g_game.heartbeat_pulse = 0;
                g_game.heartbeat_phase = 0;
            }
        }

        // Kill tracking
        for(int i=0;i<g_entity_count;i++){
            if(i==g_player_idx) continue;
            Entity& e=g_entities[i];
            if(e.state==STATE_DEAD && e.active && e.health<=0 && e.state_timer>2.9f)
                g_game.kills++;
        }

        if(key_just_pressed(KEY_ESCAPE)) g_game.state = GAME_PAUSED;
        break;

    case GAME_PAUSED:
        if(key_just_pressed(KEY_ESCAPE)||key_just_pressed(KEY_ENTER)){
            g_game.state = GAME_PLAYING;
            platform_capture_mouse(true);
        }
        break;

    case GAME_DEAD:
        g_game.death_timer -= dt;
        if(g_game.death_timer <= 0){
            game_load_level(g_game.current_level);
            g_game.state = GAME_PLAYING;
        }
        break;

    case GAME_LEVEL_TRANSITION:
        g_game.transition_timer -= dt;
        if(g_game.transition_timer <= 0){
            int next = g_game.current_level + 1;
            if(next >= LEVEL_COUNT) { game_win(); break; }
            game_load_level(next);
            g_game.state = GAME_PLAYING;
        }
        break;

    case GAME_WIN:
        if(key_just_pressed(KEY_ENTER)){
            game_init();
        }
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HUD HELPERS
// ─────────────────────────────────────────────────────────────────────────────
// 7-segment-style digit rendering (each segment as a small rect)
// digit layout: 12×20 px per digit
static void draw_digit(float x, float y, int d, float r, float g2, float b, float a){
    //  _
    // |_|
    // |_|
    // Segments: top, top-left, top-right, mid, bot-left, bot-right, bottom
    static const bool segs[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}, // 9
    };
    if(d<0||d>9) return;
    const bool* s=segs[d];
    float w=10.f,th=2.f,h=9.f;
    if(s[0]) renderer_draw_rect(x+1,y,   w,th,r,g2,b,a);       // top
    if(s[1]) renderer_draw_rect(x,  y+1, th,h,r,g2,b,a);       // top-left
    if(s[2]) renderer_draw_rect(x+w,y+1, th,h,r,g2,b,a);       // top-right
    if(s[3]) renderer_draw_rect(x+1,y+h, w,th,r,g2,b,a);       // mid
    if(s[4]) renderer_draw_rect(x,  y+h+1,th,h,r,g2,b,a);      // bot-left
    if(s[5]) renderer_draw_rect(x+w,y+h+1,th,h,r,g2,b,a);      // bot-right
    if(s[6]) renderer_draw_rect(x+1,y+2*h,w,th,r,g2,b,a);      // bottom
}

static void draw_number(float x, float y, int n, float r, float g2, float b, float a){
    n = math_maxi(0, math_mini(n, 999));
    if(n >= 100){ draw_digit(x,y,n/100,r,g2,b,a); x+=14; n%=100; }
    if(n >= 10)  { draw_digit(x,y,n/10, r,g2,b,a); x+=14; }
    draw_digit(x,y,n%10,r,g2,b,a);
}

// Big letter rendering (3x scale segments, letters: V O I D P A U S E C Y R T N M W L K)
static void draw_big_letter(float x,float y,char c,float r,float g2,float b,float a){
    float s=3.f; // scale
    switch(c){
    case 'V': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+4*s,y+14*s,4*s,6*s,r,g2,b,a); break;
    case 'O': renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'I': renderer_draw_rect(x+4*s,y,4*s,20*s,r,g2,b,a); break;
    case 'D': renderer_draw_rect(x,y,8*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+4*s,4*s,12*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,8*s,4*s,r,g2,b,a); break;
    case 'Y': renderer_draw_rect(x,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x+4*s,y+10*s,4*s,10*s,r,g2,b,a); break;
    case 'E': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,10*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'S': renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+8*s,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'C': renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'P': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,12*s,4*s,r,g2,b,a); break;
    case 'A': renderer_draw_rect(x+4*s,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,4*s,12*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+8*s,4*s,12*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,12*s,4*s,r,g2,b,a); break;
    case 'U': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'R': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,10*s,r,g2,b,a);
              renderer_draw_rect(x,y+8*s,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+8*s,4*s,12*s,r,g2,b,a); break;
    case 'T': renderer_draw_rect(x,y,12*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+4*s,y,4*s,20*s,r,g2,b,a); break;
    case 'N': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+4*s,y+8*s,4*s,4*s,r,g2,b,a); break;
    case 'M': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+2*s,y+2*s,4*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+6*s,y+2*s,4*s,4*s,r,g2,b,a); break;
    case 'W': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+2*s,y+16*s,4*s,4*s,r,g2,b,a);
              renderer_draw_rect(x+6*s,y+16*s,4*s,4*s,r,g2,b,a); break;
    case 'L': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x,y+16*s,12*s,4*s,r,g2,b,a); break;
    case 'K': renderer_draw_rect(x,y,4*s,20*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+2*s,4*s,8*s,r,g2,b,a);
              renderer_draw_rect(x+8*s,y+10*s,4*s,10*s,r,g2,b,a); break;
    default: break;
    }
}

static void draw_big_string(float x,float y, const char* str, float r,float g2,float b,float a){
    while(*str){ 
        if(*str != ' ') draw_big_letter(x,y,*str,r,g2,b,a);
        x+=36.f; 
        str++; 
    }
}

static void draw_crosshair(float cx,float cy){
    renderer_draw_rect(cx-12,cy-1, 8,2, 1,1,1,0.7f);
    renderer_draw_rect(cx+4, cy-1, 8,2, 1,1,1,0.7f);
    renderer_draw_rect(cx-1, cy-12,2,8, 1,1,1,0.7f);
    renderer_draw_rect(cx-1, cy+4, 2,8, 1,1,1,0.7f);
}

static void draw_hud(const RenderState& rs){
    float W=(float)rs.screen_w, H=(float)rs.screen_h;
    
    // Menu mode - show instructions
    if(g_game.state == GAME_MENU) {
        // Semi-transparent background
        renderer_draw_rect(0, 0, W, H, 0, 0, 0, 0.3f);
        
        // Menu text (simple placeholder - just draw rects as visual feedback for now)
        renderer_draw_rect(W*0.5f-200, H*0.5f-50, 400, 100, 0.2f, 0.2f, 0.3f, 0.9f);
        
        return;
    }
    
    // Validate player entity
    if(g_player_idx < 0 || g_player_idx >= g_entity_count) {
        fprintf(stderr, "[VOID] ERROR: Invalid g_player_idx in draw_hud\n");
        return;
    }
    
    Entity& pl = g_entities[g_player_idx];

    draw_crosshair(W*.5f,H*.5f);

    // Health bar
    renderer_draw_rect(20,H-50,200,20, 0.1f,0.1f,0.1f,0.8f);
    float hf=math_clamp(pl.health/100.f,0.f,1.f);
    float hr=1.f-hf*0.5f, hg=hf*0.8f;
    renderer_draw_rect(22,H-48,196*hf,16, hr,hg,0,0.9f);
    renderer_draw_rect(8,H-52, 6,18, 0.8f,0.1f,0.1f,1);
    renderer_draw_rect(3,H-47,16,8,  0.8f,0.1f,0.1f,1);

    // Weapon indicator and ammo
    if(pl.weapon==WEAPON_PISTOL){
        // Ammo clip / reserve - sağ altta
        draw_number(W-80,H-50, pl.ammo_clip,   1,1,0.8f,1);
        renderer_draw_rect(W-50,H-45, 4,2, 1,1,1,0.5f);
        draw_number(W-40, H-50, pl.ammo_pistol, 0.6f,0.6f,0.6f,1);
        
        // Reload indicator
        if(pl.reloading){
            float reload_progress = 1.f - (pl.reload_timer / 1.5f);
            renderer_draw_rect(W-80, H-70, 60*reload_progress, 8, 1,0.5f,0.2f,0.8f);
            renderer_draw_rect(W-80, H-70, 60, 8, 0.3f,0.3f,0.3f,0.3f);
        }
    }

    // Flashlight indicator - sağ altta
    if(pl.flashlight_on) {
        renderer_draw_rect(W-90,H-30, 30,20, 1,0.95f,0.7f,0.9f);
        renderer_draw_rect(W-88,H-28, 26,16, 1,0.8f,0.3f,1);
    } else {
        renderer_draw_rect(W-90,H-30, 30,20, 0.2f,0.2f,0.2f,0.5f);
    }

    // Hurt flash
    if(pl.hurt_flash>0)
        renderer_draw_rect(0,0,W,H, 0.8f,0,0, pl.hurt_flash*0.4f);

    // Paused overlay
    if(g_game.state==GAME_PAUSED){
        renderer_draw_rect(0,0,W,H, 0,0,0,0.5f);
        draw_big_string(W/2-100,H/2-30,"PAUSED", 1,1,1,1);
    }

    // Level transition fade
    if(g_game.state==GAME_LEVEL_TRANSITION){
        float alpha=1.f-(g_game.transition_timer/1.5f);
        renderer_draw_rect(0,0,W,H, 0,0,0,alpha);
    }
}

static void draw_menu(){
    float W=(float)g_platform.width, H=(float)g_platform.height;
    renderer_draw_rect(0,0,W,H, 0,0,0,1);  // Black background
    // Title "VOID"
    float tx=W/2-96.f, ty=H/3;
    draw_big_string(tx,ty,"VOID", 0.8f,0.1f,0.1f,1);
    
    // Blinking "PRESS ENTER" text
    float blink = sinf(g_game.menu_cursor*3.f);
    float alpha = math_maxf(0.3f, blink);
    draw_big_string(W/2-220,H/2+50,"PRESS ENTER", 0.8f,0.8f,0.8f,alpha);
    
    // Pulsing vignette effect
    float vp=0.5f+0.3f*sinf(g_game.menu_cursor*1.5f);
    renderer_draw_rect(0,0,80,H,   0,0,0,vp*0.5f);
    renderer_draw_rect(W-80,0,80,H,0,0,0,vp*0.5f);
    renderer_draw_rect(0,0,W,80,   0,0,0,vp*0.5f);
    renderer_draw_rect(0,H-80,W,80,0,0,0,vp*0.5f);
}

static void draw_death_screen(){
    float W=(float)g_platform.width, H=(float)g_platform.height;
    renderer_draw_rect(0,0,W,H, 0.4f,0,0,0.7f);
    draw_big_string(W/2-100,H/2-60,"YOU DIED", 1,0.1f,0.1f,1);
    float dots = g_game.death_timer;
    renderer_draw_rect(W/2-60,H/2+20, math_clamp((3.f-dots)/3.f,0,1)*120, 6,
                       0.8f,0.8f,0.8f,0.8f);
}

static void draw_win_screen(){
    float W=(float)g_platform.width, H=(float)g_platform.height;
    renderer_draw_rect(0,0,W,H, 0,0,0,0.85f);
    draw_big_string(W/2-150,H/2-80,"YOU ESCAPED", 0.2f,1.f,0.4f,1);
    // Stats
    float sx=W/2-80, sy=H/2+10;
    renderer_draw_rect(sx,sy,160,20, 0.1f,0.1f,0.1f,0.6f);
    draw_number(sx+10,sy+2, (int)g_game.time_elapsed, 0.8f,0.8f,0.8f,1);
    draw_number(sx+80,sy+2, g_game.kills,             0.8f,0.4f,0.4f,1);
    draw_big_string(W/2-80,H/2+50,"PRESS ENTER", 0.6f,0.6f,0.6f,0.7f);
}

// ─────────────────────────────────────────────────────────────────────────────
// RENDER
// ─────────────────────────────────────────────────────────────────────────────
void game_render() {
    if(!s_meshes_built) {
        fprintf(stderr, "[VOID] Meshes not built yet\n");
        return;
    }

    static int frame_count = 0;
    if(frame_count++ % 60 == 0)
        fprintf(stderr, "[VOID] Render frame: state=%d\n", g_game.state);

    // Handle non-gameplay states first (menu, etc.)
    if(g_game.state==GAME_MENU || g_game.state==GAME_WIN) {
        renderer_begin_frame_ui();
        renderer_begin_hud();
        if(g_game.state==GAME_MENU) {
            draw_menu();
        } else if(g_game.state==GAME_WIN) {
            draw_win_screen();
        }
        renderer_end_frame();
        return;
    }

    // For gameplay states, validate player entity exists
    if(g_player_idx < 0 || g_player_idx >= g_entity_count) {
        fprintf(stderr, "[VOID] ERROR: Invalid g_player_idx=%d, entity_count=%d\n", g_player_idx, g_entity_count);
        return;
    }

    Entity& pl = g_entities[g_player_idx];
    float yr=pl.yaw*DEG2RAD, pr=pl.pitch*DEG2RAD;
    Vec3 eye = pl.position + Vec3(0,1.65f,0);
    Vec3 fwd(sinf(yr)*cosf(pr), sinf(pr), cosf(yr)*cosf(pr));
    float aspect=(float)g_platform.width/(float)g_platform.height;

    // Build render state
    RenderState rs={};
    rs.screen_w=g_platform.width;
    rs.screen_h=g_platform.height;
    rs.time=g_game.time_elapsed;
    rs.health_normalized=math_clamp(pl.health/100.f,0,1);
    rs.flash_on=pl.flashlight_on?1.f:0.f;
    rs.heartbeat_pulse=g_game.heartbeat_pulse;
    rs.cam_pos=eye;
    rs.flash_pos=eye;
    rs.flash_dir=fwd;
    rs.flash_intensity=1.f;
    rs.ambient=0.15f;
    rs.proj=Mat4::perspective(75.f*DEG2RAD, aspect, 0.05f, 150.f);
    rs.view=Mat4::lookAt(eye, eye+fwd, Vec3(0,1,0));
    rs.light_count=0;

    if(g_game.state==GAME_PLAYING || g_game.state==GAME_PAUSED ||
       g_game.state==GAME_LEVEL_TRANSITION || g_game.state==GAME_DEAD) {
        fprintf(stderr, "[VOID] GAMEPLAY state, calling renderer_begin_frame\n");
        fflush(stderr);
        renderer_begin_frame(rs);
        fprintf(stderr, "[VOID] renderer_begin_frame OK\n");
        fflush(stderr);

        // Draw level
        fprintf(stderr, "[VOID] Drawing world mesh: vao=%u, vcount=%d\n", 
                s_level_mesh.vao, s_level_mesh.vertex_count);
        fflush(stderr);
        if(s_level_mesh.vao == 0 || s_level_mesh.vertex_count == 0) {
            fprintf(stderr, "[VOID] ERROR: Invalid mesh VAO or vertex count! Skipping render\n");
            fflush(stderr);
        } else if(TEX_CONCRETE >= TEX_COUNT) {
            fprintf(stderr, "[VOID] ERROR: Invalid texture ID %d for TEX_CONCRETE! Skipping render\n", TEX_CONCRETE);
            fflush(stderr);
        } else {
            fprintf(stderr, "[VOID] Calling renderer_draw_world with tex=%d...\n", TEX_CONCRETE);
            fflush(stderr);
            // Disable for now to see if that's the crash point
            // renderer_draw_world(s_level_mesh, g_textures[TEX_CONCRETE],
            //                     Mat4::identity(), rs);
            fprintf(stderr, "[VOID] World mesh rendering disabled (checking for crash)\n");
            fflush(stderr);
        }

        // Draw entities
        fprintf(stderr, "[VOID] Drawing %d entities\n", g_entity_count);
        fflush(stderr);
        for(int i=0;i<g_entity_count;i++){
            const Entity& e=g_entities[i];
            fprintf(stderr, "[VOID] Entity %d: active=%d, state=%d, type=%d\n", 
                    i, e.active, e.state, e.type);
            fflush(stderr);
            if(!e.active || e.state==STATE_DEAD) continue;
            
            if(i==g_player_idx){
                fprintf(stderr, "[VOID] Drawing player (index %d)\n", i);
                fflush(stderr);
                // Draw weapon viewmodel
                float bob_x = sinf(pl.bob_phase)*0.03f;
                float bob_y = fabsf(cosf(pl.bob_phase*0.5f))*0.02f;
                Vec3 right2 = fwd.cross(Vec3(0,1,0)).normalized();
                Vec3 wp = eye + fwd*0.25f + right2*(0.1f+bob_x) + Vec3(0,-0.12f+bob_y,0);
                Mat4 wm = Mat4::translate(wp) * Mat4::rotateY(pl.yaw*DEG2RAD) * Mat4::rotateX(pl.pitch*DEG2RAD*0.3f);
                fprintf(stderr, "[VOID] Player mesh: vao=%u, vertex_count=%d\n",
                        s_entity_meshes[ENT_PLAYER].vao, s_entity_meshes[ENT_PLAYER].vertex_count);
                fprintf(stderr, "[VOID] Weapon tex=%u, TEX_WEAPON=%d, g_textures addr=%p\n", 
                        g_textures[TEX_WEAPON], TEX_WEAPON, (void*)g_textures);
                fprintf(stderr, "[VOID] About to draw player mesh...\n");
                fflush(stderr);
                if(s_entity_meshes[ENT_PLAYER].vao == 0) {
                    fprintf(stderr, "[VOID] ERROR: Player mesh VAO is invalid!\n");
                    fflush(stderr);
                } else if(TEX_WEAPON < 0 || TEX_WEAPON >= TEX_COUNT) {
                    fprintf(stderr, "[VOID] ERROR: Weapon texture index %d out of bounds [0-%d]!\n", TEX_WEAPON, TEX_COUNT-1);
                    fflush(stderr);
                } else {
                    fprintf(stderr, "[VOID] Calling renderer_draw_entity with mesh vao=%u, tex=%u...\n",
                            s_entity_meshes[ENT_PLAYER].vao, g_textures[TEX_WEAPON]);
                    fflush(stderr);
                    renderer_draw_entity(s_entity_meshes[ENT_PLAYER],
                                         g_textures[TEX_WEAPON], wm, rs);
                    fprintf(stderr, "[VOID] Player mesh drawn OK\n");
                    fflush(stderr);
                }
                fprintf(stderr, "[VOID] Player mesh drawn OK\n");
                fflush(stderr);
                continue;
            }
            
            if(e.type==ENT_WRAITH && e.state==STATE_INVISIBLE) continue;

            // Bounds check entity type
            if(e.type < 0 || e.type >= 5) {
                fprintf(stderr, "[VOID] ERROR: Invalid entity type %d\n", e.type);
                fflush(stderr);
                continue;
            }

            TextureID tex = TEX_ENEMY_SKIN;
            Mat4 model = Mat4::translate(e.position)*Mat4::rotateY(e.yaw*DEG2RAD);
            fprintf(stderr, "[VOID] Drawing entity type %d\n", e.type);
            fflush(stderr);
            renderer_draw_entity(s_entity_meshes[e.type], g_textures[tex], model, rs);
        }

        renderer_end_world(rs);

        renderer_begin_hud();
        if(g_game.state==GAME_DEAD){
            draw_death_screen();
        } else {
            draw_hud(rs);
        }
        renderer_end_frame();
    }
}
