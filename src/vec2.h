#ifndef VEC2_H
#define VEC2_H

#include <math.h>

typedef struct {
    float x;
    float y;
} Vec2;

// Constructor macro for convenience
#define vec2(x, y) ((Vec2){(x), (y)})

// Zero vector constant
#define VEC2_ZERO ((Vec2){0.0f, 0.0f})

// --- Basic Arithmetic ---

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

static inline Vec2 vec2_scale(Vec2 v, float s) {
    return (Vec2){v.x * s, v.y * s};
}

static inline Vec2 vec2_negate(Vec2 v) {
    return (Vec2){-v.x, -v.y};
}

// --- Products ---

static inline float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

// 2D cross product returns a scalar (the z-component of the 3D cross product)
// Useful for determining winding order and calculating torque
static inline float vec2_cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

// --- Length Operations ---

static inline float vec2_len_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

static inline float vec2_len(Vec2 v) {
    return sqrtf(vec2_len_sq(v));
}

// Returns unit vector, or zero vector if degenerate (near-zero length)
static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_len(v);
    if (len < 1e-8f) {
        return VEC2_ZERO;
    }
    return (Vec2){v.x / len, v.y / len};
}

// Returns unit vector, assumes non-zero input (no safety check)
static inline Vec2 vec2_normalize_unsafe(Vec2 v) {
    float len = vec2_len(v);
    return (Vec2){v.x / len, v.y / len};
}

static inline float vec2_dist(Vec2 a, Vec2 b) {
    return vec2_len(vec2_sub(b, a));
}

// --- Utilities ---

// Returns perpendicular vector (90 degrees counter-clockwise)
static inline Vec2 vec2_perp(Vec2 v) {
    return (Vec2){-v.y, v.x};
}

// Linear interpolation: returns a + t*(b - a)
static inline Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y)
    };
}

#endif // VEC2_H
