#pragma once

#include "../world.h"
#include "raylib.h"

void draw_triangle_pixels_tiled(float *depthbuffer, Color *framebuffer,
                                int *idbuffer, int screen_width,
                                const screen_tri_t *st, int clip_x0,
                                int clip_y0, int clip_x1, int clip_y1);

void draw_tiles_parallel(world *world, const int num_tiles,
                         const int tile_count_buf[MAX_TILES],
                         const int tile_start_buf[MAX_TILES],
                         const int bin_buf[MAX_BIN_ENTRIES],
                         const screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                         const int tiles_x);

void draw_normals(world *world);

void draw_line_fb(renderer *r, int x0, int y0, int x1, int y1, Color c);
