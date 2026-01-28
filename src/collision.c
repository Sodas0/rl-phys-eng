#include "collision.h"
#include <math.h>

// --- Positional Correction ---
// Pushes overlapping bodies apart to prevent sinking
static void collision_positional_correction(Body *a, Body *b, Collision *col) {
    const float PERCENT = 0.2f;   // 20% of penetration corrected per iteration
    const float SLOP = 0.01f;     // Allow small overlap to prevent jitter
    
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;  // Both static
    
    float correction = fmaxf(col->penetration - SLOP, 0.0f) * PERCENT / inv_mass_sum;
    Vec2 correction_vec = vec2_scale(col->normal, correction);
    
    a->position = vec2_sub(a->position, vec2_scale(correction_vec, a->inv_mass));
    b->position = vec2_add(b->position, vec2_scale(correction_vec, b->inv_mass));
}

// --- Impulse-Based Collision Resolution ---
void collision_resolve(Body *a, Body *b, Collision *col) {
    // Early exit: both bodies are static
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;
    
    // Calculate relative velocity (b relative to a)
    Vec2 rel_vel = vec2_sub(b->velocity, a->velocity);
    
    // Relative velocity along collision normal
    float vel_along_normal = vec2_dot(rel_vel, col->normal);
    
    // Early exit: bodies are separating (moving apart)
    if (vel_along_normal > 0.0f) {
        // Still apply positional correction if overlapping
        collision_positional_correction(a, b, col);
        return;
    }
    
    // Calculate restitution (use minimum of the two bodies)
    float e = fminf(a->restitution, b->restitution);
    
    // Calculate impulse magnitude
    // j = -(1 + e) * v_rel_n / (inv_mass_a + inv_mass_b)
    float j = -(1.0f + e) * vel_along_normal / inv_mass_sum;
    
    // Apply impulse to velocities
    Vec2 impulse = vec2_scale(col->normal, j);
    a->velocity = vec2_sub(a->velocity, vec2_scale(impulse, a->inv_mass));
    b->velocity = vec2_add(b->velocity, vec2_scale(impulse, b->inv_mass));
    
    // Apply positional correction to prevent sinking
    collision_positional_correction(a, b, col);
}

int collision_detect_circles(const Body *a, const Body *b, Collision *out) {
    // Vector from A to B
    Vec2 ab = vec2_sub(b->position, a->position);
    
    // Distance squared between centers
    float dist_sq = vec2_len_sq(ab);
    float radius_sum = a->radius + b->radius;
    
    // Check if circles are overlapping
    if (dist_sq >= radius_sum * radius_sum) {
        return 0;  // No collision
    }
    
    float dist = sqrtf(dist_sq);
    
    // Handle case where circles are at the same position
    if (dist < 1e-8f) {
        out->normal = vec2(1.0f, 0.0f);  // Arbitrary direction
        out->penetration = radius_sum;
        out->contact = a->position;
    } else {
        // Normal points from A to B
        out->normal = vec2_scale(ab, 1.0f / dist);
        out->penetration = radius_sum - dist;
        // Contact point: on the surface of A, offset toward B
        out->contact = vec2_add(a->position, vec2_scale(out->normal, a->radius - out->penetration * 0.5f));
    }
    
    // body_a and body_b indices are set by the caller
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}
