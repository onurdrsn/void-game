// renderer.cpp — VOID Horror FPS
// OpenGL 3.3 Core Profile renderer
// All shaders embedded as string literals
#include "renderer.h"
#include "platform.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

// ── FBO globals ───────────────────────────────────────────────────────────────
unsigned int g_fbo           = 0;
unsigned int g_fbo_color_tex = 0;
unsigned int g_fbo_depth_rbo = 0;
unsigned int g_textures[TEX_COUNT] = {};

// ── Internal state ────────────────────────────────────────────────────────────
static unsigned int s_prog_world  = 0;
static unsigned int s_prog_post   = 0;
static unsigned int s_prog_hud    = 0;
static unsigned int s_quad_vao    = 0;
static unsigned int s_quad_vbo    = 0;
static unsigned int s_hud_vao     = 0;
static unsigned int s_hud_vbo     = 0;
static int s_fbo_w = 0, s_fbo_h = 0;

// HUD batch buffer  (pos2 + uv2 + rgba4 = 8 floats per vertex, 6 verts per quad)
static const int HUD_MAX_VERTS = 4096 * 6;
static float s_hud_buf[HUD_MAX_VERTS * 8];
static int   s_hud_verts = 0;

// ─────────────────────────────────────────────────────────────────────────────
// SHADER SOURCE STRINGS
// ─────────────────────────────────────────────────────────────────────────────

static const char* VERT_WORLD_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_mvp;
uniform mat4 u_model;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_uv;
out float v_depth;

void main() {
    vec4 world  = u_model * vec4(a_pos, 1.0);
    v_world_pos = world.xyz;
    v_normal    = mat3(u_model) * a_normal;
    v_uv        = a_uv;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_depth     = gl_Position.z / gl_Position.w;
}
)glsl";

static const char* FRAG_WORLD_SRC = R"glsl(
#version 330 core
in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_uv;
in float v_depth;

uniform sampler2D u_tex;
uniform vec3  u_cam_pos;
uniform float u_time;
uniform vec3  u_flash_pos;
uniform vec3  u_flash_dir;
uniform float u_flash_on;
uniform float u_flash_intensity;
uniform vec3  u_light_pos[8];
uniform vec3  u_light_color[8];
uniform float u_light_radius[8];
uniform int   u_light_count;
uniform float u_ambient;

out vec4 frag_color;

void main() {
    vec3 albedo = texture(u_tex, v_uv).rgb;
    vec3 N      = normalize(v_normal);
    vec3 color  = albedo * u_ambient;

    for (int i = 0; i < u_light_count; i++) {
        vec3  L    = u_light_pos[i] - v_world_pos;
        float dist = length(L);
        if (dist < u_light_radius[i]) {
            float atten = 1.0 - (dist / u_light_radius[i]);
            atten = atten * atten;
            float diff = max(dot(N, normalize(L)), 0.0);
            color += albedo * u_light_color[i] * diff * atten;
        }
    }

    if (u_flash_on > 0.5) {
        vec3  L      = u_flash_pos - v_world_pos;
        float dist   = length(L);
        vec3  Lnorm  = L / dist;
        float theta  = dot(Lnorm, normalize(-u_flash_dir));
        float outer  = 0.85;
        float cutoff = 0.92;
        if (theta > outer && dist < 20.0) {
            float atten = 1.0 - clamp(dist / 20.0, 0.0, 1.0);
            atten *= atten;
            float spot = clamp((theta - outer) / (cutoff - outer), 0.0, 1.0);
            float diff = max(dot(N, Lnorm), 0.0);
            color += albedo * vec3(0.9, 0.85, 0.7) * diff * atten * spot * u_flash_intensity;
        }
    }

    float dist_to_cam = length(u_cam_pos - v_world_pos);
    float fog = clamp((dist_to_cam - 5.0) / 25.0, 0.0, 1.0);
    fog = fog * fog;
    color = mix(color, vec3(0.0, 0.0, 0.02), fog);

    frag_color = vec4(color, 1.0);
}
)glsl";

