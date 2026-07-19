#pragma once

#include "raylib.h"

#include "../math/vec.h"
#include "buffers.h"

#include "lighting.h"

typedef struct world_s world;
typedef depthbuffer depthbuffer_t;

// ============ TILE RENDERING ============
#define TILE_SIZE 32
// worst-case tiles: ceil(8192/32)^2 — much more than any real window
#define MAX_SCREEN_TRIS 32768
#define MAX_TILES 16384
// worst-case bin entries: each tri overlaps up to ~(screen/TILE) tiles;
// 32768 tris × 16 avg-tile-overlap = 524288
#define MAX_BIN_ENTRIES 524288

// ============ SHADOW MAP ============
#define SHADOW_MAP_SIZE 1024
#define SHADOW_ORTHO_HALF_EXTENT                                               \
  20.0f // world-space half-size of light frustum, tune to scene
#define SHADOW_BIAS 0.50f     // world units, kills shadow acne — tune visually
#define SHADOW_DARKNESS 0.50f // multiplier for occluded pixels

typedef struct screen_tri_s {
  vec3f v[3];
  vec2f uv[3];
  float inv_z[3];
  Color *tex;
  int tex_w, tex_h;
  Color flat_color;
  int instance_id;
  int bx0, by0, bx1, by1; /* screen-space bbox, clamped */
  float fp_bias;          /* conservative FP error bound for edge functions */
  vec3f light_rgb;
  vec3f normals[3];
  float shadow[3];
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
  depthbuffer_t *depthbuffer;
  idbuffer *idbuffer; /* instance index per pixel, -1 = empty */
  albedobuffer *albedobuffer;
  normalbuffer *normalbuffer;
  depthbuffer_t *shadow_depthbuffer;

  vec3f light_right, light_up, light_fwd;

  Texture2D screen_texture;

  Texture2D albedo_texture, normal_texture;
  Shader phong_shader;
  int phong_normal_map_loc, phong_light_dirs_loc, phong_light_colors_loc,
      phong_light_intensities_loc, phong_light_count_loc, phong_ambient_loc;

  Texture2D sky_texture; /* equirectangular panorama; id 0 = no sky */
  int phong_sky_map_loc, phong_use_sky_loc, phong_cam_right_loc,
      phong_cam_up_loc, phong_cam_fwd_loc, phong_focal_loc, phong_aspect_loc;
  int phong_boost_loc, phong_time_loc,
      phong_speed_loc; /* speed / boost screen FX */
} renderer;

void render(world *world);

void resize_renderer_to(world *world, int display_w, int display_h);
void resize_renderer(world *world);
