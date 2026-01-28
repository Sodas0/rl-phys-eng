#ifndef COLLISION_H
#define COLLISION_H

#include "body.h"

typedef struct {
    int body_a;           // Index of first body
    int body_b;           // Index of second body
    Vec2 normal;          // Collision normal (points from A to B)
    float penetration;    // Overlap depth
    Vec2 contact;         // Contact point (midpoint on collision axis)
} Collision;

// Returns 1 if colliding, 0 otherwise. Fills `out` with collision data.
int collision_detect_circles(const Body *a, const Body *b, Collision *out);

#endif // COLLISION_H