static const char* VERT_POST_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv        = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)glsl";

static const char* FRAG_POST_SRC = R"glsl(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_screen;
uniform float u_time;
uniform float u_health;
uniform float u_flash_on;
uniform float u_heartbeat;
out vec4 frag_color;

void main() {
    vec2 uv = v_uv;

    float ca_strength = 0.003 + (1.0 - u_health) * 0.006;
    float ca_dist     = length(uv - 0.5) * ca_strength;
    vec2  ca_dir      = normalize(uv - 0.5 + vec2(0.0001)) * ca_dist;

    float r = texture(u_screen, uv + ca_dir).r;
    float g = texture(u_screen, uv).g;
    float b = texture(u_screen, uv - ca_dir).b;
    vec3 color = vec3(r, g, b);

    float vig_base   = 0.35;
    float vig_health = (1.0 - u_health) * 0.35;
    float vig_beat   = u_heartbeat * 0.1;
    float vignette   = vig_base + vig_health + vig_beat;
    float vig_factor = 1.0 - smoothstep(0.4, 1.0, length((uv - 0.5) * 1.6)) * vignette * 2.0;
    color *= max(vig_factor, 0.0);

    if (u_health < 0.35) {
        float blood_edge  = smoothstep(0.3, 0.9, length((uv - 0.5) * 1.5));
        float blood_pulse = u_heartbeat;
        color = mix(color, vec3(0.4, 0.0, 0.0), blood_edge * (0.3 + blood_pulse * 0.4));
    }

    if (u_flash_on > 0.5) {
        float center = 1.0 - smoothstep(0.0, 0.3, length(uv - 0.5));
        color += vec3(0.04, 0.03, 0.01) * center;
    }

    float gt = u_time * 7919.0;
    float gh = fract(sin(dot(uv * 1000.0, vec2(12.9898, 78.233)) + gt) * 43758.5453);
    color += (gh - 0.5) * 0.03;

    frag_color = vec4(color, 1.0);
}
)glsl";

static const char* VERT_HUD_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
uniform vec2 u_screen_size;
out vec2 v_uv;
out vec4 v_color;
void main() {
    vec2 ndc    = (a_pos / u_screen_size) * 2.0 - 1.0;
    ndc.y       = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv        = a_uv;
    v_color     = a_color;
}
)glsl";

static const char* FRAG_HUD_SRC = R"glsl(
#version 330 core
in vec2 v_uv;
in vec4 v_color;
uniform sampler2D u_tex;
uniform int u_use_tex;
out vec4 frag_color;
void main() {
    if (u_use_tex == 1)
        frag_color = texture(u_tex, v_uv) * v_color;
    else
        frag_color = v_color;
    if (frag_color.a < 0.01) discard;
}
)glsl";

// ── Shader compilation ────────────────────────────────────────────────────────
static unsigned int compile_shader(unsigned int type, const char* src) {
    unsigned int sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    int ok; glGetShaderiv(sh, 0x8B81 /* GL_COMPILE_STATUS */, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        fprintf(stderr, "[VOID] Shader compile error:\n%s\n", log);
        platform_fatal("Shader compilation failed");
    }
    return sh;
}

