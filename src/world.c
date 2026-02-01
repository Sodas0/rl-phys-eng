#include "world.h"
#include "render.h"
#include "collision.h"
#include <stdlib.h>
#include <stdint.h>

void world_init(World *w, Vec2 gravity, float dt) {
    w->body_count = 0;
    w->gravity = gravity;
    w->dt = dt;
    w->bounds_enabled = 0;
    w->actuator_body_index = -1;
    w->actuator_pivot = (Vec2){0.0f, 0.0f};

    // Default debug flags (all off)
    w->debug.show_velocity = 0;
    w->debug.show_contacts = 0;
    w->debug.show_normals = 0;

    // Initialize RNG with a default seed
    world_seed(w, 1);
}

void world_set_bounds(World *w, float left, float top, float right, float bottom) {
    w->bound_left = left;
    w->bound_top = top;
    w->bound_right = right;
    w->bound_bottom = bottom;
    w->bounds_enabled = 1;
}

int world_add_body(World *w, Body b) {
    if (w->body_count >= MAX_BODIES) {
        return -1;  // World is full
    }
    int index = w->body_count;
    w->bodies[index] = b;
    w->body_count++;
    return index;
}

Body* world_get_body(World *w, int index) {
    if (index < 0 || index >= w->body_count) {
        return NULL;
    }
    return &w->bodies[index];
}

// --- Internal helper functions ---

static void integrate_bodies(World *w) {
    for (int i = 0; i < w->body_count; i++) {
        Body *b = &w->bodies[i];
        
        if (body_is_static(b)) continue;
        
        // Semi-implicit Euler: update velocity first, then position
        b->velocity = vec2_add(b->velocity, vec2_scale(w->gravity, w->dt));
        b->position = vec2_add(b->position, vec2_scale(b->velocity, w->dt));
        
        // Angular integration (no torque sources yet, but structure supports it)
        // angular_velocity would be updated by torque here if we had it

        // 1. Integrate angular velocity from torque
        // b->angular_velocity += b->torque * b->inv_inertia * dt;

        // // 2. Integrate angle from angular velocity
        // b->angle += b->angular_velocity * dt;

        // // 3. Clear torque
        // b->torque = 0.0f;

        b->angle += b->angular_velocity * w->dt;
    }
}

// --- Shape vs Plane helpers ---
// Boundaries are treated as infinite static planes.

static void resolve_circle_vs_bounds(Body *b, float left, float top, float right, float bottom) {
    float radius = b->shape.circle.radius;
    // REST_VEL_EPS: Resting contact threshold (0.05 m/s = 5 pixels/s)
    // Bodies moving slower than this are treated as at rest to prevent jitter
    const float REST_VEL_EPS = 0.05f * PIXELS_PER_METER;  // 5.0 pixels/sec
    
    // Left wall
    if (b->position.x - radius < left) {
        b->position.x = left + radius;
        if (fabsf(b->velocity.x) > REST_VEL_EPS) {
            b->velocity.x = -b->velocity.x * b->restitution;
        } else {
            b->velocity.x = 0.0f;  // Kill micro-velocity to prevent jitter
        }
    }
    // Right wall
    if (b->position.x + radius > right) {
        b->position.x = right - radius;
        if (fabsf(b->velocity.x) > REST_VEL_EPS) {
            b->velocity.x = -b->velocity.x * b->restitution;
        } else {
            b->velocity.x = 0.0f;
        }
    }
    // Ceiling (top)
    if (b->position.y - radius < top) {
        b->position.y = top + radius;
        if (fabsf(b->velocity.y) > REST_VEL_EPS) {
            b->velocity.y = -b->velocity.y * b->restitution;
        } else {
            b->velocity.y = 0.0f;
        }
    }
    // Floor (bottom)
    if (b->position.y + radius > bottom) {
        b->position.y = bottom - radius;
        if (fabsf(b->velocity.y) > REST_VEL_EPS) {
            b->velocity.y = -b->velocity.y * b->restitution;
        } else {
            b->velocity.y = 0.0f;
        }
    }
}

