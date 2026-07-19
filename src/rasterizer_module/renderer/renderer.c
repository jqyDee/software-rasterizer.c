#include "renderer.h"
#include "../testing.h"

#include <stdio.h>
#include <stdlib.h>

#include "../engine/shaders.h"
#include "../engine/texture.h"
#include "../world.h"
#include "buffers.h"
#include "draw.h"
#include "geometry.h"
#include "shadow.h"
#include "tiling.h"

void render(world *world) {
  int screen_width = world->renderer->screen_width;
  int screen_height = world->renderer->screen_height;

  int tiles_x = (screen_width + TILE_SIZE - 1) / TILE_SIZE;
  int tiles_y = (screen_height + TILE_SIZE - 1) / TILE_SIZE;

  int num_tiles = tiles_x * tiles_y;

  // static buffers: allocated once, reused every frame
  static screen_tri_t screen_triangles[MAX_SCREEN_TRIS];
  static int tile_count_buf[MAX_TILES];
  static int tile_start_buf[MAX_TILES];
  static int bin_buf[MAX_BIN_ENTRIES];

  if (num_tiles > MAX_TILES)
    num_tiles = MAX_TILES; // safety clamp

  if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
    draw_shadow_map(world);
  }

  // Phase 1: transform all triangles -> screen_tri_t list
  int screen_triangles_count = build_screen_tris(world, screen_triangles);

  // Phase 2: bin triangles to tiles (two-pass prefix sum)
  compute_triangles_per_tile(tile_count_buf, num_tiles, screen_triangles_count,
                             screen_triangles, tiles_x, tiles_y);
  compute_tile_starts(tile_start_buf, tile_count_buf, num_tiles);
  bin_triangles_into_tiles(tile_count_buf, screen_triangles, bin_buf,
                           tile_start_buf, num_tiles, screen_triangles_count,
                           tiles_x, tiles_y);

  // Phase 3: render tiles in parallel
  draw_tiles_parallel(world, num_tiles, tile_count_buf, tile_start_buf, bin_buf,
                      screen_triangles, tiles_x);
}

void resize_renderer_to(world *world, int display_w, int display_h) {
  printf("INFO: resizing\n");
  renderer *renderer = world->renderer;
  int render_width = world->settings.render_width;

  renderer->display_width = display_w;
  renderer->display_height = display_h;

  int new_w = render_width;
  int new_h = (int)(render_width * ((float)display_h / (float)display_w));

  renderer->screen_width = new_w;
  renderer->screen_height = new_h;
  renderer->aspect_ratio = (float)new_w / (float)new_h;

  destroy_buffers(renderer);
  init_buffers(renderer);

  destroy_textures(renderer);
  init_textures(renderer);

  destroy_shaders(renderer);
  init_shaders(renderer);
}

void resize_renderer(world *world) {
  resize_renderer_to(world, GetScreenWidth(), GetScreenHeight());
}
