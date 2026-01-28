#include "collision.h"

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