static unsigned int link_program(const char* vsrc, const char* fsrc) {
    unsigned int vs = compile_shader(0x8B31 /* GL_VERTEX_SHADER   */, vsrc);
    unsigned int fs = compile_shader(0x8B30 /* GL_FRAGMENT_SHADER */, fsrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    int ok; glGetProgramiv(prog, 0x8B82 /* GL_LINK_STATUS */, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "[VOID] Program link error:\n%s\n", log);
        platform_fatal("Shader link failed");
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ── FBO creation ─────────────────────────────────────────────────────────────
static void create_fbo(int w, int h) {
    s_fbo_w = w; s_fbo_h = h;

    if (g_fbo) {
        glDeleteFramebuffers(1, &g_fbo);
        glDeleteRenderbuffers(1, &g_fbo_depth_rbo);
        unsigned int t = g_fbo_color_tex;
        glDeleteTextures(1, &t);
    }

    // Color texture
    glGenTextures(1, &g_fbo_color_tex);
    glBindTexture(GL_TEXTURE_2D, g_fbo_color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Depth renderbuffer
    glGenRenderbuffers(1, &g_fbo_depth_rbo);
    glBindRenderbuffer(0x8D41 /* GL_RENDERBUFFER */, g_fbo_depth_rbo);
    glRenderbufferStorage(0x8D41, 0x81A5 /* GL_DEPTH_COMPONENT24 */, w, h);

    // FBO
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, g_fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, GL_TEXTURE_2D, g_fbo_color_tex, 0);
    glFramebufferRenderbuffer(0x8D40, 0x8D00 /* GL_DEPTH_ATTACHMENT */, 0x8D41, g_fbo_depth_rbo);

    unsigned int status = glCheckFramebufferStatus(0x8D40);
    if (status != 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */)
        platform_fatal("Framebuffer not complete");

    glBindFramebuffer(0x8D40, 0);
}

// ── Fullscreen quad ───────────────────────────────────────────────────────────
static void create_fullscreen_quad() {
    static const float quad[] = {
        -1.f,-1.f, 0.f,0.f,
         1.f,-1.f, 1.f,0.f,
         1.f, 1.f, 1.f,1.f,
        -1.f,-1.f, 0.f,0.f,
         1.f, 1.f, 1.f,1.f,
        -1.f, 1.f, 0.f,1.f,
    };
    glGenVertexArrays(1, &s_quad_vao);
    glBindVertexArray(s_quad_vao);
    glGenBuffers(1, &s_quad_vbo);
    glBindBuffer(0x8892, s_quad_vbo);
    glBufferData(0x8892, sizeof(quad), quad, 0x88B8);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, 16, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, 16, (void*)8);
    glBindVertexArray(0);
}

// ── HUD VAO ───────────────────────────────────────────────────────────────────
static void create_hud_vao() {
    glGenVertexArrays(1, &s_hud_vao);
    glBindVertexArray(s_hud_vao);
    glGenBuffers(1, &s_hud_vbo);
    glBindBuffer(0x8892, s_hud_vbo);
    glBufferData(0x8892, sizeof(s_hud_buf), nullptr, 0x88E8 /* GL_STREAM_DRAW */);
    // layout: a_pos(2) a_uv(2) a_color(4) — stride = 8 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, 32, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, 32, (void*)8);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, 0, 32, (void*)16);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCEDURAL TEXTURE GENERATION
// ─────────────────────────────────────────────────────────────────────────────
#include "vmath.h"

static void gen_concrete(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float base  = noise_fbm(nx*4.f,ny*4.f,5);
        float crack = noise_voronoi(nx*8.f,ny*8.f);
        crack = 1.f - math_clamp(crack*6.f,0.f,1.f);
        float val = math_clamp(base*0.6f+0.2f-crack*0.3f,0.f,1.f);
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)(val*140);
        px[i+1]=(unsigned char)(val*138);
        px[i+2]=(unsigned char)(val*145);
        px[i+3]=255;
    }
}

static void gen_metal(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float streak = noise_value2d(nx*2.f,ny*16.f)*0.3f;
        float base   = noise_fbm(nx*8.f,ny*8.f,3)*0.2f;
        float rust   = noise_fbm(nx*5.f+1.7f,ny*5.f+2.3f,4);
        float is_rust= math_clamp((rust-0.55f)*5.f,0.f,1.f);
        float val = 0.25f+base+streak;
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)(math_lerp(val*160,val*110+60,is_rust));
        px[i+1]=(unsigned char)(math_lerp(val*158,val*50,is_rust));
        px[i+2]=(unsigned char)(math_lerp(val*165,val*20,is_rust));
        px[i+3]=255;
    }
}

