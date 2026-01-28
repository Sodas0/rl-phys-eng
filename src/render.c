#include "render.h"
#include "body.h"
#include <math.h>

void render_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);

        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void render_circle_filled(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(r, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(r, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(r, cx - y, cy - x, cx + y, cy - x);

        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void render_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

void render_point(SDL_Renderer *r, int x, int y, int size, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x - size / 2, y - size / 2, size, size};
    SDL_RenderFillRect(r, &rect);
}

void render_arrow(SDL_Renderer *r, int x, int y, float vx, float vy, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int ex = x + (int)vx;
    int ey = y + (int)vy;

    // Main line
    SDL_RenderDrawLine(r, x, y, ex, ey);

    // Arrowhead (two small lines)
    float len = sqrtf(vx * vx + vy * vy);
    if (len < 1.0f) return;

    float nx = vx / len;
    float ny = vy / len;
    float px = -ny;
    float py = nx;
    float head = 8.0f;

    int ax = ex - (int)(nx * head + px * head * 0.5f);
    int ay = ey - (int)(ny * head + py * head * 0.5f);
    int bx = ex - (int)(nx * head - px * head * 0.5f);
    int by = ey - (int)(ny * head - py * head * 0.5f);

    SDL_RenderDrawLine(r, ex, ey, ax, ay);
    SDL_RenderDrawLine(r, ex, ey, bx, by);
}

void render_body(SDL_Renderer *r, const Body *b) {
    int cx = (int)b->position.x;
    int cy = (int)b->position.y;
    int radius = (int)b->radius;

    // Filled circle with body color
    render_circle_filled(r, cx, cy, radius, b->color);

    // White outline for visibility
    SDL_Color outline = {255, 255, 255, 255};
    render_circle(r, cx, cy, radius, outline);
}

void render_body_debug(SDL_Renderer *r, const Body *b) {
    // Draw the body itself
    render_body(r, b);

    // Draw velocity vector for dynamic bodies
    if (!body_is_static(b)) {
        float vel_scale = 20.0f;  // Scale velocity for visibility
        SDL_Color yellow = {255, 255, 0, 255};
        render_arrow(r, (int)b->position.x, (int)b->position.y,
                     b->velocity.x * vel_scale, b->velocity.y * vel_scale, yellow);
    }

    // Draw center point
    SDL_Color center_color = {255, 255, 255, 255};
    render_point(r, (int)b->position.x, (int)b->position.y, 4, center_color);
}
