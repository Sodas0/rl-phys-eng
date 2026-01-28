#ifndef WORLD_H
#define WORLD_H

#include "body.h"
#include <SDL.h>

#define MAX_BODIES 256

typedef struct {
    Body bodies[MAX_BODIES];
    int body_count;
    Vec2 gravity;
    float dt;  // Fixed timestep
    
    // World boundaries (for constraining bodies)
    float bound_left;
    float bound_right;
    float bound_top;
    float bound_bottom;
    int bounds_enabled;
} World;

// Initialize world with gravity vector and fixed timestep
void world_init(World *w, Vec2 gravity, float dt);

// Set world boundaries (left, top, right, bottom)
void world_set_bounds(World *w, float left, float top, float right, float bottom);

// Add a body to the world. Returns body index, or -1 if full
int world_add_body(World *w, Body b);

// Get pointer to body at index (NULL if invalid)
Body* world_get_body(World *w, int index);

// Advance simulation by one timestep (integrates velocities and positions)
void world_step(World *w);

// Render all bodies with debug info (velocity vectors, centers)
void world_render_debug(World *w, SDL_Renderer *r);

#endif // WORLD_H