static void gen_floor(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float tile_x=math_fract(nx*4.f), tile_y=math_fract(ny*4.f);
        float grout=(tile_x<0.06f||tile_y<0.06f)?1.f:0.f;
        float grime=noise_fbm(nx*6.f,ny*6.f,3)*0.3f;
        float val = grout>0.5f?(0.08f+grime*0.1f):(0.35f+grime);
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)(val*180);
        px[i+1]=(unsigned char)(val*175);
        px[i+2]=(unsigned char)(val*170);
        px[i+3]=255;
    }
}

static void gen_blood(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float splat=noise_fbm(nx*5.f,ny*5.f,4);
        float mask =math_clamp((splat-0.4f)*3.f,0.f,1.f);
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)(mask*120);
        px[i+1]=(unsigned char)(mask*5);
        px[i+2]=(unsigned char)(mask*5);
        px[i+3]=(unsigned char)(mask*255);
    }
}

static void gen_panel(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float base=noise_fbm(nx*20.f,ny*20.f,2)*0.05f+0.12f;
        float scan=((y%3)==0)?0.06f:0.f;
        int gx=(int)(nx*8.f), gy=(int)(ny*4.f);
        float fx=math_fract(nx*8.f)-0.5f, fy=math_fract(ny*4.f)-0.5f;
        float dot_dist=math_sqrt(fx*fx+fy*fy);
        float dot=(dot_dist<0.15f)?1.f:0.f;
        float dot_on=noise_hash((float)(gx+gy*8))>0.6f?1.f:0.f;
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)math_clamp((base+scan)*200+dot*dot_on*100,0,255);
        px[i+1]=(unsigned char)math_clamp((base+scan)*220+dot*dot_on*255,0,255);
        px[i+2]=(unsigned char)math_clamp((base+scan)*200+dot*dot_on*80, 0,255);
        px[i+3]=255;
    }
}

static void gen_enemy_skin(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float base=noise_fbm(nx*6.f,ny*6.f,4)*0.4f+0.15f;
        float bump=noise_value2d(nx*12.f,ny*12.f)*0.1f;
        int i=(y*64+x)*4;
        px[i+0]=(unsigned char)((base+bump)*60);
        px[i+1]=(unsigned char)((base+bump)*80);
        px[i+2]=(unsigned char)((base+bump)*50);
        px[i+3]=255;
    }
}

static void gen_weapon(unsigned char* px) {
    for(int y=0;y<64;y++) for(int x=0;x<64;x++){
        float nx=x/64.f, ny=y/64.f;
        float grip=(y%4<2)?0.8f:1.f;
        float val=(noise_fbm(nx*10.f,ny*10.f,3)*0.2f+0.3f)*grip;
        unsigned char v=(unsigned char)(val*120);
        int i=(y*64+x)*4;
        px[i+0]=px[i+1]=px[i+2]=v; px[i+3]=255;
    }
}

unsigned int texture_create(int w, int h, const unsigned char* rgba) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,0x2703); // GL_LINEAR_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    return id;
}

unsigned int texture_generate(TextureID id) {
    static unsigned char px[64*64*4];
    switch(id){
        case TEX_CONCRETE:   gen_concrete(px);   break;
        case TEX_METAL:      gen_metal(px);      break;
        case TEX_FLOOR:      gen_floor(px);      break;
        case TEX_BLOOD:      gen_blood(px);      break;
        case TEX_PANEL:      gen_panel(px);      break;
        case TEX_ENEMY_SKIN: gen_enemy_skin(px); break;
        case TEX_WEAPON:     gen_weapon(px);     break;
        default: break;
    }
    g_textures[id] = texture_create(64,64,px);
    return g_textures[id];
}

void texture_bind(unsigned int id, int slot) {
    glActiveTexture(0x84C0 + slot); // GL_TEXTURE0 + slot
    glBindTexture(GL_TEXTURE_2D, id);
}

