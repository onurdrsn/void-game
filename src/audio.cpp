// audio.cpp — VOID Horror FPS
// miniaudio single-header audio + procedural PCM sound synthesis
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include "vmath.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static const int   SAMPLE_RATE = 44100;
static const float TWO_PI_A    = 6.28318530717958647692f;

// ── Sound buffer storage ──────────────────────────────────────────────────────
struct SoundBuffer {
    float*  samples;
    int     count;
    bool    looping;
};

static SoundBuffer s_sounds[SND_COUNT];

// Sound sample pools (static arrays, no heap after init)
static float s_buf_footstep[1960];
static float s_buf_gunshot[13230];
static float s_buf_reload[22050];
static float s_buf_growl[22050];
static float s_buf_attack[8820];
static float s_buf_hum[88200];
static float s_buf_heart[88200];
static float s_buf_hurt[8820];
static float s_buf_die[44100];
static float s_buf_transition[44100];

// ── Active sound instances ────────────────────────────────────────────────────
static const int MAX_VOICES = 16;
struct Voice {
    int   sound_id;
    int   pos;       // sample position
    float volume;
    float pitch;     // not used for pitch shifting here (simplification)
    bool  active;
    bool  looping;
};
static Voice s_voices[MAX_VOICES];

// ── Miniaudio device ──────────────────────────────────────────────────────────
static ma_device  s_device;
static bool       s_device_ok = false;
static bool       s_ambient   = false;
static int        s_ambient_voice = -1;

// ── Audio callback ────────────────────────────────────────────────────────────
static void audio_callback(ma_device* pDev, void* pOut, const void*, ma_uint32 frameCount) {
    (void)pDev;
    float* out = (float*)pOut;
    memset(out, 0, frameCount * sizeof(float));

    for (int v = 0; v < MAX_VOICES; v++) {
        Voice& voice = s_voices[v];
        if (!voice.active) continue;

        SoundBuffer& buf = s_sounds[voice.sound_id];
        if (!buf.samples || buf.count == 0) { voice.active=false; continue; }

        for (ma_uint32 i = 0; i < frameCount; i++) {
            if (voice.pos >= buf.count) {
                if (voice.looping) voice.pos = 0;
                else { voice.active = false; break; }
            }
            out[i] += buf.samples[voice.pos++] * voice.volume;
        }
    }

    // Soft clip to prevent distortion
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float s = out[i];
        if (s >  1.0f) s =  1.0f;
        if (s < -1.0f) s = -1.0f;
        out[i] = s;
    }
}

// ── Sound synthesis ───────────────────────────────────────────────────────────
static void gen_footstep(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t    = i / (float)SAMPLE_RATE;
        float nz   = noise_hash((float)i * 0.001f) * 2.f - 1.f;
        float env  = expf(-t * 40.f);
        float thud = sinf(TWO_PI_A * 80.f * t) * expf(-t * 20.f);
        buf[i] = (nz * 0.3f + thud * 0.7f) * env;
    }
}

static void gen_gunshot(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t   = i / (float)SAMPLE_RATE;
        float nz  = noise_hash((float)i * 0.00137f) * 2.f - 1.f;
        float bang = sinf(TWO_PI_A * 120.f * t) * expf(-t * 60.f);
        float tail = nz * expf(-t * 8.f);
        buf[i] = (bang * 0.5f + tail * 0.5f) * 0.9f;
    }
}

static void gen_reload(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t = i / (float)SAMPLE_RATE;
        float c1 = 0.f, c2 = 0.f;
        // Click 1 at t=0.1s
        float t1 = t - 0.1f;
        if (t1 >= 0.f && t1 < 0.03f) {
            float nz = noise_hash((float)i * 0.013f) * 2.f - 1.f;
            c1 = (sinf(TWO_PI_A * 800.f * t1) + nz * 0.5f) * expf(-t1 * 200.f);
        }
        // Click 2 at t=0.35s
        float t2 = t - 0.35f;
        if (t2 >= 0.f && t2 < 0.03f) {
            float nz = noise_hash((float)(i+77) * 0.013f) * 2.f - 1.f;
            c2 = (sinf(TWO_PI_A * 600.f * t2) + nz * 0.5f) * expf(-t2 * 200.f) * 0.8f;
        }
        buf[i] = c1 + c2;
    }
}

