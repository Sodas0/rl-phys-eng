#ifndef RENDER_H
#define RENDER_H

#include <SDL.h>

// Forward declaration to avoid circular include
typedef struct Body Body;

// Primitive drawing functions
void render_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color);
void render_circle_filled(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color);
void render_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color color);
void render_point(SDL_Renderer *r, int x, int y, int size, SDL_Color color);
void render_arrow(SDL_Renderer *r, int x, int y, float vx, float vy, SDL_Color color);

// Body rendering functions
void render_body(SDL_Renderer *r, const Body *b);
void render_body_debug(SDL_Renderer *r, const Body *b);  // Also draws velocity vector

#endif
