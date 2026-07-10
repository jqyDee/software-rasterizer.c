#include "renderer.h"

#include <stdlib.h>

#include "../world.h"
#include "draw.h"
#include "geometry.h"
#include "../engine/texture.h"

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

  // Phase 4: debug normal arrows (single-threaded, only when enabled)
  draw_normals(world);
}

void resize_renderer_to(world *world, int display_w, int display_h) {
  renderer *renderer = world->renderer;
  int render_width = world->settings.render_width;

  renderer->display_width = display_w;
  renderer->display_height = display_h;

  int new_w = render_width;
  int new_h = (int)(render_width * ((float)display_h / (float)display_w));

  renderer->screen_width = new_w;
  renderer->screen_height = new_h;
  renderer->aspect_ratio = (float)new_w / (float)new_h;

  free(renderer->framebuffer);
  free(renderer->depthbuffer);
  free(renderer->idbuffer);
  renderer->framebuffer = malloc(new_w * new_h * sizeof(Color));
  if (!renderer->framebuffer)
    exit(2);
  renderer->depthbuffer = malloc(new_w * new_h * sizeof(float));
  if (!renderer->depthbuffer)
    exit(2);
  renderer->idbuffer = malloc(new_w * new_h * sizeof(int));
  if (!renderer->idbuffer)
    exit(2);

  UnloadTexture(renderer->screen_texture);
  init_texture(renderer);
}

void resize_renderer(world *world) {
  resize_renderer_to(world, GetScreenWidth(), GetScreenHeight());
}
