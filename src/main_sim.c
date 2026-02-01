#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include "simulator.h"
#include "env.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define SIM_DT (1.0f / 240.0f)  // 240 Hz fixed physics timestep

int main(int argc, char *argv[]) {
    // Parse command line arguments for headless mode
    int headless = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0 || strcmp(argv[i], "-h") == 0) {
            headless = 1;
            printf("Running in headless mode (no rendering)\n");
            break;
        }
    }

    // SDL window and renderer (NULL in headless mode)
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    // Only initialize SDL and create window/renderer if not headless
    if (!headless) {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("2D phys-eng - Env API Test",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }

    // Create simulator
    Simulator* sim = sim_create("scenes/fulcrum.json", 12345, SIM_DT);
    if (!sim) {
        fprintf(stderr, "Failed to create simulator\n");
        if (!headless) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
        return 1;
    }

    // Configure debug visualization (only matters in non-headless mode)
    if (!headless) {
        World* world = sim_get_world(sim);
        world->debug.show_velocity = 1;
        world->debug.show_contacts = 1;
    }
    
    // Create environment wrapping the simulator
    Env* env = env_create(sim);
    if (!env) {
        fprintf(stderr, "Failed to create environment\n");
        sim_destroy(sim);
        if (!headless) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
        return 1;
    }
    
    // Enable rendering only if not headless
    env_set_render_enabled(env, !headless);
    
    // Reset environment to get initial observation
    StepResult result = env_reset(env);
    printf("Initial observation: ball pos = (%.2f, %.2f)\n", 
           result.obs.data[0], result.obs.data[1]);
    
    // Timing variables (used differently for headless vs non-headless)
    Uint64 last_time = headless ? 0 : SDL_GetPerformanceCounter();
    float accumulator = 0.0f;
    
    // Debug stats tracking
    int frame_count = 0;
    long long step_count = 0;  // Use long long to avoid overflow for large simulations
    float debug_timer = 0.0f;
    const float DEBUG_PRINT_INTERVAL = 1.0f;
    float total_reward = 0.0f;  // Track cumulative reward  
    uint32_t episode_count = 0;  // Track number of episodes for randomization  

    int running = 1;
    SDL_Event event;
    
    while (running) {
        float frame_time;
        
        if (headless) {
            // In headless mode, run fixed timesteps as fast as possible
            frame_time = SIM_DT;
            accumulator = SIM_DT;
            
            // Run for a limited number of steps (e.g., 10 million steps = ~11.5 hours of simulated time)
            if (step_count >= 10000000LL) {  // Use LL suffix for long long literal
                running = 0;
            }
        } else {
            // Calculate elapsed time since last frame
            Uint64 current_time = SDL_GetPerformanceCounter();
            frame_time = (float)(current_time - last_time) / SDL_GetPerformanceFrequency();
            last_time = current_time;
            
            // Cap frame time to prevent spiral of death
            if (frame_time > 0.25f) frame_time = 0.25f;
            
            accumulator += frame_time;
            
            // Handle input (only in non-headless mode)
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = 0;
                if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R) {
                    // Reset via environment API with new seed for randomization
                    sim->seed = episode_count++;
                    result = env_reset(env);
                    total_reward = 0.0f;
                    printf("Environment reset (seed=%u). Ball pos = (%.2f, %.2f)\n",
                           sim->seed, result.obs.data[0], result.obs.data[1]);
                }
            }
        }

        // Keyboard control (only in non-headless mode)
        float action_value = 0.0f;
        if (!headless) {
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if (keys[SDL_SCANCODE_A]) action_value -= 1.0f;  // Left: negative command
            if (keys[SDL_SCANCODE_D]) action_value += 1.0f;  // Right: positive command
        }

        // Run physics steps as needed to catch up to real time
        while (accumulator >= SIM_DT) {
            // Create action and step via environment API
            Action action = { .torque = action_value };
            result = env_step(env, action);
            total_reward += result.reward;
            
            // Print per-step information with terminated/truncated flags
            printf("Step %lld | Obs: [%.2f, %.2f, %.2f, %.2f] | Reward: %+.4f | Term: %d | Trunc: %d | Action: %+.3f\n",
                   step_count,
                   result.obs.data[0], result.obs.data[1], 
                   result.obs.data[2], result.obs.data[3],
                   result.reward, result.terminated, result.truncated, action_value);
            
            // Handle episode termination (failure or time limit)
            if (result.terminated || result.truncated) {
                const char* reason = result.terminated ? "TERMINATED (ball fell off)" : "TRUNCATED (time limit)";
                printf("Episode ended: %s | Total reward: %+.2f | Steps: %lld\n", 
                       reason, total_reward, step_count);
                
                // Auto-reset for continuous operation with new seed for randomization
                sim->seed = episode_count++;
                result = env_reset(env);
                total_reward = 0.0f;
                printf("Environment auto-reset (seed=%u). Starting new episode.\n", sim->seed);
            }
            
            accumulator -= SIM_DT;
            step_count++;
        }
        
        // Debug output: print actuator stats and env info periodically
        debug_timer += frame_time;
        frame_count++;
        if (debug_timer >= DEBUG_PRINT_INTERVAL) {
            float fps = frame_count / debug_timer;
            const char* mode = headless ? "Headless" : "Rendering";
            printf("[%s] FPS: %.1f | Steps: %lld | Action: %+.3f | Ball pos: (%.1f, %.1f) | Reward: %+.3f | Total reward: %+.2f\n",
                   mode, fps, step_count,
                   action_value,
                   result.obs.data[0], result.obs.data[1],
                   result.reward, total_reward);
            debug_timer = 0.0f;
            frame_count = 0;
        }

        // Render via environment API (no-op in headless mode)
        if (!headless) {
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);
            env_render(env, renderer);
            SDL_RenderPresent(renderer);
        }
    }

    // Cleanup via environment API (which destroys the simulator)
    env_destroy(env);
    
    // Cleanup SDL resources (only if not headless)
    if (!headless) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    
    printf("Simulation complete. Total steps: %lld\n", step_count);
    return 0;
}
