#ifndef ENV_WRAPPER_H
#define ENV_WRAPPER_H

#include "simulator.h"
#include "env.h"

// Pure RAII wrapper around C environment API
// NO LOGIC: This is a transparent pass-through
// NO KNOWLEDGE: Does not configure scenes, seeds, timesteps, or rendering backends
// ONLY PURPOSE: Lifetime management + pybind11 compatibility
class Environment {
private:
    Simulator* sim_;
    Env* env_;

public:
    // Constructor: takes ownership of pre-configured simulator
    // sim: already-created simulator (caller creates with scene, seed, dt)
    // The wrapper owns the simulator but does not configure it
    explicit Environment(Simulator* sim);

    // Destructor: ensures proper cleanup order (env before sim)
    ~Environment();

    // Disable copy (unique resource ownership)
    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

    // Reset environment to randomized initial state
    // Pure pass-through to env_reset()
    StepResult reset();

    // Step environment forward by one timestep
    // Pure pass-through to env_step()
    StepResult step(float action);

    // Render environment
    // Pure pass-through to env_render()
    // Rendering backend management is the responsibility of env/simulator
    void render();
};

#endif // ENV_WRAPPER_H