// ── Mesh ──────────────────────────────────────────────────────────────────────
Mesh mesh_create(const float* verts, int vcount, int fpv,
                 const unsigned int* idx, int icount) {
    Mesh m={};
    m.vertex_count=vcount; m.index_count=icount;
    m.indexed=(icount>0);

    glGenVertexArrays(1,&m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1,&m.vbo);
    glBindBuffer(0x8892,m.vbo);
    glBufferData(0x8892,(long)(vcount*fpv*sizeof(float)),verts,0x88B8);

    // Vertex layout: pos(3) + normal(3) + uv(2) = 8 floats
    int stride=fpv*sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,0,stride,(void*)0);
    if(fpv>=6){
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,0,stride,(void*)12);
    }
    if(fpv>=8){
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2,GL_FLOAT,0,stride,(void*)24);
    }

    if(icount>0){
        glGenBuffers(1,&m.ebo);
        glBindBuffer(0x8893,m.ebo); // GL_ELEMENT_ARRAY_BUFFER
        glBufferData(0x8893,(long)(icount*sizeof(unsigned int)),idx,0x88B8);
    }
    glBindVertexArray(0);
    return m;
}

void mesh_destroy(Mesh& m) {
    if(m.indexed && m.ebo) glDeleteBuffers(1,&m.ebo);
    if(m.vbo) glDeleteBuffers(1,&m.vbo);
    if(m.vao) glDeleteVertexArrays(1,&m.vao);
    m={};
}

