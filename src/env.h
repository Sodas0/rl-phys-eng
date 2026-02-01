#ifndef ENV_H
#define ENV_H

#include "simulator.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque environment handle
typedef struct Env Env;

// Observation dimension - must match simulator's observation space
#define OBS_DIM SIM_OBS_DIM

// Episode configuration
#define MAX_EPISODE_STEPS 2400  // Time-limit for truncation

// Compile-time check that Env and Simulator agree on observation dimension
// This prevents silent memory corruption
#if defined(__cplusplus)
static_assert(OBS_DIM == SIM_OBS_DIM, "Env/Simulator observation dimension mismatch");
#else
_Static_assert(OBS_DIM == SIM_OBS_DIM, "Env/Simulator observation dimension mismatch");
#endif

// Action: torque in [-1, 1]
typedef struct {
    float torque;  // Normalized torque command in range [-1, 1]
} Action;

// Observation: 4D state vector (semantics defined by Simulator)
//   data[0]: beam angle θ (radians)
//   data[1]: beam angular velocity θ̇ (rad/s)
//   data[2]: ball position along beam x, relative to beam center (pixels)
//   data[3]: ball velocity along beam ẋ, projected onto beam axis (pixels/s)
typedef struct {
    float data[OBS_DIM];
} Observation;

// Result of stepping the environment
typedef struct {
    Observation obs;   // Current observation
    float reward;      // Reward signal
    int terminated;    // Task failure (ball fell off beam): 1 = failed, 0 = continuing
    int truncated;     // Time limit reached: 1 = time's up, 0 = continuing
} StepResult;

// Create a new environment instance that wraps an existing simulator
// sim: already-created simulator instance (env takes ownership)
// Returns: pointer to initialized environment, or NULL on failure
Env* env_create(Simulator* sim);

// Destroy environment and free resources
// env: environment to destroy (can be NULL)
void env_destroy(Env* env);

// Reset environment to initial state
// env: environment to reset
// Returns: initial step result with observation, reward=0.0, terminated=0, truncated=0
StepResult env_reset(Env* env);

// Step the environment forward by one timestep
// env: environment to step
// action: action to apply (torque in [-1, 1])
// Returns: step result with observation, reward, terminated, and truncated flags
StepResult env_step(Env* env, Action action);

// Render the environment for debugging purposes (optional, disabled by default)
// This is a thin passthrough to the underlying simulator rendering.
// Rendering does NOT affect physics, observations, rewards, or determinism.
// env: environment to render
// If rendering is disabled or simulator is headless, this is a no-op.
void env_render(Env* env);

// Enable or disable rendering for this environment
// By default, rendering is disabled for headless operation.
// env: environment to configure
// enabled: 1 to enable rendering, 0 to disable
void env_set_render_enabled(Env* env, int enabled);

#ifdef __cplusplus
}
#endif

#endif // ENV_H
