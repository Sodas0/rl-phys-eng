#ifndef RENDER_H
#define RENDER_H

#include <SDL.h>

void render_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color);
void render_circle_filled(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color);
void render_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color color);
void render_point(SDL_Renderer *r, int x, int y, int size, SDL_Color color);
void render_arrow(SDL_Renderer *r, int x, int y, float vx, float vy, SDL_Color color);

#endif
