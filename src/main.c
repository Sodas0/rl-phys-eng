#include <SDL.h>
#include "render.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("2D phys-eng",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Test shapes
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {255, 100, 100, 255};
        SDL_Color green = {100, 255, 100, 255};
        SDL_Color yellow = {255, 255, 100, 255};
        render_circle(renderer, 300, 300, 50, white);
        render_circle_filled(renderer, 500, 300, 50, red);
        render_line(renderer, 100, 100, 700, 500, green);
        render_point(renderer, 400, 450, 8, yellow);
        render_arrow(renderer, 500, 300, 80, -60, white);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
