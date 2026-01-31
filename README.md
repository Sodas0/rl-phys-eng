# phys-engine

A 2D physics engine written in C with SDL2 rendering.

My goal with this project is to use this as a very simple physics engine for elementary RL training as a learning exercise. Future projects will leverage this engine as a sandbox for training agents on classic control tasks like ball balancing, target hitting, and multi-body coordination.

**Note:** The RL interface is a work in progress. 

## Demos

https://github.com/user-attachments/assets/f86cbff9-2a39-44fe-be45-3632ccf1025a

https://github.com/user-attachments/assets/de39ddf9-bbd8-478f-8692-32b7ad2940a2

## Features

- Circle and rectangle rigid body simulation with rotation
- Collision detection and impulse-based response (SAT for rectangles)
- Configurable gravity and world boundaries
- Adjustable body properties (mass, restitution/bounciness)
- Debug visualization for velocity vectors and contact points
- JSON scene loading system
- Deterministic RNG for reproducible simulations
- Uint system where 100 pixels = 1 meter

## Requirements

- C compiler (clang or gcc)
- SDL2

## Building

```bash
make
```

## Running

```bash
make run
```