// Resolve rotated rectangle vs world boundaries (OBB vs planes)
// Strategy: Compute all 4 rotated corners, check each against boundaries,
// find worst penetration, then apply impulse-based collision response.
// This correctly handles rotated rectangles by using b->angle.
static void resolve_rect_vs_bounds(Body *b, float left, float top, float right, float bottom) {
    float half_w = b->shape.rect.width * 0.5f;
    float half_h = b->shape.rect.height * 0.5f;
    
    // Compute the 4 corners of the rotated rectangle (OBB)
    float cos_angle = cosf(b->angle);
    float sin_angle = sinf(b->angle);
    
    // Local corner offsets (relative to center)
    Vec2 local_corners[4] = {
        vec2(-half_w, -half_h),  // Top-left
        vec2( half_w, -half_h),  // Top-right
        vec2( half_w,  half_h),  // Bottom-right
        vec2(-half_w,  half_h)   // Bottom-left
    };
    
    // Transform corners to world space
    Vec2 world_corners[4];
    for (int i = 0; i < 4; i++) {
        float wx = local_corners[i].x * cos_angle - local_corners[i].y * sin_angle;
        float wy = local_corners[i].x * sin_angle + local_corners[i].y * cos_angle;
        world_corners[i] = vec2_add(vec2(wx, wy), b->position);
    }
    
    // Check each corner against boundaries and resolve
    // Track the worst penetration for each boundary
    float max_penetration = 0.0f;
    Vec2 correction = VEC2_ZERO;
    Vec2 collision_normal = VEC2_ZERO;
    Vec2 contact_point = VEC2_ZERO;
    int had_collision = 0;
    
    for (int i = 0; i < 4; i++) {
        Vec2 corner = world_corners[i];
        
        // Left wall
        if (corner.x < left) {
            float penetration = left - corner.x;
            if (penetration > max_penetration) {
                max_penetration = penetration;
                correction = vec2(penetration, 0.0f);
                collision_normal = vec2(1.0f, 0.0f);  // Normal points right (into rect)
                contact_point = vec2(left, corner.y);
                had_collision = 1;
            }
        }
        // Right wall
        if (corner.x > right) {
            float penetration = corner.x - right;
            if (penetration > max_penetration) {
                max_penetration = penetration;
                correction = vec2(-penetration, 0.0f);
                collision_normal = vec2(-1.0f, 0.0f);  // Normal points left (into rect)
                contact_point = vec2(right, corner.y);
                had_collision = 1;
            }
        }
        // Ceiling (top)
        if (corner.y < top) {
            float penetration = top - corner.y;
            if (penetration > max_penetration) {
                max_penetration = penetration;
                correction = vec2(0.0f, penetration);
                collision_normal = vec2(0.0f, 1.0f);  // Normal points down (into rect)
                contact_point = vec2(corner.x, top);
                had_collision = 1;
            }
        }
        // Floor (bottom)
        if (corner.y > bottom) {
            float penetration = corner.y - bottom;
            if (penetration > max_penetration) {
                max_penetration = penetration;
                correction = vec2(0.0f, -penetration);
                collision_normal = vec2(0.0f, -1.0f);  // Normal points up (into rect)
                contact_point = vec2(corner.x, bottom);
                had_collision = 1;
            }
        }
    }
    
    // Apply collision response if we had a boundary collision
    if (had_collision) {
        // Apply positional correction
        b->position = vec2_add(b->position, correction);
        
        // Calculate velocity at contact point
        Vec2 r = vec2_sub(contact_point, b->position);
        Vec2 point_velocity = vec2_add(b->velocity, vec2_scale(vec2_perp(r), b->angular_velocity));
        
        // Velocity component along the collision normal
        float vel_along_normal = vec2_dot(point_velocity, collision_normal);
        
        // Resting contact threshold to prevent jitter
        const float REST_VEL_EPS = 0.05f * PIXELS_PER_METER;  // 5.0 pixels/sec (0.05 m/s)
        
        // Only resolve if moving into the boundary (not resting or separating)
        if (vel_along_normal < -REST_VEL_EPS) {
            // Calculate impulse (treating boundary as infinite mass)
            // For boundary collision, the effective mass calculation simplifies
            float r_cross_n = vec2_cross(r, collision_normal);
            float inv_mass_sum = b->inv_mass + r_cross_n * r_cross_n * b->inv_inertia;
            
            if (inv_mass_sum > 1e-8f) {
                float j = -(1.0f + b->restitution) * vel_along_normal / inv_mass_sum;
                Vec2 impulse = vec2_scale(collision_normal, j);
                
                // Apply impulse to velocity
                b->velocity = vec2_add(b->velocity, vec2_scale(impulse, b->inv_mass));
                
                // Apply angular impulse
                b->angular_velocity += vec2_cross(r, impulse) * b->inv_inertia;
            }
        }
    }
}

