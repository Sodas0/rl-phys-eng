#include <pybind11/pybind11.h>
#include "env_wrapper.h"
#include "simulator.h"

namespace py = pybind11;

// Helper: Convert Observation to Python tuple
static py::tuple obs_to_tuple(const Observation& obs) {
    return py::make_tuple(obs.data[0], obs.data[1], obs.data[2], obs.data[3]);
}

PYBIND11_MODULE(sim_bindings, m) {
    m.doc() = "Physics simulator bindings for RL";
    
    // Bind Environment class with unique_ptr for automatic lifetime management
    // Python never sees or holds a Simulator* - it's fully owned by Environment
    py::class_<Environment, std::unique_ptr<Environment>>(m, "Environment")
        // Constructor: takes scene config and creates simulator internally
        .def(py::init([](const std::string& scene_path, uint32_t seed, float dt, bool headless) {
            // Create simulator (C API)
            Simulator* sim = sim_create(scene_path.c_str(), seed, dt, headless ? 1 : 0);
            if (!sim) {
                throw std::runtime_error("Failed to create simulator");
            }
            
            // Construct Environment (takes ownership of sim)
            // Environment destructor will call env_destroy which frees the simulator
            // Return raw pointer - pybind11 will wrap it in unique_ptr
            return new Environment(sim);
        }),
        py::arg("scene_path"),
        py::arg("seed"),
        py::arg("dt"),
        py::arg("headless") = true,
        "Create a new environment with specified scene and parameters")
        
        // reset() returns observation tuple
        .def("reset", [](Environment& env) {
            StepResult result = env.reset();
            return obs_to_tuple(result.obs);
        },
        "Reset environment to initial state, returns observation tuple")
        
        // step(action) returns (obs, reward, terminated, truncated)
        .def("step", [](Environment& env, float action) {
            StepResult result = env.step(action);
            return py::make_tuple(
                obs_to_tuple(result.obs),
                result.reward,
                static_cast<bool>(result.terminated),
                static_cast<bool>(result.truncated)
            );
        },
        py::arg("action"),
        "Step environment with action, returns (obs, reward, terminated, truncated)")
        
        // render() for visualization (no-op if headless)
        .def("render", &Environment::render,
             "Render the environment (no-op if headless)");
}
