# phys-engine

A 2D physics engine written in C with SDL2 rendering.

My goal with this project is to use this as a physics engine for elementary RL training exerices. The current state has full capability to leverage this engine as a sandbox for training agents on classic control tasks like ball balancing. 


## Demos

Manual Control of Physics Based Actuator (Beam) for Beam Balance Task:

https://github.com/user-attachments/assets/adcab64b-75db-405b-b858-6104af13cdd2

Trained Policy Controlling Beam:

https://github.com/user-attachments/assets/be5a38c5-a90e-4438-8b09-40d59ea6d082

Fun Showcase of Physics:

https://github.com/user-attachments/assets/de39ddf9-bbd8-478f-8692-32b7ad2940a2

## Features

- Circles and rectangles with rotation and realistic collisions
- Collision detection for all shape pairs; bounciness and stacking
- Configurable gravity and world bounds; bodies can be fixed in place
- Per-body mass, bounciness, color, and initial speed/angle
- Load scenes from JSON (world + bodies); one body can be a controllable beam on a pivot (for now)
- Beam control: apply torque to tilt the beam; smooth response and angle limits
- RL-ready: reset, step with action, get observation (beam angle/speed, ball position/speed), reward, and done flags
- Python API from a C++ wrapper of environment to create envs, step, and render; can run in headless mode for fast training
- Train with PPO; scripts to train and to run a saved policy
- Debug view: show velocity arrows and contact points (contact points only for rectangles for now)

## Requirements

- C compiler (clang or gcc)
- SDL2
- pybind11


## Building [instructions wip]

For RL API:

```bash
make bindings
```

Then in python:
```python
import sim_bindings
```

For playing with sim:

```bash
make all
./sim
```