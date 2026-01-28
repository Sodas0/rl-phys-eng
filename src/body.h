#ifndef BODY_H
#define BODY_H

#include "vec2.h"
#include <SDL.h>

typedef struct Body {
    Vec2 position;       // Center position
    Vec2 velocity;       // Linear velocity
    float radius;        // Circle radius
    float mass;          // Mass (kg)
    float inv_mass;      // 1/mass (0 = static/immovable)
    float restitution;   // Bounciness [0-1]
    SDL_Color color;     // For rendering
} Body;

// Create a dynamic body with given properties (full control)
Body body_create(Vec2 pos, float radius, float mass, float restitution);

// Create a body with sensible defaults (mass=1, restitution=0.8, white color)
// This is the preferred constructor for most use cases
Body body_default(Vec2 pos, float radius);

// Create a static (immovable) body
Body body_create_static(Vec2 pos, float radius);

// Make an existing body static (sets inv_mass = 0)
void body_set_static(Body *b);

// Check if body is static (inv_mass == 0)
int body_is_static(const Body *b);

#endif // BODY_H
