#pragma once

#include "../world.h"
#include "raylib.h"

// iter/edge_pass/depth_pass are accumulated into (+=), not overwritten —
// caller resets once per frame. iter = pixels tested against the triangle's
// edge equations (bbox-in-tile area, regardless of coverage), edge_pass =
// pixels actually inside the triangle, depth_pass = pixels that also won
// the depth test (real overdraw vs wasted edge-test area).
// skip_texture/skip_normalbuffer/skip_albedobuffer are raster cost-isolation
// toggles (world->settings.debug_skip_*) — each disables one per-fragment
// shading component so its cost can be attributed by elimination via the
// existing raster ms stat, without per-pixel timers.
void draw_triangle_pixels_tiled(depthbuffer *depthbuffer,
                                framebuffer *framebuffer, idbuffer *idbuffer,
                                albedobuffer *albedobuffer,
                                normalbuffer *normalbuffer, int screen_width,
                                const screen_tri_t *st, int clip_x0,
                                int clip_y0, int clip_x1, int clip_y1,
                                long *iter, long *edge_pass, long *depth_pass,
                                bool skip_texture, bool skip_normalbuffer,
                                bool skip_albedobuffer, bool skip_framebuffer);

void draw_tiles_parallel(world *world, const viewport *vp, const int num_tiles,
                         const int tile_count_buf[MAX_TILES],
                         const int tile_start_buf[MAX_TILES],
                         const int bin_buf[MAX_BIN_ENTRIES],
                         const screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                         const int tiles_x, long *iter, long *edge_pass,
                         long *depth_pass);
