#include "env.h"
#include "simulator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal Env structure
struct Env {
    Simulator* sim;           // Owned simulator instance
    int render_enabled;       // Flag: 1 = rendering enabled, 0 = disabled (default)
    int step_count;           // Current episode step counter
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
    
    // Rendering enabled by default if simulator is not headless
    // This allows env_render() to work without explicit configuration
    env->render_enabled = !sim->headless;
    
    // Initialize episode step counter
    env->step_count = 0;
    
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

void env_render(Env* env) {
    // No-op if env is NULL or rendering is disabled
    if (!env || !env->render_enabled) {
        return;
    }
    
    // Thin passthrough to simulator rendering
    sim_render(env->sim);
}

void env_set_render_enabled(Env* env, int enabled) {
    if (env) {
        env->render_enabled = enabled ? 1 : 0;
    }
}

StepResult env_reset(Env* env) {
    StepResult result = {0};
    
    if (!env) {
        return result;
    }
    
    // Reset the simulator to initial state (deterministic: reloads scene, resets RNG)
    sim_reset(env->sim);
    
    // Reset episode step counter
    env->step_count = 0;
    
    // Get initial observation via simulator accessor
    sim_get_observation(env->sim, result.obs.data, OBS_DIM);
    
    // Initial reward is 0
    result.reward = 0.0f;
    
    // Episode just started, not terminated or truncated
    result.terminated = 0;
    result.truncated = 0;
    
    return result;
}

StepResult env_step(Env* env, Action action) {
    StepResult result = {0};
    
    if (!env) {
        return result;
    }
    
    // Step the simulator with the action
    sim_step(env->sim, action.torque);
    
    // Increment step counter
    env->step_count++;
    
    // Get observation via simulator accessor (single authority on state semantics)
    sim_get_observation(env->sim, result.obs.data, OBS_DIM);
    
    // Extract state from observation
    // obs[0]: beam angle, obs[2]: ball position along beam
    float beam_angle = result.obs.data[0];
    float x_along_beam = result.obs.data[2];
    
    // --- Termination Condition 1: Ball hit the floor (failure) ---
    // Check if ball has fallen to the bottom boundary
    World* world = sim_get_world(env->sim);
    if (world) {
        // Get ball body (hardcoded convention: ball is body 1)
        const int BALL_BODY_INDEX = 1;
        Body* ball = world_get_body(world, BALL_BODY_INDEX);
        
        if (ball) {
            // Get ball radius for proper collision detection
            float ball_radius = (ball->shape.type == SHAPE_CIRCLE) ? 
                ball->shape.circle.radius : 0.0f;
            
            // Ball hits floor when its bottom edge reaches or passes the floor
            // Add small tolerance (1 pixel) to avoid numerical issues
            if (ball->position.y + ball_radius >= world->bound_bottom - 1.0f) {
                result.terminated = 1;
                result.truncated = 0;
                
                // Terminal penalty for failure
                result.reward = -1.0f;
                
                return result;
            }
        }
    }
    
    // --- Termination Condition 2: Time limit reached (truncation) ---
    // Episode has run for maximum allowed steps
    if (env->step_count >= MAX_EPISODE_STEPS) {
        result.terminated = 0;
        result.truncated = 1;
        
        // No terminal penalty for time limit (neutral truncation)
        // Reward is computed from state below
    }
    
    // --- Reward Shaping (continuous, not terminal) ---
    // Reward encourages: small beam angle and ball near beam center
    // This applies to all non-failure states (including truncation)
    if (!result.terminated) {
        result.reward = -fabsf(x_along_beam) * 0.01f - fabsf(beam_angle) * 0.1f;
    }
    
    return result;
}


