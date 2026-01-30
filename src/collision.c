#include "collision.h"
#include <math.h>

// --- Positional Correction ---
// Pushes overlapping bodies apart to prevent sinking
static void collision_positional_correction(Body *a, Body *b, Collision *col) {
    const float PERCENT = 0.2f;   // 20% of penetration corrected per iteration
    const float SLOP = 0.001f;     // small overlap to prevent jitter
    
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;  // both static
    
    float correction = fmaxf(col->penetration - SLOP, 0.0f) * PERCENT / inv_mass_sum;
    Vec2 correction_vec = vec2_scale(col->normal, correction);
    
    a->position = vec2_sub(a->position, vec2_scale(correction_vec, a->inv_mass));
    b->position = vec2_add(b->position, vec2_scale(correction_vec, b->inv_mass));
}


// --- Impulse-Based Collision Resolution with Angular Effects ---
void collision_resolve(Body *a, Body *b, Collision *col) {
    // Early exit if both bodies are static
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;
    
    // Vectors from body centers to contact point
    // These "moment arms" determine how much torque is generated
    Vec2 r_a = vec2_sub(col->contact, a->position);
    Vec2 r_b = vec2_sub(col->contact, b->position);
    
    // Velocity of contact point on each body
    // v = v_linear + ω × r
    // In 2D: ω × r = vec2_perp(r) * ω
    Vec2 vel_a = vec2_add(a->velocity, vec2_scale(vec2_perp(r_a), a->angular_velocity));
    Vec2 vel_b = vec2_add(b->velocity, vec2_scale(vec2_perp(r_b), b->angular_velocity));
    
    // Relative velocity at contact point (b relative to a)
    Vec2 rel_vel = vec2_sub(vel_b, vel_a);
    
    // Relative velocity along collision normal
    float vel_along_normal = vec2_dot(rel_vel, col->normal);
    
    // contact threshold to prevent jitter from micro-corrections (need to tweak a bit)
    const float REST_VEL_EPS = 5.0f;  // [pixels/sec] Used to avoid jitter -- i want to tweak a bit.
    
    // Early exit if bodies are separating or at rest
    if (vel_along_normal > -REST_VEL_EPS) {
        // Treat as resting contact - only apply positional correction
        collision_positional_correction(a, b, col);
        return;
    }
    
    // Calculate restitution (use minimum of the two bodies)
    float e = fminf(a->restitution, b->restitution);
    
    // Cross products of moment arm with normal
    // These measure how much the collision "off-center-ness" contributes to rotation
    float r_a_cross_n = vec2_cross(r_a, col->normal);
    float r_b_cross_n = vec2_cross(r_b, col->normal);
    
    // Calculate impulse magnitude with rotational inertia
    // j = -(1 + e) * v_rel_n / (inv_mass_a + inv_mass_b + I_a_term + I_b_term)
    // The inertia terms reduce impulse when rotation "absorbs" energy
    float inv_mass_sum_angular = inv_mass_sum + 
                                  r_a_cross_n * r_a_cross_n * a->inv_inertia +
                                  r_b_cross_n * r_b_cross_n * b->inv_inertia;
    
    // numerical guard against division by zero
    const float EPSILON = 1e-8f;
    if (inv_mass_sum_angular < EPSILON) {
        collision_positional_correction(a, b, col);
        return;
    }

    float j = -(1.0f + e) * vel_along_normal / inv_mass_sum_angular;
    
    // Apply linear impulse to velocities (same as before)
    Vec2 impulse = vec2_scale(col->normal, j);
    a->velocity = vec2_sub(a->velocity, vec2_scale(impulse, a->inv_mass));
    b->velocity = vec2_add(b->velocity, vec2_scale(impulse, b->inv_mass));
    
    // Apply angular impulse (torque = r × impulse)
    // In 2D: torque is a scalar = r_cross_impulse
    // Angular velocity change: Δω = torque * inv_inertia
    a->angular_velocity -= vec2_cross(r_a, impulse) * a->inv_inertia;
    b->angular_velocity += vec2_cross(r_b, impulse) * b->inv_inertia;
    
    // Apply positional correction to prevent sinking
    collision_positional_correction(a, b, col);
}

