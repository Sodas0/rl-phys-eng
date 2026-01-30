# phys-engine

A 2D physics engine written in C with SDL2 rendering.

My goal with this project is to use this as a very simple physics engine for elementary RL training as a learning exercise. Future projects will leverage this engine as a sandbox for training agents on classic control tasks like ball balancing, target hitting, and multi-body coordination.

**Note:** The RL interface is a work in progress. 

## Demos

https://github.com/user-attachments/assets/f86cbff9-2a39-44fe-be45-3632ccf1025a

https://github.com/user-attachments/assets/de39ddf9-bbd8-478f-8692-32b7ad2940a2

## Features

- Circle-based rigid body simulation
- Collision detection and impulse-based response
- Configurable gravity and world boundaries
- Adjustable body properties (mass, restitution/bounciness)
- Debug visualization for velocity vectors
- Bulk spawning utilities for stress testing

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

## Usage

Bodies can be created and customized in `main.c`:

```c
Body b = body_default(vec2(400, 100), 60.0f);  // position, radius
b.color = (SDL_Color){255, 100, 100, 255};
b.restitution = 1.0f;  // bounciness [0-1]
world_add_body(&world, b);
```
