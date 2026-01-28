#include "body.h"

Body body_create(Vec2 pos, float radius, float mass, float restitution) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.radius = radius;
    b.mass = mass;
    b.inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    b.restitution = restitution;
    b.color = (SDL_Color){255, 255, 255, 255};  // Default white
    return b;
}

Body body_create_static(Vec2 pos, float radius) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.radius = radius;
    b.mass = 0.0f;
    b.inv_mass = 0.0f;
    b.restitution = 0.5f;
    b.color = (SDL_Color){100, 100, 100, 255};  // Gray for static
    return b;
}

void body_set_static(Body *b) {
    b->inv_mass = 0.0f;
    b->mass = 0.0f;
}

int body_is_static(const Body *b) {
    return b->inv_mass == 0.0f;
}
