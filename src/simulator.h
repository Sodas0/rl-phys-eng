#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "world.h"
#include <stdint.h>

// Observation dimension: simulator is the single authority on state semantics
#define SIM_OBS_DIM 4

// Actuator state: provides realistic dynamics for beam control
typedef struct {
    float angle;              // Current beam angle (radians)
    float angular_velocity;   // Current angular velocity (rad/s)
} Actuator;

// Minimal simulator: wraps World and provides clean API
typedef struct {
    World world;
    char scene_path[256];
    uint32_t seed;
    float dt;         // Fixed timestep (simulator-owned)
    Actuator actuator;  // Actuator state with dynamics
} Simulator;

// Core API
Simulator* sim_create(const char* scene_path, uint32_t seed, float dt);
void sim_destroy(Simulator* sim);

// Reset simulator to randomized initial state for learning
// Applies minimal controlled randomization:
//   - Ball position: ±20% of beam half-length along beam axis
//   - Beam angle: ±5 degrees (±0.087 radians)
//   - All velocities set to zero
// Uses deterministic RNG seeded with sim->seed for reproducibility
void sim_reset(Simulator* sim);

void sim_step(Simulator* sim, float action);

// Read-only access to world for rendering
World* sim_get_world(Simulator* sim);

// Observation accessor: extracts state vector from simulator
// Computes 4D observation vector:
//   obs_out[0]: beam angle θ (radians)
//   obs_out[1]: beam angular velocity θ̇ (rad/s)
//   obs_out[2]: ball position along beam x, relative to beam center (pixels)
//   obs_out[3]: ball velocity along beam ẋ, projected onto beam axis (pixels/s)
//
// Coordinate assumptions (invariants):
//   - Body.position is center of mass in world coordinates
//   - Beam local x-axis is defined by beam->angle (rotated from world +x)
//   - Ball is at body index 1 (hardcoded convention)
//
// obs_dim: size of obs_out buffer (must be >= SIM_OBS_DIM)
void sim_get_observation(const Simulator* sim, float* obs_out, int obs_dim);

#endif
