#include "simulator.h"
#include "scene.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Actuator dynamics parameters
// NOTE FOR MY OWN INTUITION:
// when tau big -- slow motor response ; when tau small -- fast motor response
#define MAX_BEAM_SPEED 2.0f        // rad/s (maximum angular velocity)
#define ACTUATOR_TAU 0.1f          // seconds (time constant for first-order lag)
#define BEAM_ANGLE_MAX 0.5f        // radians (saturation limit)

// Initial-state randomization parameters for learning
// These ranges force immediate corrective control while keeping all states recoverable
#define RANDOMIZE_BALL_POSITION_RATIO 0.2f   // ±20% of beam half-length
#define RANDOMIZE_BEAM_ANGLE_RAD 0.349f      // ±20 degrees (≈ ±0.349 radians)

// Helper: apply actuator pose (matches main.c logic exactly)
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

Simulator* sim_create(const char* scene_path, uint32_t seed, float dt) {
    Simulator* sim = (Simulator*)malloc(sizeof(Simulator));
    if (!sim) return NULL;
    
    strncpy(sim->scene_path, scene_path, sizeof(sim->scene_path) - 1);
    sim->scene_path[sizeof(sim->scene_path) - 1] = '\0';
    sim->seed = seed;
    sim->dt = dt;
    
    // Initialize actuator state
    sim->actuator.angle = 0.0f;
    sim->actuator.angular_velocity = 0.0f;
    
    // Load initial scene
    if (scene_load(scene_path, &sim->world) != 0) {
        free(sim);
        return NULL;
    }
    
    // Set simulator-owned dt
    sim->world.dt = dt;
    
    // Seed the world's RNG
    world_seed(&sim->world, seed);
    
    return sim;
}

void sim_destroy(Simulator* sim) {
    if (sim) {
        free(sim);
    }
}

void sim_reset(Simulator* sim) {
    if (!sim) return;
    
    // Reload scene from JSON (deterministic base state)
    scene_load(sim->scene_path, &sim->world);
    sim->world.dt = sim->dt;
    world_seed(&sim->world, sim->seed);
    
    // Reset actuator state to zero before randomization
    sim->actuator.angle = 0.0f;
    sim->actuator.angular_velocity = 0.0f;
    
    // Apply initial-state randomization for learning
    // This forces feedback control without making episodes unrecoverable
    
    printf("[sim_reset] seed=%u, rng_state=%u, actuator_idx=%d\n", 
           sim->seed, sim->world.rng_state, sim->world.actuator_body_index);
    
    // Get beam (actuator) body
    Body* beam = world_get_body(&sim->world, sim->world.actuator_body_index);
    if (!beam || beam->shape.type != SHAPE_RECT) {
        return;  // No randomization if beam is invalid
    }
    
    // Get ball body (hardcoded convention: ball is body 1)
    const int BALL_BODY_INDEX = 1;
    Body* ball = world_get_body(&sim->world, BALL_BODY_INDEX);
    if (!ball) {
        return;  // No randomization if ball is invalid
    }
    
    // --- Randomize beam angle: uniform in ±5 degrees (±0.087 rad) ---
    // Generate random value in [-1, 1]
    float random_angle_norm = world_randf(&sim->world) * 2.0f - 1.0f;
    float initial_beam_angle = random_angle_norm * RANDOMIZE_BEAM_ANGLE_RAD;
    printf("[sim_reset] random_angle_norm=%.3f, beam_angle=%.3f\n", 
           random_angle_norm, initial_beam_angle);
    
    // Apply randomized beam angle to actuator
    sim->actuator.angle = initial_beam_angle;
    apply_actuator_pose(&sim->world, sim->actuator.angle);
    
    // --- Randomize ball position: add random X offset to JSON position ---
    float beam_half_length = beam->shape.rect.width * 0.5f;
    
    // Generate random X offset: ±20% of beam half-length
    float random_pos_norm = world_randf(&sim->world) * 2.0f - 1.0f;
    float random_x_offset = random_pos_norm * RANDOMIZE_BALL_POSITION_RATIO * beam_half_length;
    printf("[sim_reset] random_pos_norm=%.3f, random_x_offset=%.1f px\n", 
           random_pos_norm, random_x_offset);
    
    // Add random X offset to the JSON position (Y stays as-is from JSON)
    ball->position.x += random_x_offset;
    
    // Keep ball velocities at zero (no velocity randomization yet)
    ball->velocity = VEC2_ZERO;
    ball->angular_velocity = 0.0f;
}

