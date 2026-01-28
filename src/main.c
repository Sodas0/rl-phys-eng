#include <SDL.h>
#include <stdio.h>
#include "world.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("2D phys-eng",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);

    // === Initialize physics world ===
    // Gravity: 98.1 pixels/s^2 downward (scaled for screen coordinates)
    // Timestep: 1/60 second for 60 FPS
    World world;
    world_init(&world, vec2(0, 98.1f), 1.0f / 60.0f);
    
    // Set world boundaries to match window size (left, top, right, bottom)
    world_set_bounds(&world, 0.0f, 0.0f, 800.0f, 600.0f);

    // === Add test bodies ===
    Body b;

    // Row 1 - Top row, varying sizes
    b = body_create(vec2(100, 50), 15.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 100, 100, 255};  // Red
    b.velocity = vec2(20.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(200, 40), 25.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 180, 100, 255};  // Orange
    b.velocity = vec2(-15.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(320, 60), 35.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 255, 100, 255};  // Yellow
    b.velocity = vec2(10.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(450, 45), 20.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){100, 255, 100, 255};  // Green
    b.velocity = vec2(-25.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(550, 55), 30.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){100, 255, 255, 255};  // Cyan
    b.velocity = vec2(-10.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(680, 50), 18.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){100, 100, 255, 255};  // Blue
    b.velocity = vec2(-20.0f, 0.0f);
    world_add_body(&world, b);

    // Row 2 - Middle row, staggered positions
    b = body_create(vec2(150, 150), 40.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){180, 100, 255, 255};  // Purple
    b.velocity = vec2(15.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(280, 140), 12.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 100, 180, 255};  // Pink
    b.velocity = vec2(25.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(400, 160), 50.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 255, 255, 255};  // White (large)
    b.velocity = vec2(-5.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(550, 130), 22.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){200, 200, 100, 255};  // Olive
    b.velocity = vec2(-15.0f, 0.0f);
    world_add_body(&world, b);

    // Row 3 - Lower bodies that others will fall into
    b = body_create(vec2(200, 280), 45.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){150, 150, 150, 255};  // Gray
    b.velocity = vec2(10.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(500, 300), 38.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){100, 180, 180, 255};  // Teal
    b.velocity = vec2(-20.0f, 0.0f);
    world_add_body(&world, b);

    b = body_create(vec2(650, 250), 28.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){180, 130, 100, 255};  // Brown
    b.velocity = vec2(-10.0f, 0.0f);
    world_add_body(&world, b);

    // A few tiny ones scattered around
    b = body_create(vec2(350, 100), 8.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 50, 50, 255};  // Bright red (tiny)
    b.velocity = vec2(30.0f, 0.0f);
    world_add_body(&world, b);
    

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }

        // Physics update
        world_step(&world);

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        world_render_debug(&world, renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
