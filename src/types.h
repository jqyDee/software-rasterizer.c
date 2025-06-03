#pragma once

#include <stddef.h>

#include "raylib.h"

#define TITLE "rasterizer.c"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

// #define EPSILON 1e-5f

typedef struct vec3f_s {
  float x, y, z;
} vec3f;

typedef struct render_target_s {
  int screen_width, screen_height;
  float aspect_ratio;
  Color *framebuffer;
  float *depthbuffer;
  Texture2D screen_texture;
} renderer;

typedef struct mesh_s {
  vec3f *vertices;
  size_t vertex_count;
} mesh;

typedef struct cam_s {
  vec3f pos;
  float pitch, yaw;
  float fov;
  float focal_length;
} cam;

typedef struct world_s {
  cam *cam;
  renderer *renderer;
  mesh *meshes;
  size_t mesh_count;
} world;

