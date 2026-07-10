#pragma once

#include "raylib.h"

#include "../math/vec.h"
#include "buffers.h"

typedef struct world_s world;

#define TILE_SIZE 32
// worst-case tiles: ceil(8192/32)^2 — much more than any real window
#define MAX_SCREEN_TRIS 32768
#define MAX_TILES 16384
// worst-case bin entries: each tri overlaps up to ~(screen/TILE) tiles;
// 32768 tris × 16 avg-tile-overlap = 524288
#define MAX_BIN_ENTRIES 524288

typedef struct screen_tri_s {
  vec3f v[3];
  vec2f uv[3];
  float inv_z[3];
  Color *tex;
  int tex_w, tex_h;
  Color flat_color;
  int instance_id;
  int bx0, by0, bx1, by1; /* screen-space bbox, clamped */
  float fp_bias;      /* conservative FP error bound for edge functions */
} screen_tri_t;

typedef struct edge_walk_s {
  float e0_dx, e0_dy, e0_seed;
  float e1_dx, e1_dy, e1_seed;
  float e2_dx, e2_dy, e2_seed;
  float inv_signed_area;
} edge_walk_t;

typedef struct renderer_s {
  int screen_width, screen_height;   // framebuffer/buffer dimensions
  int display_width, display_height; // logical draw area for DrawTexturePro
  int window_pos_x, window_pos_y;
  float aspect_ratio;
  framebuffer *framebuffer;
  depthbuffer *depthbuffer;
  idbuffer *idbuffer; /* instance index per pixel, -1 = empty */
  Texture2D screen_texture;
} renderer;

void render(world *world);

void resize_renderer_to(world *world, int display_w, int display_h);
void resize_renderer(world *world);
