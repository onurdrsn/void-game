// math.cpp — VOID Horror FPS
#include "vmath.h"
#include <cmath>

// ── Vec2 ─────────────────────────────────────────────────────────────────────
Vec2 Vec2::operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
Vec2 Vec2::operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
Vec2 Vec2::operator*(float s)        const { return Vec2(x * s, y * s); }
Vec2 Vec2::operator/(float s)        const { return Vec2(x / s, y / s); }
Vec2& Vec2::operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
Vec2& Vec2::operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
float Vec2::length() const    { return sqrtf(x*x + y*y); }
float Vec2::dot(const Vec2& o) const { return x*o.x + y*o.y; }
Vec2 Vec2::normalized() const {
    float l = length();
    if (l < EPSILON) return Vec2(0,0);
    return Vec2(x/l, y/l);
}

// ── Vec3 ─────────────────────────────────────────────────────────────────────
Vec3 Vec3::operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
Vec3 Vec3::operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
Vec3 Vec3::operator*(float s)        const { return Vec3(x*s, y*s, z*s); }
Vec3 Vec3::operator/(float s)        const { float inv = 1.0f/s; return Vec3(x*inv, y*inv, z*inv); }
Vec3 Vec3::operator-()               const { return Vec3(-x, -y, -z); }
Vec3& Vec3::operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
Vec3& Vec3::operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
Vec3& Vec3::operator*=(float s)       { x*=s; y*=s; z*=s; return *this; }
bool  Vec3::operator==(const Vec3& o) const {
    return fabsf(x-o.x)<EPSILON && fabsf(y-o.y)<EPSILON && fabsf(z-o.z)<EPSILON;
}
float Vec3::length()   const { return sqrtf(x*x + y*y + z*z); }
float Vec3::lengthSq() const { return x*x + y*y + z*z; }
float Vec3::dot(const Vec3& o)   const { return x*o.x + y*o.y + z*o.z; }
Vec3  Vec3::cross(const Vec3& o) const {
    return Vec3(y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x);
}
Vec3 Vec3::normalized() const {
    float l = length();
    if (l < EPSILON) return Vec3(0,0,0);
    float inv = 1.0f / l;
    return Vec3(x*inv, y*inv, z*inv);
}
Vec3 Vec3::lerp(const Vec3& a, const Vec3& b, float t) {
    return Vec3(a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t);
}

// ── Mat4 ─────────────────────────────────────────────────────────────────────
Mat4::Mat4() {
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
}

Mat4 Mat4::identity() {
    Mat4 r;
    r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f;
    return r;
}

Mat4 Mat4::translate(const Vec3& t) {
    Mat4 r = identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

Mat4 Mat4::rotateX(float a) {
    Mat4 r = identity();
    float c = cosf(a), s = sinf(a);
    r.m[5]=c;  r.m[9]=-s;
    r.m[6]=s;  r.m[10]=c;
    return r;
}

Mat4 Mat4::rotateY(float a) {
    Mat4 r = identity();
    float c = cosf(a), s = sinf(a);
    r.m[0]=c;  r.m[8]=s;
    r.m[2]=-s; r.m[10]=c;
    return r;
}

Mat4 Mat4::rotateZ(float a) {
    Mat4 r = identity();
    float c = cosf(a), s = sinf(a);
    r.m[0]=c;  r.m[4]=-s;
    r.m[1]=s;  r.m[5]=c;
    return r;
}

Mat4 Mat4::scale(const Vec3& s) {
    Mat4 r = identity();
    r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z;
    return r;
}

Mat4 Mat4::perspective(float fovY, float aspect, float nearZ, float farZ) {
    Mat4 r;
    float tanHalf = tanf(fovY * 0.5f);
    r.m[0]  =  1.0f / (aspect * tanHalf);
    r.m[5]  =  1.0f / tanHalf;
    r.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    return r;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalized();
    Vec3 r2 = f.cross(up).normalized();
    Vec3 u2 = r2.cross(f);
    Mat4 m;
    m.m[0] = r2.x; m.m[4] = r2.y; m.m[8]  = r2.z; m.m[12] = -r2.dot(eye);
    m.m[1] = u2.x; m.m[5] = u2.y; m.m[9]  = u2.z; m.m[13] = -u2.dot(eye);
    m.m[2] =-f.x;  m.m[6] =-f.y;  m.m[10] =-f.z;  m.m[14] =  f.dot(eye);
    m.m[15] = 1.0f;
    return m;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += m[k*4 + row] * o.m[col*4 + k];
            r.m[col*4 + row] = sum;
        }
    }
    return r;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    return Vec4(
        m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,
        m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,
        m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
        m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
    );
}

Vec3 Mat4::transformPoint(const Vec3& p) const {
    Vec4 r = (*this) * Vec4(p, 1.0f);
    float inv = 1.0f / r.w;
    return Vec3(r.x*inv, r.y*inv, r.z*inv);
}

Vec3 Mat4::transformDir(const Vec3& d) const {
    return Vec3(
        m[0]*d.x + m[4]*d.y + m[8]*d.z,
        m[1]*d.x + m[5]*d.y + m[9]*d.z,
        m[2]*d.x + m[6]*d.y + m[10]*d.z
    );
}

