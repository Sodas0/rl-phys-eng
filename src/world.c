#include "world.h"
#include "render.h"
#include "collision.h"
#include <stdio.h>

void world_init(World *w, Vec2 gravity, float dt) {
    w->body_count = 0;
    w->gravity = gravity;
    w->dt = dt;
    w->bounds_enabled = 0;
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
    }
}

static void resolve_boundary_collisions(World *w) {
    if (!w->bounds_enabled) return;
    
    for (int i = 0; i < w->body_count; i++) {
        Body *b = &w->bodies[i];
        if (body_is_static(b)) continue;
        
        // Left wall
        if (b->position.x - b->radius < w->bound_left) {
            b->position.x = w->bound_left + b->radius;
            b->velocity.x = -b->velocity.x * b->restitution;
        }
        // Right wall
        if (b->position.x + b->radius > w->bound_right) {
            b->position.x = w->bound_right - b->radius;
            b->velocity.x = -b->velocity.x * b->restitution;
        }
        // Ceiling (top)
        if (b->position.y - b->radius < w->bound_top) {
            b->position.y = w->bound_top + b->radius;
            b->velocity.y = -b->velocity.y * b->restitution;
        }
        // Floor (bottom)
        if (b->position.y + b->radius > w->bound_bottom) {
            b->position.y = w->bound_bottom - b->radius;
            b->velocity.y = -b->velocity.y * b->restitution;
        }
    }
}

static void detect_body_collisions(World *w) {
    for (int i = 0; i < w->body_count; i++) {
        for (int j = i + 1; j < w->body_count; j++) {
            Collision col;
            if (collision_detect_circles(&w->bodies[i], &w->bodies[j], &col)) {
                col.body_a = i;
                col.body_b = j;
                printf("collision detected: body %d and body %d\n", i, j);
            }
        }
    }
}

// --- Public API ---

void world_step(World *w) {
    integrate_bodies(w);
    resolve_boundary_collisions(w);
    detect_body_collisions(w);
}

void world_render_debug(World *w, SDL_Renderer *r) {
    for (int i = 0; i < w->body_count; i++) {
        render_body_debug(r, &w->bodies[i]);
    }
}
