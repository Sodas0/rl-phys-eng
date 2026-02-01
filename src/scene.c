#include "scene.h"
#include "body.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: Read entire file into string
static char* read_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer
    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read file
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    fclose(file);

    return buffer;
}

// Helper: Parse Vec2 from JSON array [x, y]
static int parse_vec2(const cJSON *item, Vec2 *out) {
    if (!cJSON_IsArray(item) || cJSON_GetArraySize(item) != 2) {
        return -1;
    }

    cJSON *x = cJSON_GetArrayItem(item, 0);
    cJSON *y = cJSON_GetArrayItem(item, 1);

    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        return -1;
    }

    out->x = (float)x->valuedouble;
    out->y = (float)y->valuedouble;
    return 0;
}

// Helper: Parse SDL_Color from JSON array [r, g, b, a]
static int parse_color(const cJSON *item, SDL_Color *out) {
    if (!cJSON_IsArray(item) || cJSON_GetArraySize(item) != 4) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        cJSON *val = cJSON_GetArrayItem(item, i);
        if (!cJSON_IsNumber(val)) {
            return -1;
        }
        int component = (int)val->valuedouble;
        if (component < 0 || component > 255) {
            return -1;
        }
        ((unsigned char*)out)[i] = (unsigned char)component;
    }
    return 0;
}

// Helper: Parse world configuration
static int parse_world_config(const cJSON *world_obj, World *world) {
    // Parse gravity
    cJSON *gravity = cJSON_GetObjectItem(world_obj, "gravity");
    Vec2 grav = {0, 98.1f}; // Default
    if (gravity && parse_vec2(gravity, &grav) != 0) {
        fprintf(stderr, "Invalid gravity format\n");
        return -1;
    }

    // dt is owned by the simulator; use default here, simulator overwrites after load
    float timestep = 1.0f / 60.0f;

    // Initialize world
    world_init(world, grav, timestep);

    // Parse bounds
    cJSON *bounds = cJSON_GetObjectItem(world_obj, "bounds");
    if (bounds && cJSON_IsObject(bounds)) {
        cJSON *left = cJSON_GetObjectItem(bounds, "left");
        cJSON *top = cJSON_GetObjectItem(bounds, "top");
        cJSON *right = cJSON_GetObjectItem(bounds, "right");
        cJSON *bottom = cJSON_GetObjectItem(bounds, "bottom");

        if (left && top && right && bottom &&
            cJSON_IsNumber(left) && cJSON_IsNumber(top) &&
            cJSON_IsNumber(right) && cJSON_IsNumber(bottom)) {
            world_set_bounds(world,
                (float)left->valuedouble,
                (float)top->valuedouble,
                (float)right->valuedouble,
                (float)bottom->valuedouble);
        }
    }

    return 0;
}

