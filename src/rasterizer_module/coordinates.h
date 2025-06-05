#pragma once

#include <stdbool.h>

#include "types.h"

bool project(const world *world, const vec3f world_pos, vec3f *out);

bool point_in_triangle(const vec3f p, const vec3f a, const vec3f b,
                       const vec3f c, float *u, float *v, float *w);

vec3f rotate_vector(const vec3f v, const float pitch, const float yaw);
