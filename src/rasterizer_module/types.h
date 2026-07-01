#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "raylib.h"

#define TITLE "rasterizer.c"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800
#define BASE_RENDER_WIDTH 800

#define CUT_OFF_PARALLEL_DRAWING 32

#define EPSILON 1e-2f
#define NEAR_PLANE 0.1f

typedef struct vec3f_s {
  float x, y, z;
} vec3f;

typedef struct render_target_s {
  int screen_width, screen_height;   // framebuffer/buffer dimensions
  int display_width, display_height; // logical draw area for DrawTexturePro
  int window_x, window_y;
  float aspect_ratio;
  Color *framebuffer;
  float *depthbuffer;
  Texture2D screen_texture;
} renderer;

typedef struct mesh_s {
  vec3f *vertices;
  size_t vertex_count;
} mesh;

typedef struct mesh_instance_s {
  mesh *mesh;
  vec3f pos;
} mesh_instance;

typedef struct cam_s {
  vec3f pos;
  float pitch, yaw;
  float fov;
  float focal_length;
} cam;

typedef struct settings_s {
  bool show_debug_gui;
  int  render_width;
  int  parallel_cutoff_rows;
  float near_plane;
} settings;

typedef struct world_s {
  cam *cam;
  renderer *renderer;
  mesh *mesh_data;
  size_t mesh_data_count;
  mesh_instance *instances;
  size_t instance_count;
  size_t obj_count;
  char **obj_paths;
  settings settings;
} world;
