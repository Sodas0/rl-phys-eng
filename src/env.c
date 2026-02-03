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

//TODO: Let API configure the reward and termination conditions.
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
    // obs[0]: beam angle (rad), obs[1]: beam angular velocity (rad/s)
    // obs[2]: ball position along beam (px), obs[3]: ball velocity along beam (px/s)
    float beam_angle = result.obs.data[0];
    float beam_angular_velocity = result.obs.data[1];
    float x_along_beam = result.obs.data[2];
    float vel_along_beam = result.obs.data[3];
    
    // Get beam half-length for early termination threshold
    World* world = sim_get_world(env->sim);
    Body* beam = NULL;
    float beam_half_length = 0.0f;
    
    if (world) {
        beam = world_get_body(world, world->actuator_body_index);
        if (beam && beam->shape.type == SHAPE_RECT) {
            beam_half_length = beam->shape.rect.width * 0.5f;
        }
    }
    
    // --- Termination Condition 1: Ball hit the floor (catastrophic failure) ---
    // Check if ball has fallen to the bottom boundary
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
                
                // Large penalty for catastrophic failure
                result.reward = -10.0f;
                
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
    
    // --- Dense Quadratic Reward Shaping ---
    // Penalize squared deviations from equilibrium (zero position, zero velocities, zero angle)
    // Quadratic penalties provide stronger gradients when far from equilibrium
    
    // Normalize state components for balanced penalty magnitudes
    // Typical ranges: angle +/-0.5 rad, ang_vel +/-2 rad/s, pos +/-500 px, vel +/-500 px/s
    float norm_angle = beam_angle / 0.5f;              // normalize to ±1
    float norm_ang_vel = beam_angular_velocity / 2.0f; // normalize to ±1
    float norm_pos = x_along_beam / 500.0f;            // normalize to ±1
    float norm_vel = vel_along_beam / 500.0f;          // normalize to ±1
    
    // Compute quadratic penalties (squared deviations)
    float angle_penalty = norm_angle * norm_angle;
    float ang_vel_penalty = norm_ang_vel * norm_ang_vel;
    float pos_penalty = norm_pos * norm_pos;
    float vel_penalty = norm_vel * norm_vel;
    
    // Weight coefficients for penalty terms
    // Position and angle are primary objectives
    // Velocities provide damping and discourage oscillation
    float w_angle = 1.0f;
    float w_ang_vel = 0.5f;
    float w_pos = 1.5f;     // Most important reward is to keep ball centered.
    float w_vel = 0.5f;
    
    // Compute total reward (negative of weighted penalty sum)
    result.reward = -(w_angle * angle_penalty + 
                     w_ang_vel * ang_vel_penalty + 
                     w_pos * pos_penalty + 
                     w_vel * vel_penalty);
    
    return result;
}


