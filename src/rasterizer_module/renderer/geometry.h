#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../assets/mesh.h"
#include "../engine/cam.h"
#include "../math/vec.h"
#include "../world.h"
#include "renderer.h"

#define BB_PAD 0.0f
#define AREA_THRESHOLD 0.5f
#define EDGE_THRESHOLD 1e-2f

#define EPSILON 1e-2f

bool is_backfacing(const vec3f cam_space_verts[3]);

void compute_triangles_per_tile(
    int tile_count_buf[MAX_TILES], const int num_tiles,
    const int screen_triangles_count,
    const screen_tri_t screen_triangles[MAX_SCREEN_TRIS], const int tiles_x,
    const int tiles_y);

void compute_tile_starts(int tile_start_buf[MAX_TILES],
                         const int tile_count_buf[MAX_TILES],
                         const int num_tiles);

void bin_triangles_into_tiles(int tile_count_buf[MAX_TILES],
                              screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                              int bin_buf[MAX_BIN_ENTRIES],
                              const int tile_start_buf[MAX_TILES],
                              const int num_tiles,
                              const int screen_triangles_count,
                              const int tiles_x, const int tiles_y);

void transform_triangle_to_camera(const mesh *mesh_data, size_t triangle_index,
                                  const mesh_instance *inst, const cam *cam,
                                  vec3f out[3]);

int build_screen_tris(world *world,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS]);

void tile_coordinates_to_triangle_coordinates(
    const int tile_index, const int tiles_x, const int screen_width,
    const int screen_height, int *x0, int *x1, int *y0, int *y1);
