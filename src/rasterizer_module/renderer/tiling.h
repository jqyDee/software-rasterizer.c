#pragma once

#include "renderer.h"

void compute_triangles_per_tile(
    int tile_count_buf[MAX_TILES], const int num_tiles,
    const int screen_triangles_count,
    const screen_tri_t screen_triangles[MAX_SCREEN_TRIS], const int vp_x0,
    const int vp_y0, const int tiles_x, const int tiles_y);

void compute_tile_starts(int tile_start_buf[MAX_TILES],
                         const int tile_count_buf[MAX_TILES],
                         const int num_tiles);

void bin_triangles_into_tiles(int tile_count_buf[MAX_TILES],
                              screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                              int bin_buf[MAX_BIN_ENTRIES],
                              const int tile_start_buf[MAX_TILES],
                              const int num_tiles,
                              const int screen_triangles_count,
                              const int vp_x0, const int vp_y0,
                              const int tiles_x, const int tiles_y);

void tile_coordinates_to_triangle_coordinates(const int tile_index,
                                              const int tiles_x,
                                              const int vp_x0, const int vp_y0,
                                              const int vp_w, const int vp_h,
                                              int *x0, int *x1, int *y0,
                                              int *y1);