static void resolve_boundary_collisions(World *w) {
    if (!w->bounds_enabled) return;
    
    for (int i = 0; i < w->body_count; i++) {
        Body *b = &w->bodies[i];
        if (body_is_static(b)) continue;
        
        if (b->shape.type == SHAPE_CIRCLE) {
            resolve_circle_vs_bounds(b, w->bound_left, w->bound_top, 
                                     w->bound_right, w->bound_bottom);
        } else if (b->shape.type == SHAPE_RECT) {
            resolve_rect_vs_bounds(b, w->bound_left, w->bound_top,
                                   w->bound_right, w->bound_bottom);
        }
    }
}

// Detect all body-body collisions and store in array.
// Returns the number of collisions detected.
static int detect_all_collisions(World *w, Collision *collisions, int max_collisions) {
    int count = 0;
    
    //TODO: optimize the collision detection algorithm
    for (int i = 0; i < w->body_count && count < max_collisions; i++) {
        for (int j = i + 1; j < w->body_count && count < max_collisions; j++) {
            Body *a = &w->bodies[i];
            Body *b = &w->bodies[j];
            
            Collision col;
            int collided = 0;
            
            if (a->shape.type == SHAPE_CIRCLE && b->shape.type == SHAPE_CIRCLE) {
                // Circle-circle collision
                collided = collision_detect_circles(a, b, &col);
            } 
            else if (a->shape.type == SHAPE_CIRCLE && b->shape.type == SHAPE_RECT) {
                // Circle-rect collision (circle is A, rect is B)
                collided = collision_detect_circle_rect(a, b, &col);
            }
            else if (a->shape.type == SHAPE_RECT && b->shape.type == SHAPE_CIRCLE) {
                // Rect-circle collision: call with swapped order, then negate normal
                collided = collision_detect_circle_rect(b, a, &col);
                if (collided) {
                    col.normal = vec2_negate(col.normal);
                }
            }
            else if (a->shape.type == SHAPE_RECT && b->shape.type == SHAPE_RECT) {
                // Rect-rect collision using SAT
                collided = collision_detect_rects(a, b, &col);
            }
            
            if (collided) {
                col.body_a = i;
                col.body_b = j;
                collisions[count++] = col;
            }
        }
    }
    
    return count;
}

// --- Public API ---

// MAIN PHYSICS STEP FUNCTION 
void world_step(World *w) {
    // Step 1: Integrate velocities and positions (dynamics)
    integrate_bodies(w);
    
    // Step 2: Iterative collision solver
    // Re-detecting each iteration handles cascading collisions
    static Collision collisions[MAX_COLLISIONS];
    
    for (int iter = 0; iter < SOLVER_ITERATIONS; iter++) {
        // Detect all body-body collisions fresh each iteration
        int collision_count = detect_all_collisions(w, collisions, MAX_COLLISIONS);
        
        // Resolve each body-body collision
        for (int i = 0; i < collision_count; i++) {
            Body *a = &w->bodies[collisions[i].body_a];
            Body *b = &w->bodies[collisions[i].body_b];
            collision_resolve(a, b, &collisions[i]);
        }
        
        // Resolve boundaries last - ensures bodies stay inside world
        resolve_boundary_collisions(w);
    }
}

