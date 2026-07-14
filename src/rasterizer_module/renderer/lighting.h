#pragma once

#include "raylib.h"

#include "../math/vec.h"
#include "renderer.h"

typedef struct world_s world;

#define MAX_LIGHT_SOURCES 32

typedef struct light_s {
    vec3f light_dir;
    Color color;
    float intensity;
} light_t;

void compute_light_dirs_cam(world *world);
