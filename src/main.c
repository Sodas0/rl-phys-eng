#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "world.h"
#include "scene.h"

// Window dimensions [im using 1920x1080 cause 1440p 240hz oled makes sim look amazing]
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

//TODO: make main function more concise
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("2D phys-eng",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);

    // Seed random for spawn_random
    srand((unsigned)time(NULL));

    // === Load scene ===
    World world;
    if (scene_load("scenes/gravity_test.json", &world) != 0) {
        fprintf(stderr, "Failed to load scene\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Configure debug visualization
    world.debug.show_velocity = 1;   // See velocity vectors
    world.debug.show_contacts = 1;   // See rect-rect contact points, normals, penetration
    
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
