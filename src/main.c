#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "world.h"
#include "scene.h"
#include "vec2.h"

// Window dimensions [im using 1920x1080 cause 1440p 240hz oled makes sim look amazing]
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Actuator control --simple beam for now
#define BEAM_ANGLE_SPEED  0.005f
#define BEAM_ANGLE_MAX    0.5f

static void apply_actuator_pose(World *world, float angle) {
    if (world->actuator_body_index < 0) return;
    Body *beam = world_get_body(world, world->actuator_body_index);
    if (!beam || beam->shape.type != SHAPE_RECT) return;

    Body *base = world_get_body(world, 0);
    int use_fulcrum = (base && base != beam && base->shape.type == SHAPE_RECT);

    if (use_fulcrum) {
        float h_base = base->shape.rect.height;
        float h_beam = beam->shape.rect.height;
        float pivot_y = base->position.y - h_base * 0.5f;
        float beam_y = pivot_y - h_beam * 0.5f;
        beam->position.x = base->position.x;
        beam->position.y = beam_y;
    } else {
        beam->position = world->actuator_pivot;
    }
    beam->angle = angle;
    beam->velocity = VEC2_ZERO;
    beam->angular_velocity = 0.0f;
}

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
    if (scene_load("scenes/fulcrum.json", &world) != 0) {
        fprintf(stderr, "Failed to load scene\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    float beam_angle = 0.0f;

    // Configure debug visualization
    world.debug.show_velocity = 1;   // See velocity vectors
    world.debug.show_contacts = 1;   // See rect-rect contact points, normals, penetration

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R) {
                if (scene_load("scenes/fulcrum.json", &world) == 0) {
                    beam_angle = 0.0f;
                    world.debug.show_velocity = 0;
                    world.debug.show_contacts = 0;
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (world.actuator_body_index >= 0) {
            if (keys[SDL_SCANCODE_A]) beam_angle -= BEAM_ANGLE_SPEED;
            if (keys[SDL_SCANCODE_D]) beam_angle += BEAM_ANGLE_SPEED;
            if (beam_angle > BEAM_ANGLE_MAX)  beam_angle = BEAM_ANGLE_MAX;
            if (beam_angle < -BEAM_ANGLE_MAX) beam_angle = -BEAM_ANGLE_MAX;
            apply_actuator_pose(&world, beam_angle);
        }

        // Physics update
        world_step(&world);

        if (world.actuator_body_index >= 0)
            apply_actuator_pose(&world, beam_angle);

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