// ── AABB ─────────────────────────────────────────────────────────────────────
bool AABB::intersects(const AABB& o) const {
    return (min.x <= o.max.x && max.x >= o.min.x) &&
           (min.y <= o.max.y && max.y >= o.min.y) &&
           (min.z <= o.max.z && max.z >= o.min.z);
}
bool AABB::containsPoint(const Vec3& p) const {
    return p.x>=min.x && p.x<=max.x &&
           p.y>=min.y && p.y<=max.y &&
           p.z>=min.z && p.z<=max.z;
}
Vec3 AABB::center() const {
    return Vec3((min.x+max.x)*0.5f, (min.y+max.y)*0.5f, (min.z+max.z)*0.5f);
}
Vec3 AABB::size() const {
    return Vec3(max.x-min.x, max.y-min.y, max.z-min.z);
}

// ── Ray ──────────────────────────────────────────────────────────────────────
bool Ray::intersectsAABB(const AABB& box, float& tMin, float& tMax) const {
    tMin = -1e30f; tMax = 1e30f;
    float invD, t0, t1;
    // X
    invD = (fabsf(direction.x) > EPSILON) ? 1.0f/direction.x : 1e30f;
    t0 = (box.min.x - origin.x) * invD;
    t1 = (box.max.x - origin.x) * invD;
    if (invD < 0.0f) { float tmp=t0; t0=t1; t1=tmp; }
    tMin = t0 > tMin ? t0 : tMin;
    tMax = t1 < tMax ? t1 : tMax;
    if (tMax < tMin) return false;
    // Y
    invD = (fabsf(direction.y) > EPSILON) ? 1.0f/direction.y : 1e30f;
    t0 = (box.min.y - origin.y) * invD;
    t1 = (box.max.y - origin.y) * invD;
    if (invD < 0.0f) { float tmp=t0; t0=t1; t1=tmp; }
    tMin = t0 > tMin ? t0 : tMin;
    tMax = t1 < tMax ? t1 : tMax;
    if (tMax < tMin) return false;
    // Z
    invD = (fabsf(direction.z) > EPSILON) ? 1.0f/direction.z : 1e30f;
    t0 = (box.min.z - origin.z) * invD;
    t1 = (box.max.z - origin.z) * invD;
    if (invD < 0.0f) { float tmp=t0; t0=t1; t1=tmp; }
    tMin = t0 > tMin ? t0 : tMin;
    tMax = t1 < tMax ? t1 : tMax;
    return tMax >= tMin && tMax >= 0.0f;
}

// ── Scalar utilities ─────────────────────────────────────────────────────────
float math_sqrt(float x)            { return sqrtf(x); }
float math_abs(float x)             { return fabsf(x); }
float math_sin(float x)             { return sinf(x); }
float math_cos(float x)             { return cosf(x); }
float math_tan(float x)             { return tanf(x); }
float math_atan2(float y, float x)  { return atan2f(y, x); }
float math_acos(float x)            { return acosf(x); }
float math_floor(float x)           { return floorf(x); }
float math_ceil(float x)            { return ceilf(x); }
float math_pow(float base, float e) { return powf(base, e); }
float math_clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
float math_lerp(float a, float b, float t) { return a + (b-a)*t; }
float math_smoothstep(float e0, float e1, float x) {
    float t = math_clamp((x-e0)/(e1-e0), 0.0f, 1.0f);
    return t*t*(3.0f - 2.0f*t);
}
float math_fract(float x) { return x - floorf(x); }
float math_mod(float x, float y) { return fmodf(x, y); }
int   math_mini(int a, int b) { return a < b ? a : b; }
int   math_maxi(int a, int b) { return a > b ? a : b; }
float math_minf(float a, float b) { return a < b ? a : b; }
float math_maxf(float a, float b) { return a > b ? a : b; }

// ── Noise ────────────────────────────────────────────────────────────────────
float noise_hash(float n) {
    return math_fract(math_sin(n) * 43758.5453f);
}

float noise_value2d(float x, float y) {
    float ix = math_floor(x), iy = math_floor(y);
    float fx = math_fract(x), fy = math_fract(y);
    float ux = math_smoothstep(0.0f, 1.0f, fx);
    float uy = math_smoothstep(0.0f, 1.0f, fy);
    float h00 = noise_hash(ix       + iy       * 57.0f);
    float h10 = noise_hash(ix+1.0f  + iy       * 57.0f);
    float h01 = noise_hash(ix       + (iy+1.0f)* 57.0f);
    float h11 = noise_hash(ix+1.0f  + (iy+1.0f)* 57.0f);
    float a = math_lerp(h00, h10, ux);
    float b2 = math_lerp(h01, h11, ux);
    return math_lerp(a, b2, uy);
}

float noise_fbm(float x, float y, int octaves) {
    float value = 0.0f, amplitude = 0.5f, frequency = 1.0f, total = 0.0f;
    for (int i = 0; i < octaves; i++) {
        value     += amplitude * noise_value2d(x * frequency, y * frequency);
        total     += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return value / total;
}

float noise_voronoi(float x, float y) {
    float ix = math_floor(x), iy = math_floor(y);
    float min_dist = 1e30f;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float cx = ix + (float)dx, cy = iy + (float)dy;
            float rx = noise_hash(cx + cy * 37.0f);
            float ry = noise_hash(cx * 31.0f + cy + 17.0f);
            float px = cx + rx, py = cy + ry;
            float ddx = px - x, ddy = py - y;
            float d = ddx*ddx + ddy*ddy;
            if (d < min_dist) min_dist = d;
        }
    }
    return math_sqrt(min_dist);
}
