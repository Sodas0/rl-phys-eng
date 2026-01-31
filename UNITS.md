# Physics Engine Unit System

This document defines the consistent unit system used throughout the physics engine.

## Core Principle

**Scale Factor: 100 pixels = 1 meter**

This scale provides intuitive sizes for common objects while maintaining numerical stability.

## Unit Definitions

### Base Units

| Quantity | Unit | Symbol | Notes |
|----------|------|--------|-------|
| Length | pixels | px | Screen coordinates |
| Mass | kilograms | kg | SI unit |
| Time | seconds | s | Real-time |

### Derived Units

| Quantity | Unit | Formula | Example |
|----------|------|---------|---------|
| Velocity | pixels/second | px/s | 200 px/s = 2.0 m/s |
| Acceleration | pixels/s² | px/s² | 981 px/s² = 9.81 m/s² |
| Force | pixel·kg/s² | px·kg/s² | F = ma |
| Momentum | pixel·kg/s | px·kg/s | p = mv |
| Angular velocity | radians/second | rad/s | Same in both systems |
| Moment of inertia | pixel²·kg | px²·kg | I = mr² (for point mass) |

## Standard Constants

### Defined in `world.h`

```c
#define PIXELS_PER_METER 100.0f           // Scale factor: 100 px = 1 m
#define GRAVITY_EARTH_MS2 9.81f           // Earth gravity in m/s²
#define GRAVITY_EARTH_PX 981.0f           // Earth gravity in px/s² (9.81 * 100)
```

### Physics Thresholds

```c
REST_VEL_EPS = 5.0 px/s = 0.05 m/s        // Resting contact threshold
```

Objects moving slower than this are treated as at rest to prevent jitter in stacked configurations.

## Practical Examples

### Screen Space
- **Window**: 1920×1080 pixels = **19.2m × 10.8m** physical space
- Think of your screen as a ~20m wide room

### Common Objects

| Object | Pixels | Physical Size | Notes |
|--------|--------|---------------|-------|
| Basketball | 30 px radius | 0.30 m (30 cm) | Typical ball size |
| Small box | 40×40 px | 0.4×0.4 m | Hand-sized |
| Large box | 100×60 px | 1.0×0.6 m | Torso-sized |
| Floor/wall | 50 px thick | 0.5 m | Structural element |

### Velocities

| Velocity | Pixels/s | Physical | Scenario |
|----------|----------|----------|----------|
| Slow roll | 50 px/s | 0.5 m/s | Gentle motion |
| Walking | 150 px/s | 1.5 m/s | Casual movement |
| Running | 400 px/s | 4.0 m/s | Fast motion |
| Fast throw | 800 px/s | 8.0 m/s | High-speed impact |

### Gravity

Standard Earth gravity:
- **9.81 m/s²** = **981 pixels/s²**

In scene files:
```json
"gravity": [0, 981.0]
```

The y-component is positive because screen coordinates increase downward.

### Mass

Use realistic SI masses (kilograms):
- Small ball: 0.5 - 1.5 kg
- Box: 1.0 - 5.0 kg
- Heavy object: 10.0+ kg
- Static objects: mass = 0 (infinite mass)

### Time Step (dt)

Standard fixed timestep:
- **dt = 0.016667 seconds** (60 Hz, 1/60 s)
- **dt = 0.008333 seconds** (120 Hz, 1/120 s) - for high precision

```json
"dt": 0.016667
```

## Conversion Helpers

### Pixels ↔ Meters

```c
// Pixels to meters
float meters = pixels / PIXELS_PER_METER;

// Meters to pixels
float pixels = meters * PIXELS_PER_METER;
```

### Physical Velocity ↔ Pixel Velocity

```c
// Pixels/s to m/s
float velocity_ms = velocity_px / PIXELS_PER_METER;

// m/s to pixels/s
float velocity_px = velocity_ms * PIXELS_PER_METER;
```

### Physical Acceleration ↔ Pixel Acceleration

```c
// Pixels/s² to m/s²
float accel_ms2 = accel_px / PIXELS_PER_METER;

// m/s² to pixels/s²
float accel_px = accel_ms2 * PIXELS_PER_METER;
```

## Scene File Guidelines

### Example: Gravity Scene

```json
{
  "world": {
    "gravity": [0, 981.0],    // Earth gravity (9.81 m/s²)
    "bounds": {
      "left": 0,
      "top": 0,
      "right": 1920,          // 19.2 m wide
      "bottom": 1080          // 10.8 m tall
    }
  },
  "bodies": [
    {
      "type": "circle",
      "position": [960, 200],  // Center-top (9.6m, 2.0m)
      "radius": 30,            // 0.30 m radius (basketball)
      "mass": 1.0,             // 1 kg
      "restitution": 0.8,      // 80% bounce
      "velocity": [200, 0]     // 2.0 m/s to the right
    }
  ]
}
```

### Zero-Gravity Collision Tests

For testing collisions without gravity:
```json
"gravity": [0, 0]
```