int collision_detect_circles(const Body *a, const Body *b, Collision *out) {
    // Vector from A to B
    Vec2 ab = vec2_sub(b->position, a->position);
    
    // Distance squared between centers
    float dist_sq = vec2_len_sq(ab);
    float radius_sum = a->shape.circle.radius + b->shape.circle.radius;
    
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
        out->contact = vec2_add(a->position, vec2_scale(out->normal, a->shape.circle.radius - out->penetration * 0.5f));
    }
    
    // body_a and body_b indices are set by the caller
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}

// Circle vs Oriented Bounding Box (OBB) collision detection
// Strategy: Transform circle into rectangle's local coordinate frame where rect is axis-aligned,
// perform AABB test, then transform results (normal, contact) back to world space.
// This handles rotated rectangles by using rect->angle
int collision_detect_circle_rect(const Body *circle, const Body *rect, Collision *out) {
    float radius = circle->shape.circle.radius;
    
    // Compute rectangle half-extents
    float half_w = rect->shape.rect.width * 0.5f;
    float half_h = rect->shape.rect.height * 0.5f;
    
    // Transform circle center into rectangle's local space (OBB approach)
    // 1. Translate to rect's origin
    Vec2 circle_local = vec2_sub(circle->position, rect->position);
    
    // 2. Rotate by -rect->angle to align with rect's local axes
    float cos_angle = cosf(-rect->angle);
    float sin_angle = sinf(-rect->angle);
    float local_x = circle_local.x * cos_angle - circle_local.y * sin_angle;
    float local_y = circle_local.x * sin_angle + circle_local.y * cos_angle;
    circle_local = vec2(local_x, local_y);
    
    // Now perform AABB collision test in local space
    // Clamp circle center to rectangle bounds to find closest point
    float closest_x = fmaxf(-half_w, fminf(circle_local.x, half_w));
    float closest_y = fmaxf(-half_h, fminf(circle_local.y, half_h));
    Vec2 closest_local = vec2(closest_x, closest_y);
    
    // Vector from closest point to circle center (in local space)
    Vec2 diff_local = vec2_sub(circle_local, closest_local);
    float dist_sq = vec2_len_sq(diff_local);
    
    // Check if circle center is inside the rectangle (in local space)
    int inside = (circle_local.x >= -half_w && circle_local.x <= half_w &&
                  circle_local.y >= -half_h && circle_local.y <= half_h);
    
    Vec2 normal_local;
    float penetration;
    Vec2 contact_local;
    
    if (!inside) {
        // Circle center is outside rectangle
        if (dist_sq >= radius * radius) {
            return 0;  // No collision
        }
        
        float dist = sqrtf(dist_sq);
        
        // Handle edge case: closest point is exactly at circle center
        if (dist < 1e-8f) {
            normal_local = vec2(1.0f, 0.0f);  // Arbitrary direction
            penetration = radius;
        } else {
            // Normal points from circle (A) toward rect (B) in local space
            normal_local = vec2_scale(diff_local, -1.0f / dist);
            penetration = radius - dist;
        }
        contact_local = closest_local;
    } else {
        // Circle center is inside rectangle - find closest edge (in local space)
        float dx_left = circle_local.x - (-half_w);
        float dx_right = half_w - circle_local.x;
        float dy_top = circle_local.y - (-half_h);
        float dy_bottom = half_h - circle_local.y;
        
        // Find minimum distance to edge
        float min_dist = dx_left;
        normal_local = vec2(1.0f, 0.0f);   // Escape left, normal points right
        contact_local = vec2(-half_w, circle_local.y);
        
        if (dx_right < min_dist) {
            min_dist = dx_right;
            normal_local = vec2(-1.0f, 0.0f);  // Escape right, normal points left
            contact_local = vec2(half_w, circle_local.y);
        }
        if (dy_top < min_dist) {
            min_dist = dy_top;
            normal_local = vec2(0.0f, 1.0f);   // Escape up, normal points down
            contact_local = vec2(circle_local.x, -half_h);
        }
        if (dy_bottom < min_dist) {
            min_dist = dy_bottom;
            normal_local = vec2(0.0f, -1.0f);  // Escape down, normal points up
            contact_local = vec2(circle_local.x, half_h);
        }
        
        penetration = radius + min_dist;
    }
    
    // Transform results back to world space
    // Rotate normal by +rect->angle
    cos_angle = cosf(rect->angle);
    sin_angle = sinf(rect->angle);
    float world_nx = normal_local.x * cos_angle - normal_local.y * sin_angle;
    float world_ny = normal_local.x * sin_angle + normal_local.y * cos_angle;
    out->normal = vec2(world_nx, world_ny);
    
    // Rotate and translate contact point to world space
    float world_cx = contact_local.x * cos_angle - contact_local.y * sin_angle;
    float world_cy = contact_local.x * sin_angle + contact_local.y * cos_angle;
    out->contact = vec2_add(vec2(world_cx, world_cy), rect->position);
    
    out->penetration = penetration;
    
    // body_a and body_b indices are set by the caller
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}

