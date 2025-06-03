#pragma once

#include <stddef.h>

#define TITLE "rasterizer.c"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define EPSILON 1e-5f

#define DEFAULT_TRIANGLE_COUNT 10

typedef struct vec2_s {
  int x, y;
} vec2;

typedef struct vec2f_s {
  float x, y;
} vec2f;

typedef struct vec3_s {
  int x, y, z;
} vec3;

typedef struct vec3f_s {
  double x, y, z;
} vec3f;

typedef struct mesh_s {
  vec3f* positions;
  size_t vertex_count;
} mesh;

float det_2D(vec3f v1, vec3f v2);

