#include "world.h"
#include "render.h"
#include "collision.h"
#include <stdlib.h>  // For rand()

void world_init(World *w, Vec2 gravity, float dt) {
    w->body_count = 0;
    w->gravity = gravity;
    w->dt = dt;
    w->bounds_enabled = 0;
    
    // Default debug flags (all off)
    w->debug.show_velocity = 0;
    w->debug.show_contacts = 0;
    w->debug.show_normals = 0;
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
    
    // Left wall
    if (b->position.x - radius < left) {
        b->position.x = left + radius;
        b->velocity.x = -b->velocity.x * b->restitution;
    }
    // Right wall
    if (b->position.x + radius > right) {
        b->position.x = right - radius;
        b->velocity.x = -b->velocity.x * b->restitution;
    }
    // Ceiling (top)
    if (b->position.y - radius < top) {
        b->position.y = top + radius;
        b->velocity.y = -b->velocity.y * b->restitution;
    }
    // Floor (bottom)
    if (b->position.y + radius > bottom) {
        b->position.y = bottom - radius;
        b->velocity.y = -b->velocity.y * b->restitution;
    }
}

static void resolve_rect_vs_bounds(Body *b, float left, float top, float right, float bottom) {
    float half_w = b->shape.rect.width * 0.5f;
    float half_h = b->shape.rect.height * 0.5f;
    
    // Left wall
    if (b->position.x - half_w < left) {
        b->position.x = left + half_w;
        b->velocity.x = -b->velocity.x * b->restitution;
    }
    // Right wall
    if (b->position.x + half_w > right) {
        b->position.x = right - half_w;
        b->velocity.x = -b->velocity.x * b->restitution;
    }
    // Ceiling (top)
    if (b->position.y - half_h < top) {
        b->position.y = top + half_h;
        b->velocity.y = -b->velocity.y * b->restitution;
    }
    // Floor (bottom)
    if (b->position.y + half_h > bottom) {
        b->position.y = bottom - half_h;
        b->velocity.y = -b->velocity.y * b->restitution;
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
            // Rect-rect: not implemented yet
            
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
        // Random position within bounds
        float x = x_min + ((float)rand() / RAND_MAX) * (x_max - x_min);
        float y = y_min + ((float)rand() / RAND_MAX) * (y_max - y_min);
        
        // Random radius
        float radius = min_radius + ((float)rand() / RAND_MAX) * (max_radius - min_radius);
        
        // Random restitution (bounciness)
        float restitution = min_restitution + ((float)rand() / RAND_MAX) * (max_restitution - min_restitution);
        
        // Random color
        SDL_Color color = {
            (Uint8)(rand() % 156 + 100),
            (Uint8)(rand() % 156 + 100),
            (Uint8)(rand() % 156 + 100),
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