// Update actuator dynamics and apply to world
// action ∈ [-1, 1]: normalized motor command
void sim_step(Simulator* sim, float action) {
    if (!sim) return;
    
    // Clamp action to valid range
    if (action > 1.0f) action = 1.0f;
    if (action < -1.0f) action = -1.0f;
    
    // First-order actuator dynamics
    float target_velocity = action * MAX_BEAM_SPEED;
    float dt = sim->dt;
    float tau = ACTUATOR_TAU;
    
    // Exponential filter: v_new = v_old + (dt/tau) * (target - v_old)
    // this is a differential equation.
    // physical interpretation: 
    // angular acceleration is determined by the difference between the target velocity and the current velocity, scaled by constant 1/tau.
    // which results in fast acceleration when the difference is large, and slow acceleration when the difference is small, kind of like a dampening effect.
    sim->actuator.angular_velocity += (dt / tau) * (target_velocity - sim->actuator.angular_velocity);
    
    // Integrate angle
    sim->actuator.angle += sim->actuator.angular_velocity * dt; 
    
    // Apply saturation limits
    if (sim->actuator.angle > BEAM_ANGLE_MAX) {
        sim->actuator.angle = BEAM_ANGLE_MAX;
        sim->actuator.angular_velocity = 0.0f;  // Stop at limit
    }
    if (sim->actuator.angle < -BEAM_ANGLE_MAX) {
        sim->actuator.angle = -BEAM_ANGLE_MAX;
        sim->actuator.angular_velocity = 0.0f;  // Stop at limit
    }
    
    // Apply actuator pose before physics step
    apply_actuator_pose(&sim->world, sim->actuator.angle);
    
    // Advance physics by one timestep
    world_step(&sim->world);
    
    // Reapply actuator pose after physics step
    apply_actuator_pose(&sim->world, sim->actuator.angle);
}

// pointer to world for rendering purposes
World* sim_get_world(Simulator* sim) {
    return sim ? &sim->world : NULL;
}

// Extract observation vector from simulator state
void sim_get_observation(const Simulator* sim, float* obs_out, int obs_dim) {
    // Validate inputs
    if (!sim || !obs_out || obs_dim < SIM_OBS_DIM) {
        // Safety: zero out buffer on error
        if (obs_out && obs_dim > 0) {
            for (int i = 0; i < obs_dim; i++) {
                obs_out[i] = 0.0f;
            }
        }
        return;
    }
    
    // Get beam (actuator) body
    const Body* beam = world_get_body((World*)&sim->world, sim->world.actuator_body_index);
    if (!beam) {
        // Zero out on error
        for (int i = 0; i < SIM_OBS_DIM; i++) {
            obs_out[i] = 0.0f;
        }
        return;
    }
    
    // TODO: figure out design for not hardcoding ball body index.
    // Get ball body (hardcoded convention: ball is body 1)
    const int BALL_BODY_INDEX = 1;
    const Body* ball = world_get_body((World*)&sim->world, BALL_BODY_INDEX);
    if (!ball) {
        // Zero out on error
        for (int i = 0; i < SIM_OBS_DIM; i++) {
            obs_out[i] = 0.0f;
        }
        return;
    }
    
    // Extract actuator state
    float beam_angle = sim->actuator.angle;
    float beam_angular_velocity = sim->actuator.angular_velocity;
    
    // Cache trig for projection (optimization for hot loop)
    float c = cosf(beam_angle);
    float s = sinf(beam_angle);
    
    // Compute vector from beam center to ball center (both in world coordinates)
    // Invariant: Body.position is center of mass in world coordinates
    float dx = ball->position.x - beam->position.x;
    float dy = ball->position.y - beam->position.y;
    
    // Project onto beam's local x-axis
    // Beam local x-axis in world coords: [cos(θ), sin(θ)]
    // This gives ball position along beam, relative to the beam's center
    float x_along_beam = dx * c + dy * s;
    
    // Project ball velocity onto beam axis
    // This gives ball velocity along beam in beam's local frame
    float vel_along_beam = ball->velocity.x * c + ball->velocity.y * s;
    
    // Write observation vector
    obs_out[0] = beam_angle;              // beam angle θ (radians)
    obs_out[1] = beam_angular_velocity;   // beam angular velocity θ̇ (rad/s)
    obs_out[2] = x_along_beam;            // ball position along beam (pixels)
    obs_out[3] = vel_along_beam;          // ball velocity along beam (pixels/s)
}