// Helper: Parse a single body from JSON
static int parse_body(const cJSON *body_obj, Body *out) {
    // Get type (required)
    cJSON *type = cJSON_GetObjectItem(body_obj, "type");
    if (!type || !cJSON_IsString(type)) {
        fprintf(stderr, "Body missing 'type' field\n");
        return -1;
    }

    // Get position (required)
    cJSON *position = cJSON_GetObjectItem(body_obj, "position");
    Vec2 pos;
    if (!position || parse_vec2(position, &pos) != 0) {
        fprintf(stderr, "Body missing or invalid 'position' field\n");
        return -1;
    }

    // Parse based on type
    if (strcmp(type->valuestring, "circle") == 0) {
        // Get radius (required for circle)
        cJSON *radius = cJSON_GetObjectItem(body_obj, "radius");
        if (!radius || !cJSON_IsNumber(radius)) {
            fprintf(stderr, "Circle body missing 'radius' field\n");
            return -1;
        }

        float r = (float)radius->valuedouble;

        // Get mass (default 1.0)
        cJSON *mass = cJSON_GetObjectItem(body_obj, "mass");
        float m = mass && cJSON_IsNumber(mass) ? (float)mass->valuedouble : 1.0f;

        // Get restitution (default 0.8)
        cJSON *restitution = cJSON_GetObjectItem(body_obj, "restitution");
        float rest = restitution && cJSON_IsNumber(restitution) ? (float)restitution->valuedouble : 0.8f;

        // Create body
        *out = body_create_circle(pos, r, m, rest);

    } else if (strcmp(type->valuestring, "rect") == 0) {
        // Get width and height (required for rect)
        cJSON *width = cJSON_GetObjectItem(body_obj, "width");
        cJSON *height = cJSON_GetObjectItem(body_obj, "height");
        if (!width || !height || !cJSON_IsNumber(width) || !cJSON_IsNumber(height)) {
            fprintf(stderr, "Rect body missing 'width' or 'height' field\n");
            return -1;
        }

        float w = (float)width->valuedouble;
        float h = (float)height->valuedouble;

        // Get mass (default 1.0)
        cJSON *mass = cJSON_GetObjectItem(body_obj, "mass");
        float m = mass && cJSON_IsNumber(mass) ? (float)mass->valuedouble : 1.0f;

        // Get restitution (default 0.8)
        cJSON *restitution = cJSON_GetObjectItem(body_obj, "restitution");
        float rest = restitution && cJSON_IsNumber(restitution) ? (float)restitution->valuedouble : 0.8f;

        // Create body
        *out = body_create_rect(pos, w, h, m, rest);

    } else {
        fprintf(stderr, "Unknown body type: %s\n", type->valuestring);
        return -1;
    }

    // Parse optional fields

    // Velocity (default [0, 0])
    cJSON *velocity = cJSON_GetObjectItem(body_obj, "velocity");
    if (velocity) {
        Vec2 vel;
        if (parse_vec2(velocity, &vel) == 0) {
            out->velocity = vel;
        }
    }

    // Angular velocity (default 0)
    cJSON *angular_velocity = cJSON_GetObjectItem(body_obj, "angular_velocity");
    if (angular_velocity && cJSON_IsNumber(angular_velocity)) {
        out->angular_velocity = (float)angular_velocity->valuedouble;
    }

    // Angle (default 0)
    cJSON *angle = cJSON_GetObjectItem(body_obj, "angle");
    if (angle && cJSON_IsNumber(angle)) {
        out->angle = (float)angle->valuedouble;
    }

    // Color (default white)
    cJSON *color = cJSON_GetObjectItem(body_obj, "color");
    if (color) {
        SDL_Color col;
        if (parse_color(color, &col) == 0) {
            out->color = col;
        }
    }

    // Static flag (default false)
    cJSON *is_static = cJSON_GetObjectItem(body_obj, "static");
    if (is_static && cJSON_IsTrue(is_static)) {
        body_set_static(out);
    }

    return 0;
}

int scene_load(const char *filepath, World *world) {
    // Read file
    char *json_str = read_file(filepath);
    if (!json_str) {
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            fprintf(stderr, "JSON parse error before: %s\n", error_ptr);
        }
        return -1;
    }

    // Parse world configuration
    cJSON *world_obj = cJSON_GetObjectItem(root, "world");
    if (world_obj && cJSON_IsObject(world_obj)) {
        if (parse_world_config(world_obj, world) != 0) {
            cJSON_Delete(root);
            return -1;
        }
    } else {
        // Use defaults (dt is simulator-owned; simulator overwrites after load)
        world_init(world, vec2(0, 98.1f), 1.0f / 60.0f);
    }

    // Parse bodies array
    cJSON *bodies = cJSON_GetObjectItem(root, "bodies");
    if (bodies && cJSON_IsArray(bodies)) {
        int body_count = cJSON_GetArraySize(bodies);
        
        for (int i = 0; i < body_count; i++) {
            cJSON *body_obj = cJSON_GetArrayItem(bodies, i);
            if (!cJSON_IsObject(body_obj)) {
                fprintf(stderr, "Body %d is not an object\n", i);
                continue;
            }

            Body body;
            if (parse_body(body_obj, &body) == 0) {
                if (world_add_body(world, body) != -1) {
                    cJSON *actuator = cJSON_GetObjectItem(body_obj, "actuator");
                    if (actuator && cJSON_IsTrue(actuator)) {
                        world->actuator_body_index = world->body_count - 1;
                        world->actuator_pivot = body.position;
                    }
                } else {
                    fprintf(stderr, "Warning: Failed to add body %d (world full?)\n", i);
                }
            } else {
                fprintf(stderr, "Failed to parse body %d\n", i);
            }
        }
    }

    cJSON_Delete(root);
    // printf("Scene loaded: %s\n", filepath);  // Debug output disabled
    return 0;
}
