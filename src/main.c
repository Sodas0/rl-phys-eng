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
    // world_init(&world, vec2(0, 98.1f), 1.0f / 60.0f);
    world_init(&world, vec2(0, .981f), 1.0f / 60.0f);

    // === Add test bodies ===
    Body b;

    // Dynamic body 1 - Red, moving right and down
    b = body_create(vec2(150, 200), 40.0f, 1.0f, 0.8f);
    b.color = (SDL_Color){255, 100, 100, 255};
    b.velocity = vec2(50.0f, 0.0f);
    world_add_body(&world, b);

    // Dynamic body 2 - Blue, moving left
    b = body_create(vec2(400, 150), 30.0f, 2.0f, 0.6f);
    b.color = (SDL_Color){100, 100, 255, 255};
    b.velocity = vec2(-30.0f, 0.0f);
    world_add_body(&world, b);

    // Dynamic body 3 - Green, stationary
    b = body_create(vec2(600, 300), 25.0f, 0.5f, 0.9f);
    b.color = (SDL_Color){100, 255, 100, 255};
    world_add_body(&world, b);

    b = body_create(vec2(50,50), 10.0f, 1.0f, 1.0f);
    b.color = (SDL_Color){255, 255, 255, 255};
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
