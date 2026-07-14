#include "tiling.h"

#include <string.h>

#include "../testing.h"

INTERNAL void triangle_coordinates_to_tile_coordinates(
    const screen_tri_t *screen_triangle, const int tiles_x, const int tiles_y,
    int *tx0, int *tx1, int *ty0, int *ty1) {
  // convert tri bbox to tile coordinates
  *tx0 = screen_triangle->bx0 / TILE_SIZE;
  *ty0 = screen_triangle->by0 / TILE_SIZE;
  *tx1 = screen_triangle->bx1 / TILE_SIZE,
  *ty1 = screen_triangle->by1 / TILE_SIZE;

  // clamp to valid range
  if (*tx1 >= tiles_x) {
    *tx1 = tiles_x - 1;
  }
  if (*ty1 >= tiles_y) {
    *ty1 = tiles_y - 1;
  }
}

void compute_triangles_per_tile(
    int tile_count_buf[MAX_TILES], const int num_tiles,
    const int screen_triangles_count,
    const screen_tri_t screen_triangles[MAX_SCREEN_TRIS], const int tiles_x,
    const int tiles_y) {
  memset(tile_count_buf, 0, (size_t)num_tiles * sizeof(int));
  for (int screen_triangle_id = 0; screen_triangle_id < screen_triangles_count;
       screen_triangle_id++) {
    int tx0, tx1, ty0, ty1;
    triangle_coordinates_to_tile_coordinates(
        &screen_triangles[screen_triangle_id], tiles_x, tiles_y, &tx0, &tx1,
        &ty0, &ty1);

    // compute the num of triangles for each tile
    for (int ty = ty0; ty <= ty1; ty++)
      for (int tx = tx0; tx <= tx1; tx++)
        tile_count_buf[ty * tiles_x + tx]++;
  }
}

void compute_tile_starts(int tile_start_buf[MAX_TILES],
                         const int tile_count_buf[MAX_TILES],
                         const int num_tiles) {
  int total_bin = 0;
  for (int tile_index = 0; tile_index < num_tiles; tile_index++) {
    tile_start_buf[tile_index] = total_bin;
    total_bin += tile_count_buf[tile_index];
    if (total_bin > MAX_BIN_ENTRIES) {
      total_bin = MAX_BIN_ENTRIES;
      break;
    }
  }
}

void bin_triangles_into_tiles(int tile_count_buf[MAX_TILES],
                              screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                              int bin_buf[MAX_BIN_ENTRIES],
                              const int tile_start_buf[MAX_TILES],
                              const int num_tiles,
                              const int screen_triangles_count,
                              const int tiles_x, const int tiles_y) {
  memset(tile_count_buf, 0, (size_t)num_tiles * sizeof(int));
  for (int screen_triangle_id = 0; screen_triangle_id < screen_triangles_count;
       screen_triangle_id++) {
    int tx0, tx1, ty0, ty1;
    triangle_coordinates_to_tile_coordinates(
        &screen_triangles[screen_triangle_id], tiles_x, tiles_y, &tx0, &tx1,
        &ty0, &ty1);

    for (int ty = ty0; ty <= ty1; ty++) {
      for (int tx = tx0; tx <= tx1; tx++) {
        int tile_id = ty * tiles_x + tx;
        int idx = tile_start_buf[tile_id] + tile_count_buf[tile_id];
        if (idx < MAX_BIN_ENTRIES) {
          bin_buf[idx] = screen_triangle_id;
          tile_count_buf[tile_id]++;
        }
      }
    }
  }
}

void tile_coordinates_to_triangle_coordinates(const int tile_index,
                                              const int tiles_x,
                                              const int screen_width,
                                              const int screen_height, int *x0,
                                              int *x1, int *y0, int *y1) {
  int x = tile_index % tiles_x, y = tile_index / tiles_x;

  *x0 = x * TILE_SIZE;
  *y0 = y * TILE_SIZE;
  *x1 = *x0 + TILE_SIZE - 1;

  if (*x1 >= screen_width) {
    *x1 = screen_width - 1;
  }

  *y1 = *y0 + TILE_SIZE - 1;
  if (*y1 >= screen_height) {
    *y1 = screen_height - 1;
  }
}
