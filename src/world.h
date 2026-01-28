#ifndef WORLD_H
#define WORLD_H

#include "body.h"
#include <SDL.h>

#define MAX_BODIES 256
#define MAX_COLLISIONS 512    // Worst case: n*(n-1)/2 for 256 bodies
#define SOLVER_ITERATIONS 6   // Tune: 4-8 typical for stable stacking

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

// --- Spawn Helpers (for testing/bulk creation) ---

// Spawn a grid of bodies. Returns number of bodies added.
// Origin is top-left of grid. Spacing is center-to-center distance.
int world_spawn_grid(World *w, int rows, int cols, Vec2 origin, float spacing,
                     float radius, float mass, float restitution);

// Spawn bodies at random positions within bounds. Returns number added.
// Uses simple pseudo-random; call srand() beforehand for variety.
int world_spawn_random(World *w, int count, float x_min, float y_min,
                       float x_max, float y_max, float min_radius, float max_radius,
                       float min_restitution, float max_restitution);

#endif // WORLD_H
