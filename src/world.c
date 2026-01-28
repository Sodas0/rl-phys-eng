#include "world.h"
#include "render.h"

void world_init(World *w, Vec2 gravity, float dt) {
    w->body_count = 0;
    w->gravity = gravity;
    w->dt = dt;
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

void world_step(World *w) {
    for (int i = 0; i < w->body_count; i++) {
        Body *b = &w->bodies[i];
        
        // Skip static bodies (they don't move)
        if (body_is_static(b)) continue;
        
        // Semi-implicit Euler integration:
        // Update velocity first, then use new velocity for position
        b->velocity = vec2_add(b->velocity, vec2_scale(w->gravity, w->dt));
        b->position = vec2_add(b->position, vec2_scale(b->velocity, w->dt));
    }
}

void world_render_debug(World *w, SDL_Renderer *r) {
    for (int i = 0; i < w->body_count; i++) {
        render_body_debug(r, &w->bodies[i]);
    }
}