// Helper: Get the 4 corners of a rotated rectangle in world space
static void get_rect_corners(const Body *rect, Vec2 corners[4]) {
    float half_w = rect->shape.rect.width * 0.5f;
    float half_h = rect->shape.rect.height * 0.5f;
    float cos_angle = cosf(rect->angle);
    float sin_angle = sinf(rect->angle);
    
    // Local corners (top-left, top-right, bottom-right, bottom-left)
    Vec2 local[4] = {
        vec2(-half_w, -half_h),
        vec2( half_w, -half_h),
        vec2( half_w,  half_h),
        vec2(-half_w,  half_h)
    };
    
    // Transform to world space
    for (int i = 0; i < 4; i++) {
        float wx = local[i].x * cos_angle - local[i].y * sin_angle;
        float wy = local[i].x * sin_angle + local[i].y * cos_angle;
        corners[i] = vec2_add(vec2(wx, wy), rect->position);
    }
}

// Helper: Project a polygon (defined by corners) onto an axis and return [min, max]
static void project_corners_onto_axis(const Vec2 corners[4], Vec2 axis, float *min, float *max) {
    *min = *max = vec2_dot(corners[0], axis);
    
    for (int i = 1; i < 4; i++) {
        float projection = vec2_dot(corners[i], axis);
        if (projection < *min) *min = projection;
        if (projection > *max) *max = projection;
    }
}

// Helper: Find ALL vertices farthest in a given direction (support points)
// For edge-edge contacts, there can be 2 vertices with the same projection.
// Returns the number of support points found (1 or 2) and fills the output array.
static int get_support_points(const Vec2 corners[4], Vec2 direction, Vec2 out[2]) {
    const float TOLERANCE = 1e-4f;  // Consider vertices equal if within this distance
    
    float max_proj = vec2_dot(corners[0], direction);
    out[0] = corners[0];
    int count = 1;
    
    for (int i = 1; i < 4; i++) {
        float proj = vec2_dot(corners[i], direction);
        
        if (proj > max_proj + TOLERANCE) {
            // Found a new maximum - reset
            max_proj = proj;
            out[0] = corners[i];
            count = 1;
        } else if (fabsf(proj - max_proj) <= TOLERANCE) {
            // This vertex is equally far - it's part of the support edge
            if (count < 2) {
                out[count++] = corners[i];
            }
        }
    }
    
    return count;
}