void world_render_debug(World *w, SDL_Renderer *r) {
    for (int i = 0; i < w->body_count; i++) {
        render_body_debug(r, &w->bodies[i], w->debug.show_velocity);
    }

    // Rect-rect contact debug: show contact point, normal, and penetration
    if (w->debug.show_contacts) {
        Collision contacts[MAX_COLLISIONS];
        int n = detect_all_collisions(w, contacts, MAX_COLLISIONS);
        for (int i = 0; i < n; i++) {
            Body *a = &w->bodies[contacts[i].body_a];
            Body *b = &w->bodies[contacts[i].body_b];
            if (a->shape.type == SHAPE_RECT && b->shape.type == SHAPE_RECT) {
                render_contact_debug(r,
                    contacts[i].contact.x, contacts[i].contact.y,
                    contacts[i].normal.x, contacts[i].normal.y,
                    contacts[i].penetration);
            }
        }
    }
}

// --- Deterministic RNG ---

void world_seed(World *w, uint32_t seed) {
    // Ensure seed is never 0 (xorshift32 breaks on 0)
    if (seed == 0) seed = 1;
    
    // Mix the seed using splitmix32 to avoid poor distribution with small seeds
    // This ensures that consecutive seeds (1,2,3...) produce well-distributed RNG states
    uint32_t z = seed + 0x9e3779b9;  // Golden ratio constant
    z = (z ^ (z >> 16)) * 0x85ebca6b;
    z = (z ^ (z >> 13)) * 0xc2b2ae35;
    z = z ^ (z >> 16);
    
    w->rng_state = z;
}

uint32_t world_rand(World *w) {
    // xorshift32 algorithm - simple, fast, deterministic
    uint32_t x = w->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    w->rng_state = x;
    return x;
}

float world_randf(World *w) {
    // Generate [0, 1) by dividing by max uint32 value
    return (float)world_rand(w) / (float)UINT32_MAX;
}

// --- Spawn Helpers ---

int world_spawn_grid(World *w, int rows, int cols, Vec2 origin, float spacing,
                     float radius, float mass, float restitution) {
    int added = 0;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Vec2 pos = vec2(
                origin.x + col * spacing,
                origin.y + row * spacing
            );
            
            Body b = body_create_circle(pos, radius, mass, restitution);
            // Give each body a slightly different color based on position
            b.color = (SDL_Color){
                (Uint8)(100 + (col * 30) % 156),
                (Uint8)(100 + (row * 40) % 156),
                (Uint8)(200),
                255
            };
            
            if (world_add_body(w, b) >= 0) {
                added++;
            }
        }
    }
    
    return added;
}

int world_spawn_random(World *w, int count, float x_min, float y_min,
                       float x_max, float y_max, float min_radius, float max_radius,
                       float min_restitution, float max_restitution) {
    int added = 0;
    
    for (int i = 0; i < count; i++) {
        // Random position within bounds (deterministic)
        float x = x_min + world_randf(w) * (x_max - x_min);
        float y = y_min + world_randf(w) * (y_max - y_min);
        
        // Random radius (deterministic)
        float radius = min_radius + world_randf(w) * (max_radius - min_radius);
        
        // Random restitution (bounciness, deterministic)
        float restitution = min_restitution + world_randf(w) * (max_restitution - min_restitution);
        
        // Random color (deterministic)
        SDL_Color color = {
            (Uint8)(world_rand(w) % 156 + 100),
            (Uint8)(world_rand(w) % 156 + 100),
            (Uint8)(world_rand(w) % 156 + 100),
            255
        };
        
        Body b = body_create_circle(vec2(x, y), radius, 1.0f, restitution);
        b.color = color;
        
        if (world_add_body(w, b) >= 0) {
            added++;
        }
    }
    
    return added;
}
