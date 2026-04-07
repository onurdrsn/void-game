// renderer.h — VOID Horror FPS
#pragma once
#include "vmath.h"

// ── Texture IDs ───────────────────────────────────────────────────────────────
enum TextureID {
    TEX_CONCRETE = 0,
    TEX_METAL,
    TEX_FLOOR,
    TEX_BLOOD,
    TEX_PANEL,
    TEX_ENEMY_SKIN,
    TEX_WEAPON,
    TEX_COUNT
};

// ── Mesh handle ───────────────────────────────────────────────────────────────
struct Mesh {
    unsigned int vao, vbo, ebo;
    int vertex_count;
    int index_count;
    bool indexed;
};

// ── Point light ───────────────────────────────────────────────────────────────
struct PointLight {
    Vec3  position;
    Vec3  color;
    float radius;
};

static const int MAX_LIGHTS = 8;

// ── RenderState (per-frame) ───────────────────────────────────────────────────
struct RenderState {
    int   screen_w, screen_h;
    float time;
    float health_normalized;
    float flash_on;
    float heartbeat_pulse;
    Vec3  cam_pos;
    Vec3  flash_pos;
    Vec3  flash_dir;
    float flash_intensity;
    PointLight lights[MAX_LIGHTS];
    int   light_count;
    float ambient;
    Mat4  proj;
    Mat4  view;
};

bool renderer_init(int w, int h);
void renderer_shutdown();
void renderer_resize(int w, int h);

// Mesh creation
Mesh mesh_create(const float* vertices, int vcount, int floats_per_vertex,
                 const unsigned int* indices, int icount);
void mesh_destroy(Mesh& m);
void mesh_draw(const Mesh& m);

// Texture creation
unsigned int texture_create(int w, int h, const unsigned char* rgba_data);
unsigned int texture_generate(TextureID id);
void texture_bind(unsigned int id, int slot);

// Frame lifecycle
void renderer_begin_frame(const RenderState& rs);
void renderer_draw_world(const Mesh& m, unsigned int tex_id, const Mat4& model, const RenderState& rs);
void renderer_draw_entity(const Mesh& m, unsigned int tex_id, const Mat4& model, const RenderState& rs);
void renderer_end_world(const RenderState& rs);
void renderer_begin_hud();
void renderer_draw_rect(float x, float y, float w, float h, float r, float g, float b, float a);
void renderer_draw_rect_uv(float x, float y, float w, float h, unsigned int tex, float r, float g, float b, float a);
void renderer_end_frame();

// Framebuffer for post-processing
extern unsigned int g_fbo, g_fbo_color_tex, g_fbo_depth_rbo;

// Generated texture handles
extern unsigned int g_textures[TEX_COUNT];
