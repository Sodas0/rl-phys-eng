#include "env_wrapper.h"
#include <stdexcept>

Environment::Environment(Simulator* sim)
    : sim_(sim), env_(nullptr)
{
    if (!sim_) {
        throw std::invalid_argument("Simulator pointer cannot be null");
    }
    
    // Pure pass-through: create environment wrapper around simulator
    env_ = env_create(sim_);
    if (!env_) {
        throw std::runtime_error("Failed to create environment");
    }
}

Environment::~Environment() {
    // env_destroy takes ownership and destroys the simulator
    // No need to explicitly destroy sim_ - env_destroy handles it
    if (env_) {
        env_destroy(env_);
    }
}

StepResult Environment::reset() {
    // Pure pass-through to C API
    return env_reset(env_);
}

StepResult Environment::step(float action) {
    // Pure pass-through: construct Action and forward
    Action a;
    a.torque = action;
    return env_step(env_, a);
}

void Environment::render() {
    // Pure pass-through to C API
    // Rendering backend is handled inside the simulator
    env_render(env_);
}
