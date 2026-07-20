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
#include <raylib.h>

// rasterizes one viewport's worth of geometry (transform -> bin -> draw).
// screen_triangles/tile_*_buf/bin_buf are reused across viewports: each call
// fully overwrites them before draw_tiles_parallel reads them back, so
// there's no cross-viewport aliasing even though the buffers are static.
INTERNAL_INLINE void render_viewport(world *world, const viewport *vp,
                                     int viewport_index,
                                     screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                                     int tile_count_buf[MAX_TILES],
                                     int tile_start_buf[MAX_TILES],
                                     int bin_buf[MAX_BIN_ENTRIES],
                                     double *t_geom, double *t_geom_xform,
                                     double *t_bin,
                                     double *t_raster, long *r_iter,
                                     long *r_edge_pass, long *r_depth_pass) {
  int tiles_x = (vp->w + TILE_SIZE - 1) / TILE_SIZE;
  int tiles_y = (vp->h + TILE_SIZE - 1) / TILE_SIZE;

  int num_tiles = tiles_x * tiles_y;
  if (num_tiles > MAX_TILES)
    num_tiles = MAX_TILES; // safety clamp

  double t0 = GetTime();
  int screen_triangles_count =
      build_screen_tris(world, vp, viewport_index, screen_triangles, t_geom_xform);
  *t_geom += (GetTime() - t0);

  t0 = GetTime();
  compute_triangles_per_tile(tile_count_buf, num_tiles, screen_triangles_count,
                             screen_triangles, vp->x, vp->y, tiles_x, tiles_y);
  compute_tile_starts(tile_start_buf, tile_count_buf, num_tiles);
  bin_triangles_into_tiles(tile_count_buf, screen_triangles, bin_buf,
                           tile_start_buf, num_tiles, screen_triangles_count,
                           vp->x, vp->y, tiles_x, tiles_y);
  *t_bin += (GetTime() - t0);

  t0 = GetTime();
  draw_tiles_parallel(world, vp, num_tiles, tile_count_buf, tile_start_buf,
                      bin_buf, screen_triangles, tiles_x, r_iter, r_edge_pass,
                      r_depth_pass);
  *t_raster += (GetTime() - t0);
}

void render(world *world) {
  // static buffers: allocated once, reused every frame (and every viewport)
  static screen_tri_t screen_triangles[MAX_SCREEN_TRIS];
  static int tile_count_buf[MAX_TILES];
  static int tile_start_buf[MAX_TILES];
  static int bin_buf[MAX_BIN_ENTRIES];

  double t_geom = 0.0, t_geom_xform = 0.0, t_bin = 0.0, t_raster = 0.0,
         t_shadow = 0.0;
  long r_iter = 0, r_edge_pass = 0, r_depth_pass = 0;

  reset_world_vertex_cache(world);

  if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
    // light-space, camera-independent — one shadow pass covers every
    // viewport, must not move inside the per-kart loop below
    double t0 = GetTime();
    draw_shadow_map(world);
    t_shadow += (GetTime() - t0);
  }

  if (world->settings.show_debug_gui || world->kart_count == 0) {
    // free-fly debug cam (or no karts yet): single full-frame pass through
    // whatever world->cam currently points at, split-screen disabled
    viewport vp = viewport_full_frame(world->renderer);
    render_viewport(world, &vp, 0, screen_triangles, tile_count_buf,
                    tile_start_buf, bin_buf, &t_geom, &t_geom_xform, &t_bin,
                    &t_raster, &r_iter, &r_edge_pass, &r_depth_pass);
  } else {
    cam *saved_cam = world->cam;
    for (size_t i = 0; i < world->kart_count; i++) {
      viewport vp = compute_kart_viewport((int)i, (int)world->kart_count,
                                          world->renderer->screen_width,
                                          world->renderer->screen_height);
      world->cam = &world->game_cams[i];
      render_viewport(world, &vp, (int)i, screen_triangles, tile_count_buf,
                      tile_start_buf, bin_buf, &t_geom, &t_geom_xform, &t_bin,
                      &t_raster, &r_iter, &r_edge_pass, &r_depth_pass);
    }
    world->cam = saved_cam;
  }

  perf_record(&world->perf.metrics[PERF_RENDER_GEOM], (float)(t_geom * 1000.0));
  perf_record(&world->perf.metrics[PERF_RENDER_GEOM_XFORM],
             (float)(t_geom_xform * 1000.0));
  perf_record(&world->perf.metrics[PERF_RENDER_BINNING], (float)(t_bin * 1000.0));
  perf_record(&world->perf.metrics[PERF_RENDER_RASTER], (float)(t_raster * 1000.0));
  perf_record(&world->perf.metrics[PERF_RASTER_ITER], (float)r_iter);
  perf_record(&world->perf.metrics[PERF_RASTER_EDGE_PASS], (float)r_edge_pass);
  perf_record(&world->perf.metrics[PERF_RASTER_DEPTH_PASS], (float)r_depth_pass);
  if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
    perf_record(&world->perf.metrics[PERF_RENDER_SHADOW], (float)(t_shadow * 1000.0));
  }
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

viewport viewport_full_frame(const renderer *renderer) {
  return (viewport){
      .x = 0,
      .y = 0,
      .w = renderer->screen_width,
      .h = renderer->screen_height,
      .aspect_ratio = renderer->aspect_ratio,
  };
}

// splits the framebuffer into up to MAX_KART_COUNT regions, one per local
// player — layout follows the usual racing-game convention (top/bottom for
// 2P, quad for 4P) rather than an even N-way grid
viewport compute_kart_viewport(int index, int kart_count, int screen_w,
                               int screen_h) {
  int half_w = screen_w / 2;
  int half_h = screen_h / 2;
  viewport vp;

  switch (kart_count) {
  case 2:
    vp = (index == 0) ? (viewport){0, 0, screen_w, half_h}
                      : (viewport){0, half_h, screen_w, screen_h - half_h};
    break;
  case 3:
    if (index == 0)
      vp = (viewport){0, 0, half_w, half_h};
    else if (index == 1)
      vp = (viewport){half_w, 0, screen_w - half_w, half_h};
    else
      vp = (viewport){0, half_h, screen_w, screen_h - half_h};
    break;
  case 4: {
    int col = index % 2, row = index / 2;
    int x0 = col == 0 ? 0 : half_w;
    int y0 = row == 0 ? 0 : half_h;
    int w = col == 0 ? half_w : screen_w - half_w;
    int h = row == 0 ? half_h : screen_h - half_h;
    vp = (viewport){x0, y0, w, h};
    break;
  }
  default: // 1 (or anything unexpected) -> fullscreen
    vp = (viewport){0, 0, screen_w, screen_h};
    break;
  }

  vp.aspect_ratio = (float)vp.w / (float)vp.h;
  return vp;
}