static void gen_growl(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t   = i / (float)SAMPLE_RATE;
        float lfo = sinf(TWO_PI_A * 3.5f * t);
        float freq= 90.f + lfo * 20.f;
        float osc = sinf(TWO_PI_A * freq * t);
        float nz  = noise_hash((float)i * 0.0023f) * 2.f - 1.f;
        float env = math_clamp(t * 8.f, 0.f, 1.f) * expf(-t * 2.5f);
        buf[i] = (osc * 0.6f + nz * 0.4f) * env;
    }
}

static void gen_attack(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float nz = noise_hash((float)i * 0.007f) * 2.f - 1.f;
        float env= expf(-t * 15.f);
        buf[i]   = nz * env * 0.7f;
    }
}

static void gen_hum(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float h1 = sinf(TWO_PI_A * 60.f  * t) * 0.5f;
        float h2 = sinf(TWO_PI_A * 120.f * t) * 0.25f;
        float h3 = sinf(TWO_PI_A * 180.f * t) * 0.1f;
        float nz = noise_hash((float)i * 0.0041f) * 0.05f;
        buf[i]   = (h1 + h2 + h3 + nz) * 0.3f;
    }
}

static void gen_heartbeat(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float p1 = 0.f, p2 = 0.f;
        float t1 = t;
        p1 = sinf(TWO_PI_A * 40.f * t1) * expf(-t1 * 18.f);
        float t2 = t - 0.15f;
        if (t2 > 0.f)
            p2 = sinf(TWO_PI_A * 40.f * t2) * expf(-t2 * 18.f) * 0.6f;
        buf[i] = (p1 + p2) * 0.8f;
    }
}

static void gen_hurt(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float nz = noise_hash((float)i * 0.005f) * 2.f - 1.f;
        float sweep = expf(-t * t * 200.f);
        float env   = expf(-t * 20.f);
        buf[i] = nz * env + sinf(TWO_PI_A * (1000.f - t*3000.f) * t) * sweep * 0.5f;
    }
}

static void gen_die(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float nz = noise_hash((float)i * 0.003f) * 2.f - 1.f;
        float osc= sinf(TWO_PI_A * 55.f * t);
        float env= expf(-t * 1.5f);
        buf[i] = (osc * nz) * env * 0.7f;
    }
}

static void gen_transition(float* buf, int n) {
    for (int i = 0; i < n; i++) {
        float t  = i / (float)SAMPLE_RATE;
        float nz = noise_hash((float)i * 0.002f) * 2.f - 1.f;
        float bass = sinf(TWO_PI_A * 40.f * t) * nz * expf(-t * 2.f);
        float tone = sinf(TWO_PI_A * 440.f * t) *
                     math_smoothstep(0.f, 1.f, t) *
                     expf(-(1.f - t) * 3.f);
        buf[i] = bass * 0.7f + tone * 0.3f;
    }
}

