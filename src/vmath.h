// math.h — VOID Horror FPS
#pragma once

// ── Constants ────────────────────────────────────────────────────────────────
static const float PI        = 3.14159265358979323846f;
static const float TWO_PI    = 6.28318530717958647692f;
static const float HALF_PI   = 1.57079632679489661923f;
static const float DEG2RAD   = PI / 180.0f;
static const float RAD2DEG   = 180.0f / PI;
static const float EPSILON   = 0.0001f;

// ── Vec2 ─────────────────────────────────────────────────────────────────────
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const;
    Vec2 operator-(const Vec2& o) const;
    Vec2 operator*(float s) const;
    Vec2 operator/(float s) const;
    Vec2& operator+=(const Vec2& o);
    Vec2& operator-=(const Vec2& o);
    float length() const;
    Vec2 normalized() const;
    float dot(const Vec2& o) const;
};

// ── Vec3 ─────────────────────────────────────────────────────────────────────
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const;
    Vec3 operator-(const Vec3& o) const;
    Vec3 operator*(float s) const;
    Vec3 operator/(float s) const;
    Vec3 operator-() const;
    Vec3& operator+=(const Vec3& o);
    Vec3& operator-=(const Vec3& o);
    Vec3& operator*=(float s);
    bool operator==(const Vec3& o) const;
    float length() const;
    float lengthSq() const;
    Vec3 normalized() const;
    float dot(const Vec3& o) const;
    Vec3 cross(const Vec3& o) const;
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t);
};

// ── Vec4 ─────────────────────────────────────────────────────────────────────
struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

// ── Mat4 ─────────────────────────────────────────────────────────────────────
struct Mat4 {
    float m[16]; // column-major
    Mat4();
    static Mat4 identity();
    static Mat4 translate(const Vec3& t);
    static Mat4 rotateX(float angle);
    static Mat4 rotateY(float angle);
    static Mat4 rotateZ(float angle);
    static Mat4 scale(const Vec3& s);
    static Mat4 perspective(float fovY, float aspect, float nearZ, float farZ);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    Mat4 operator*(const Mat4& o) const;
    Vec4 operator*(const Vec4& v) const;
    Vec3 transformPoint(const Vec3& p) const;
    Vec3 transformDir(const Vec3& d) const;
    const float* ptr() const { return m; }
};

// ── AABB ─────────────────────────────────────────────────────────────────────
struct AABB {
    Vec3 min, max;
    AABB() {}
    AABB(const Vec3& min, const Vec3& max) : min(min), max(max) {}
    bool intersects(const AABB& o) const;
    bool containsPoint(const Vec3& p) const;
    Vec3 center() const;
    Vec3 size() const;
};

// ── Ray ──────────────────────────────────────────────────────────────────────
struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
    bool intersectsAABB(const AABB& box, float& tMin, float& tMax) const;
    Vec3 at(float t) const { return origin + direction * t; }
};

// ── Scalar utilities ─────────────────────────────────────────────────────────
float math_sqrt(float x);
float math_abs(float x);
float math_sin(float x);
float math_cos(float x);
float math_tan(float x);
float math_atan2(float y, float x);
float math_acos(float x);
float math_floor(float x);
float math_ceil(float x);
float math_clamp(float v, float lo, float hi);
float math_lerp(float a, float b, float t);
float math_smoothstep(float edge0, float edge1, float x);
float math_fract(float x);
float math_mod(float x, float y);
int   math_mini(int a, int b);
int   math_maxi(int a, int b);
float math_minf(float a, float b);
float math_maxf(float a, float b);
float math_pow(float base, float exp);

// ── Noise (used by procedural textures) ──────────────────────────────────────
float noise_hash(float n);
float noise_value2d(float x, float y);
float noise_fbm(float x, float y, int octaves);   // Fractal Brownian Motion
float noise_voronoi(float x, float y);             // Cell/Worley noise
