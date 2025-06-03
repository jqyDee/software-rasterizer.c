#pragma once

#include <stddef.h>

#define TITLE "rasterizer.c"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

#define EPSILON 1e-5f
#define DEFAULT_TRIANGLE_COUNT 10

#define FOV 45

typedef struct vec3f_s {
  float x, y, z;
} vec3f;

typedef struct mesh_s {
  vec3f *positions;
  size_t vertex_count;
} mesh;

typedef struct cam_s {
  vec3f pos;
  float pitch, yaw;
} cam;

float det_2D(vec3f v1, vec3f v2);
