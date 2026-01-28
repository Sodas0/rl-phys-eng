#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

    // Seed random for spawn_random
    srand((unsigned)time(NULL));

    // === Initialize physics world ===
    World world;
    world_init(&world, vec2(0, 200.0f), 1.0f / 60.0f);  // Stronger gravity for fun
    world_set_bounds(&world, 0.0f, 0.0f, 800.0f, 600.0f);

    // === Add bodies using body_default() ===
    // Pattern: create with defaults, modify what you need, then add
    Body b;

    // Big red ball
    b = body_default(vec2(400, 100), 60.0f);
    b.color = (SDL_Color){255, 100, 100, 255};
    world_add_body(&world, b);

    // Green ball moving right
    b = body_default(vec2(200, 50), 30.0f);
    b.velocity = vec2(100, 0);
    b.color = (SDL_Color){100, 255, 100, 255};
    world_add_body(&world, b);

    // Bulk spawn for stress testing
    world_spawn_random(&world, 8,
        50, 50,      // min x, y
        750, 300,    // max x, y
        20, 50,      // radius range
        0.5f, 1.0f); // restitution range

    printf("Spawned %d bodies\n", world.body_count);
    

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
