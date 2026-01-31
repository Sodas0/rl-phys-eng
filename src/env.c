#include "env.h"
#include "simulator.h"
#include <stdlib.h>
#include <string.h>

// Internal Env structure
struct Env {
    Simulator* sim;           // Owned simulator instance
    int ball_body_index;      // Hardcoded to 1 (ball is body index 1)
    int render_enabled;       // Flag: 1 = rendering enabled, 0 = disabled (default)
};

Env* env_create(Simulator* sim) {
    if (!sim) {
        return NULL;
    }
    
    // Allocate environment struct
    Env* env = (Env*)malloc(sizeof(Env));
    if (!env) {
        return NULL;
    }
    
    // Take ownership of the simulator
    env->sim = sim;
    
    // Set ball body index (hardcoded as specified)
    env->ball_body_index = 1;
    
    // Rendering disabled by default (headless mode)
    env->render_enabled = 0;
    
    return env;
}

void env_destroy(Env* env) {
    if (env) {
        // Destroy owned simulator
        sim_destroy(env->sim);
        
        // Free environment struct
        free(env);
    }
}

void env_render(Env* env, SDL_Renderer* renderer) {
    // No-op if env is NULL, renderer is NULL, or rendering is disabled
    if (!env || !renderer || !env->render_enabled) {
        return;
    }
    
    // Thin passthrough to world rendering
    World* world = sim_get_world(env->sim);
    if (world) {
        world_render_debug(world, renderer);
    }
}

void env_set_render_enabled(Env* env, int enabled) {
    if (env) {
        env->render_enabled = enabled ? 1 : 0;
    }
}