// Helper: Check if two projection ranges overlap, return overlap amount
// Returns negative if separated, positive if overlapping
static float get_overlap(float min_a, float max_a, float min_b, float max_b) {
    // No overlap if ranges are separated
    if (max_a < min_b || max_b < min_a) {
        return -1.0f;  // Separated
    }
    
    // Calculate overlap (always positive when overlapping)
    float overlap1 = max_a - min_b;  // A's max overlaps B's min
    float overlap2 = max_b - min_a;  // B's max overlaps A's min
    
    return (overlap1 < overlap2) ? overlap1 : overlap2;
}

// Rectangle-Rectangle collision using Separating Axis Theorem (SAT)
// Tests 4 axes: 2 from each rectangle's edges
// Returns 1 if colliding, fills collision data in `out`
int collision_detect_rects(const Body *a, const Body *b, Collision *out) {
    // Get corners in world space
    Vec2 corners_a[4], corners_b[4];
    get_rect_corners(a, corners_a);
    get_rect_corners(b, corners_b);
    
    // Get the two axis directions for each rectangle
    // Axis 0: edge direction (right vector)
    // Axis 1: perpendicular to edge (up vector)
    Vec2 axes[4];
    axes[0] = vec2(cosf(a->angle), sinf(a->angle));      // A's right axis
    axes[1] = vec2(-sinf(a->angle), cosf(a->angle));     // A's up axis
    axes[2] = vec2(cosf(b->angle), sinf(b->angle));      // B's right axis
    axes[3] = vec2(-sinf(b->angle), cosf(b->angle));     // B's up axis
    
    // Track minimum overlap axis (for collision resolution)
    float min_overlap = INFINITY;
    Vec2 collision_axis = VEC2_ZERO;
    
    // Test all 4 axes for separation
    for (int i = 0; i < 4; i++) {
        Vec2 axis = axes[i];
        
        // Normalize axis (should already be normalized, but ensure it)
        float len = vec2_len(axis);
        if (len < 1e-8f) continue;
        axis = vec2_scale(axis, 1.0f / len);
        
        // Project both rectangles onto this axis
        float min_a, max_a, min_b, max_b;
        project_corners_onto_axis(corners_a, axis, &min_a, &max_a);
        project_corners_onto_axis(corners_b, axis, &min_b, &max_b);
        
        // Check for separation
        float overlap = get_overlap(min_a, max_a, min_b, max_b);
        
        if (overlap < 0.0f) {
            // Found separating axis - no collision
            return 0;
        }
        
        // Track axis with minimum overlap (that's our collision axis)
        if (overlap < min_overlap) {
            min_overlap = overlap;
            collision_axis = axis;
        }
    }
    
    // No separating axis found - rectangles are colliding
    out->penetration = min_overlap;
    
    // Ensure normal points from A to B
    Vec2 ab = vec2_sub(b->position, a->position);
    if (vec2_dot(collision_axis, ab) < 0.0f) {
        collision_axis = vec2_negate(collision_axis);
    }
    out->normal = collision_axis;
    
    // Calculate contact point using support points (handles edge-edge contacts):
    // - Find all vertices on A farthest toward B (1 for vertex, 2 for edge)
    // - Find all vertices on B farthest toward A (1 for vertex, 2 for edge)
    // - Average all support points to get contact at the collision interface
    Vec2 supports_a[2], supports_b[2];
    int count_a = get_support_points(corners_a, out->normal, supports_a);
    int count_b = get_support_points(corners_b, vec2_negate(out->normal), supports_b);
    
    // Average all support vertices to get the contact point
    Vec2 contact_sum = VEC2_ZERO;
    for (int i = 0; i < count_a; i++) {
        contact_sum = vec2_add(contact_sum, supports_a[i]);
    }
    for (int i = 0; i < count_b; i++) {
        contact_sum = vec2_add(contact_sum, supports_b[i]);
    }
    out->contact = vec2_scale(contact_sum, 1.0f / (count_a + count_b));
    
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}