// ── Init / shutdown ───────────────────────────────────────────────────────────
bool audio_init() {
    // Generate all sounds
    gen_footstep(s_buf_footstep, 1960);
    gen_gunshot(s_buf_gunshot,   13230);
    gen_reload(s_buf_reload,     22050);
    gen_growl(s_buf_growl,       22050);
    gen_attack(s_buf_attack,     8820);
    gen_hum(s_buf_hum,           88200);
    gen_heartbeat(s_buf_heart,   88200);
    gen_hurt(s_buf_hurt,         8820);
    gen_die(s_buf_die,           44100);
    gen_transition(s_buf_transition, 44100);

    s_sounds[SND_FOOTSTEP]      = {s_buf_footstep,   1960,  false};
    s_sounds[SND_GUNSHOT]       = {s_buf_gunshot,    13230, false};
    s_sounds[SND_RELOAD]        = {s_buf_reload,     22050, false};
    s_sounds[SND_ENEMY_GROWL]   = {s_buf_growl,      22050, false};
    s_sounds[SND_ENEMY_ATTACK]  = {s_buf_attack,     8820,  false};
    s_sounds[SND_AMBIENT_HUM]   = {s_buf_hum,        88200, true };
    s_sounds[SND_HEARTBEAT]     = {s_buf_heart,      88200, false};
    s_sounds[SND_PLAYER_HURT]   = {s_buf_hurt,       8820,  false};
    s_sounds[SND_PLAYER_DIE]    = {s_buf_die,        44100, false};
    s_sounds[SND_LEVEL_TRANSITION]={s_buf_transition,44100, false};

    memset(s_voices, 0, sizeof(s_voices));

    // Init miniaudio device
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 1;
    cfg.sampleRate        = SAMPLE_RATE;
    cfg.dataCallback      = audio_callback;
    cfg.pUserData         = nullptr;

    if (ma_device_init(nullptr, &cfg, &s_device) != MA_SUCCESS) {
        fprintf(stderr, "[VOID] Audio: failed to open playback device (continuing without audio)\n");
        s_device_ok = false;
        return true; // non-fatal
    }

    if (ma_device_start(&s_device) != MA_SUCCESS) {
        fprintf(stderr, "[VOID] Audio: failed to start device\n");
        ma_device_uninit(&s_device);
        s_device_ok = false;
        return true; // non-fatal
    }

    s_device_ok = true;
    return true;
}

void audio_shutdown() {
    if (s_device_ok) {
        ma_device_stop(&s_device);
        ma_device_uninit(&s_device);
        s_device_ok = false;
    }
}

void audio_play(SoundID id, float volume, float /*pitch*/) {
    if (!s_device_ok) return;
    // Find free voice
    for (int v = 0; v < MAX_VOICES; v++) {
        if (!s_voices[v].active) {
            s_voices[v].sound_id = id;
            s_voices[v].pos      = 0;
            s_voices[v].volume   = volume;
            s_voices[v].active   = true;
            s_voices[v].looping  = s_sounds[id].looping;
            return;
        }
    }
    // Steal oldest non-looping voice
    for (int v = 0; v < MAX_VOICES; v++) {
        if (!s_voices[v].looping) {
            s_voices[v].sound_id = id;
            s_voices[v].pos      = 0;
            s_voices[v].volume   = volume;
            s_voices[v].active   = true;
            s_voices[v].looping  = false;
            return;
        }
    }
}

void audio_play_3d(SoundID id, float volume, float pitch,
                   float sx, float sy, float sz,
                   float lx, float ly, float lz) {
    float dx=sx-lx, dy=sy-ly, dz=sz-lz;
    float dist = sqrtf(dx*dx+dy*dy+dz*dz);
    float atten = 1.f / (1.f + dist * 0.15f);
    atten = math_clamp(atten, 0.f, 1.f);
    audio_play(id, volume * atten, pitch);
}

void audio_set_ambient(bool on) {
    s_ambient = on;
    if (on && s_ambient_voice < 0) {
        // Find free voice for ambient loop
        for (int v = 0; v < MAX_VOICES; v++) {
            if (!s_voices[v].active) {
                s_voices[v].sound_id = SND_AMBIENT_HUM;
                s_voices[v].pos      = 0;
                s_voices[v].volume   = 0.4f;
                s_voices[v].active   = true;
                s_voices[v].looping  = true;
                s_ambient_voice      = v;
                return;
            }
        }
    } else if (!on && s_ambient_voice >= 0) {
        s_voices[s_ambient_voice].active = false;
        s_ambient_voice = -1;
    }
}

void audio_update(float /*dt*/) {
    // Ambient voice auto-restart if stolen
    if (s_ambient && s_ambient_voice >= 0) {
        if (!s_voices[s_ambient_voice].active) {
            s_ambient_voice = -1;
            audio_set_ambient(true);
        }
    }
}