void mesh_draw(const Mesh& m) {
    glBindVertexArray(m.vao);
    if(m.indexed)
        glDrawElements(0x0004,m.index_count,GL_UNSIGNED_INT,(void*)0);
    else
        glDrawArrays(0x0004,0,m.vertex_count);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// RENDERER INIT / SHUTDOWN
// ─────────────────────────────────────────────────────────────────────────────
bool renderer_init(int w, int h) {
    // Depth test, back-face culling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Alpha blending (for HUD)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shaders
    s_prog_world = link_program(VERT_WORLD_SRC, FRAG_WORLD_SRC);
    s_prog_post  = link_program(VERT_POST_SRC,  FRAG_POST_SRC);
    s_prog_hud   = link_program(VERT_HUD_SRC,   FRAG_HUD_SRC);

    // FBO + fullscreen quad + HUD vao
    create_fbo(w, h);
    create_fullscreen_quad();
    create_hud_vao();

    // Generate all procedural textures
    for(int i=0;i<TEX_COUNT;i++) texture_generate((TextureID)i);

    return true;
}

void renderer_shutdown() {
    if(s_prog_world) glDeleteProgram(s_prog_world);
    if(s_prog_post)  glDeleteProgram(s_prog_post);
    if(s_prog_hud)   glDeleteProgram(s_prog_hud);
    if(g_fbo)        glDeleteFramebuffers(1,&g_fbo);
    if(g_fbo_depth_rbo) glDeleteRenderbuffers(1,&g_fbo_depth_rbo);
    for(int i=0;i<TEX_COUNT;i++) if(g_textures[i]) glDeleteTextures(1,&g_textures[i]);
}

void renderer_resize(int w, int h) {
    glViewport(0,0,w,h);
    create_fbo(w,h);
}

// ─────────────────────────────────────────────────────────────────────────────
// FRAME RENDERING
// ─────────────────────────────────────────────────────────────────────────────
void renderer_begin_frame(const RenderState& rs) {
    // Render world into FBO
    glBindFramebuffer(0x8D40, g_fbo);
    glViewport(0,0,rs.screen_w,rs.screen_h);
    glClearColor(0.0f,0.0f,0.01f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

static void set_world_uniforms(unsigned int prog, const RenderState& rs,
                                const Mat4& model, unsigned int tex_id) {
    glUseProgram(prog);

    Mat4 mvp = rs.proj * rs.view * model;
    glUniformMatrix4fv(glGetUniformLocation(prog,"u_mvp"),  1, 0, mvp.ptr());
    glUniformMatrix4fv(glGetUniformLocation(prog,"u_model"),1, 0, model.ptr());

    glUniform3f(glGetUniformLocation(prog,"u_cam_pos"),
                rs.cam_pos.x, rs.cam_pos.y, rs.cam_pos.z);
    glUniform1f(glGetUniformLocation(prog,"u_time"),       rs.time);
    glUniform1f(glGetUniformLocation(prog,"u_ambient"),    rs.ambient);
    glUniform3f(glGetUniformLocation(prog,"u_flash_pos"),
                rs.flash_pos.x, rs.flash_pos.y, rs.flash_pos.z);
    glUniform3f(glGetUniformLocation(prog,"u_flash_dir"),
                rs.flash_dir.x, rs.flash_dir.y, rs.flash_dir.z);
    glUniform1f(glGetUniformLocation(prog,"u_flash_on"),       rs.flash_on);
    glUniform1f(glGetUniformLocation(prog,"u_flash_intensity"),rs.flash_intensity);
    glUniform1i(glGetUniformLocation(prog,"u_light_count"),    rs.light_count);

    // Upload light arrays
    static float lpos[24], lcol[24], lrad[8];
    for(int i=0;i<rs.light_count&&i<MAX_LIGHTS;i++){
        lpos[i*3+0]=rs.lights[i].position.x;
        lpos[i*3+1]=rs.lights[i].position.y;
        lpos[i*3+2]=rs.lights[i].position.z;
        lcol[i*3+0]=rs.lights[i].color.x;
        lcol[i*3+1]=rs.lights[i].color.y;
        lcol[i*3+2]=rs.lights[i].color.z;
        lrad[i]=rs.lights[i].radius;
    }
    if(rs.light_count>0){
        glUniform3fv(glGetUniformLocation(prog,"u_light_pos"),  rs.light_count, lpos);
        glUniform3fv(glGetUniformLocation(prog,"u_light_color"),rs.light_count, lcol);
        glUniform1fv(glGetUniformLocation(prog,"u_light_radius"),rs.light_count,lrad);
    }

    texture_bind(tex_id, 0);
    glUniform1i(glGetUniformLocation(prog,"u_tex"), 0);
}

void renderer_draw_world(const Mesh& m, unsigned int tex_id,
                         const Mat4& model, const RenderState& rs) {
    set_world_uniforms(s_prog_world, rs, model, tex_id);
    mesh_draw(m);
}

void renderer_draw_entity(const Mesh& m, unsigned int tex_id,
                          const Mat4& model, const RenderState& rs) {
    set_world_uniforms(s_prog_world, rs, model, tex_id);
    mesh_draw(m);
}

void renderer_end_world(const RenderState& rs) {
    // Blit FBO to screen with post-processing
    glBindFramebuffer(0x8D40, 0);
    glViewport(0,0,rs.screen_w,rs.screen_h);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(s_prog_post);
    texture_bind(g_fbo_color_tex, 0);
    glUniform1i(glGetUniformLocation(s_prog_post,"u_screen"),    0);
    glUniform1f(glGetUniformLocation(s_prog_post,"u_time"),      rs.time);
    glUniform1f(glGetUniformLocation(s_prog_post,"u_health"),    rs.health_normalized);
    glUniform1f(glGetUniformLocation(s_prog_post,"u_flash_on"),  rs.flash_on);
    glUniform1f(glGetUniformLocation(s_prog_post,"u_heartbeat"), rs.heartbeat_pulse);

    glBindVertexArray(s_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// HUD RENDERING
// ─────────────────────────────────────────────────────────────────────────────
void renderer_begin_hud() {
    s_hud_verts = 0;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Emit one quad (2 triangles = 6 verts) into batch buffer
static void emit_quad(float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1,
                       float r, float g2, float b, float a) {
    if(s_hud_verts + 6 > HUD_MAX_VERTS) return;
    float* p = s_hud_buf + s_hud_verts * 8;
    // triangle 1
    p[ 0]=x;   p[ 1]=y;   p[ 2]=u0; p[ 3]=v0; p[ 4]=r; p[ 5]=g2; p[ 6]=b; p[ 7]=a;
    p[ 8]=x+w; p[ 9]=y;   p[10]=u1; p[11]=v0; p[12]=r; p[13]=g2; p[14]=b; p[15]=a;
    p[16]=x+w; p[17]=y+h; p[18]=u1; p[19]=v1; p[20]=r; p[21]=g2; p[22]=b; p[23]=a;
    // triangle 2
    p[24]=x;   p[25]=y;   p[26]=u0; p[27]=v0; p[28]=r; p[29]=g2; p[30]=b; p[31]=a;
    p[32]=x+w; p[33]=y+h; p[34]=u1; p[35]=v1; p[36]=r; p[37]=g2; p[38]=b; p[39]=a;
    p[40]=x;   p[41]=y+h; p[42]=u0; p[43]=v1; p[44]=r; p[45]=g2; p[46]=b; p[47]=a;
    s_hud_verts += 6;
}

void renderer_draw_rect(float x, float y, float w, float h,
                         float r, float g2, float b, float a) {
    emit_quad(x,y,w,h, 0,0,1,1, r,g2,b,a);
}

void renderer_draw_rect_uv(float x, float y, float w, float h, unsigned int tex,
                            float r, float g2, float b, float a) {
    // Flush current batch first (different texture)
    if(s_hud_verts > 0) {
        glUseProgram(s_prog_hud);
        float sw=(float)g_platform.width, sh=(float)g_platform.height;
        glUniform2f(glGetUniformLocation(s_prog_hud,"u_screen_size"),sw,sh);
        glUniform1i(glGetUniformLocation(s_prog_hud,"u_use_tex"),0);
        glBindVertexArray(s_hud_vao);
        glBindBuffer(0x8892,s_hud_vbo);
        glBufferSubData(0x8892,0,(long)(s_hud_verts*8*sizeof(float)),s_hud_buf);
        glDrawArrays(GL_TRIANGLES,0,s_hud_verts);
        s_hud_verts=0;
    }
    // Draw textured rect immediately
    float tmp[6*8];
    s_hud_buf[0]=x;   s_hud_buf[1]=y;   s_hud_buf[2]=0; s_hud_buf[3]=0; s_hud_buf[4]=r; s_hud_buf[5]=g2; s_hud_buf[6]=b; s_hud_buf[7]=a;
    emit_quad(x,y,w,h,0,0,1,1,r,g2,b,a);
    (void)tmp;
    glUseProgram(s_prog_hud);
    float sw=(float)g_platform.width, sh=(float)g_platform.height;
    glUniform2f(glGetUniformLocation(s_prog_hud,"u_screen_size"),sw,sh);
    glUniform1i(glGetUniformLocation(s_prog_hud,"u_use_tex"),1);
    texture_bind(tex,0);
    glUniform1i(glGetUniformLocation(s_prog_hud,"u_tex"),0);
    glBindVertexArray(s_hud_vao);
    glBindBuffer(0x8892,s_hud_vbo);
    glBufferSubData(0x8892,0,(long)(s_hud_verts*8*sizeof(float)),s_hud_buf);
    glDrawArrays(GL_TRIANGLES,0,s_hud_verts);
    s_hud_verts=0;
}

void renderer_end_frame() {
    // Flush remaining HUD batch
    if(s_hud_verts > 0) {
        glUseProgram(s_prog_hud);
        float sw=(float)g_platform.width, sh=(float)g_platform.height;
        glUniform2f(glGetUniformLocation(s_prog_hud,"u_screen_size"),sw,sh);
        glUniform1i(glGetUniformLocation(s_prog_hud,"u_use_tex"),0);
        glBindVertexArray(s_hud_vao);
        glBindBuffer(0x8892,s_hud_vbo);
        glBufferSubData(0x8892,0,(long)(s_hud_verts*8*sizeof(float)),s_hud_buf);
        glDrawArrays(GL_TRIANGLES,0,s_hud_verts);
        glBindVertexArray(0);
        s_hud_verts=0;
    }
}
