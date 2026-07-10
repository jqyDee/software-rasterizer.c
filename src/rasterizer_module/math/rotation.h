#pragma once

#include "vec.h"


#define SNAP_THRESHOLD 0.01f

// sine / cosine lookups
static const float sine_lookup[4] = {0.0f, 1.0f, 0.0f, -1.0f};
static const float cosine_lookup[4] = {1.0f, 0.0f, -1.0f, 0.0f};

// ============== CAM ROTATIONS ==============
vec3f rotate_x(vec3f v, float pitch);
vec3f rotate_y(vec3f v, float yaw);
vec3f rotate_z(vec3f v, float roll);

// ============== INSTANCE ROTATIONS ==============
vec3f rotate_x_snapped(vec3f v, float pitch);
vec3f rotate_y_snapped(vec3f v, float yaw);
vec3f rotate_z_snapped(vec3f v, float roll);
