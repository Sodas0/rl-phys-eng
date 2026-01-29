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

void render_rect(SDL_Renderer *r, int cx, int cy, int width, int height, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {cx - width / 2, cy - height / 2, width, height};
    SDL_RenderDrawRect(r, &rect);
}

void render_rect_filled(SDL_Renderer *r, int cx, int cy, int width, int height, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {cx - width / 2, cy - height / 2, width, height};
    SDL_RenderFillRect(r, &rect);
}

void render_rect_rotated(SDL_Renderer *r, float cx, float cy, float width, float height, float angle, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    
    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float c = cosf(angle);
    float s = sinf(angle);
    
    // Four corners in local space: top-left, top-right, bottom-right, bottom-left
    float local[4][2] = {
        {-hw, -hh},
        { hw, -hh},
        { hw,  hh},
        {-hw,  hh}
    };
    
    // Transform to world space
    int screen[4][2];
    for (int i = 0; i < 4; i++) {
        float lx = local[i][0];
        float ly = local[i][1];
        // Rotate then translate
        screen[i][0] = (int)(cx + lx * c - ly * s);
        screen[i][1] = (int)(cy + lx * s + ly * c);
    }
    
    // Draw 4 edges
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        SDL_RenderDrawLine(r, screen[i][0], screen[i][1], screen[next][0], screen[next][1]);
    }
}

void render_rect_rotated_filled(SDL_Renderer *r, float cx, float cy, float width, float height, float angle, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    
    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float c = cosf(angle);
    float s = sinf(angle);
    
    // Four corners in local space: top-left, top-right, bottom-right, bottom-left
    float local[4][2] = {
        {-hw, -hh},
        { hw, -hh},
        { hw,  hh},
        {-hw,  hh}
    };
    
    // Transform to world space
    float corners[4][2];
    int minY = 10000, maxY = -10000;
    for (int i = 0; i < 4; i++) {
        float lx = local[i][0];
        float ly = local[i][1];
        corners[i][0] = cx + lx * c - ly * s;
        corners[i][1] = cy + lx * s + ly * c;
        if ((int)corners[i][1] < minY) minY = (int)corners[i][1];
        if ((int)corners[i][1] > maxY) maxY = (int)corners[i][1];
    }
    
    // Scanline fill for convex quad
    for (int y = minY; y <= maxY; y++) {
        float intersections[4];
        int count = 0;
        
        // Find intersections with all 4 edges
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            float y1 = corners[i][1], y2 = corners[j][1];
            float x1 = corners[i][0], x2 = corners[j][0];
            
            // Check if edge crosses this scanline
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                float t = (y - y1) / (y2 - y1);
                intersections[count++] = x1 + t * (x2 - x1);
            }
        }
        
        if (count >= 2) {
            // Sort intersections (simple bubble for 2-4 elements)
            for (int i = 0; i < count - 1; i++) {
                for (int j = i + 1; j < count; j++) {
                    if (intersections[i] > intersections[j]) {
                        float tmp = intersections[i];
                        intersections[i] = intersections[j];
                        intersections[j] = tmp;
                    }
                }
            }
            SDL_RenderDrawLine(r, (int)intersections[0], y, (int)intersections[count - 1], y);
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
    float cx = b->position.x;
    float cy = b->position.y;
    SDL_Color outline = {255, 255, 255, 255};

    if (b->shape.type == SHAPE_CIRCLE) {
        int radius = (int)b->shape.circle.radius;
        
        // Filled circle with body color
        render_circle_filled(r, (int)cx, (int)cy, radius, b->color);
        
        // White outline for visibility
        render_circle(r, (int)cx, (int)cy, radius, outline);
    } else if (b->shape.type == SHAPE_RECT) {
        float width = b->shape.rect.width;
        float height = b->shape.rect.height;
        float angle = b->angle;
        
        // Filled rectangle with body color (rotated)
        render_rect_rotated_filled(r, cx, cy, width, height, angle, b->color);
        
        // White outline for visibility (rotated)
        render_rect_rotated(r, cx, cy, width, height, angle, outline);
    }
}

void render_body_debug(SDL_Renderer *r, const Body *b, int show_velocity) {
    // Draw the body itself
    render_body(r, b);
    
    if (show_velocity && !body_is_static(b)) {
        float vel_scale = 20.0f;
        SDL_Color yellow = {255, 255, 0, 255};
        render_arrow(r, (int)b->position.x, (int)b->position.y,
                     b->velocity.x * vel_scale, b->velocity.y * vel_scale, yellow);
    }

    // Draw center point
    SDL_Color center_color = {255, 255, 255, 255};
    render_point(r, (int)b->position.x, (int)b->position.y, 4, center_color);
}
