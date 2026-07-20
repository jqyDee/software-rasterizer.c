#include "shadow.h"

#include <float.h>
#include <math.h>
#include <omp.h>
#include <stddef.h>

#include "draw.h"
#include "geometry.h"
#include "tiling.h"
#include "transform.h"
#include "../testing.h"
#include "../math/rotation.h"
#include "../math/transformation.h"
#include <raylib.h>
#include <string.h>

#define MAX_THREADS 16
static screen_tri_t shadow_thread_local_tris[MAX_THREADS][MAX_SCREEN_TRIS];
static int shadow_thread_tri_counts[MAX_THREADS];

void compute_shadow_light_basis(world *world) {
  vec3f fwd = vec_scale(vec_normalize(world->lights[0].light_dir), -1.0f);
  vec3f up_hint = (fabsf(fwd.y) > 0.99f) ? (vec3f){0, 0, 1} : (vec3f){0, 1, 0};
  vec3f right = vec_normalize(vec_cross(up_hint, fwd));
  vec3f up = vec3f_cross(fwd, right);
  world->renderer->light_right = right;
  world->renderer->light_up = up;
  world->renderer->light_fwd = fwd;
}

void world_to_light_screen(const renderer *renderer, vec3f world_pos, float *sx,
                           float *sy, float *closeness) {
  float lx = vec_dot(world_pos, renderer->light_right);
  float ly = vec_dot(world_pos, renderer->light_up);
  float lz = vec_dot(world_pos, renderer->light_fwd);

  *sx = (lx / SHADOW_ORTHO_HALF_EXTENT + 1.0f) * 0.5f * SHADOW_MAP_SIZE;
  *sy = (1.0f - ly / SHADOW_ORTHO_HALF_EXTENT) * 0.5f * SHADOW_MAP_SIZE;
  *closeness = -lz;
}

float sample_shadow(const renderer *renderer, vec3f world_pos) {
  float sx, sy, closeness;
  world_to_light_screen(renderer, world_pos, &sx, &sy, &closeness);
  int ix = (int)sx, iy = (int)sy;
  if (ix < 0 || ix >= SHADOW_MAP_SIZE || iy < 0 || iy >= SHADOW_MAP_SIZE) {
    return 1.0f;
  }
  float stored = renderer->shadow_depthbuffer[iy * SHADOW_MAP_SIZE + ix];
  return (closeness < stored - SHADOW_BIAS) ? SHADOW_DARKNESS : 1.0f;
}

// depth-only variant of draw_tiles_parallel for the shadow map: same
// non-overlapping-tile parallelism, but only shadow_depthbuffer is written
// (framebuffer/idbuffer/albedobuffer/normalbuffer all NULL), and tiles are
// sized against SHADOW_MAP_SIZE instead of the main screen dimensions
void draw_shadow_tiles_parallel(renderer *r, const int num_tiles,
                                const int tile_count_buf[MAX_TILES],
                                const int tile_start_buf[MAX_TILES],
                                const int bin_buf[MAX_BIN_ENTRIES],
                                const screen_tri_t tris[MAX_SCREEN_TRIS],
                                const int tiles_x) {
#pragma omp parallel for schedule(dynamic)
  for (int tile_index = 0; tile_index < num_tiles; tile_index++) {
    if (tile_count_buf[tile_index] == 0)
      continue;

    int x0, x1, y0, y1;
    tile_coordinates_to_triangle_coordinates(
        tile_index, tiles_x, 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, &x0, &x1,
        &y0, &y1);
    int end = tile_start_buf[tile_index] + tile_count_buf[tile_index];

    // shadow pass isn't tracked by the raster fill/overdraw stats — local,
    // throwaway counters
    long _iter = 0, _edge = 0, _depth = 0;
    for (int bin_id = tile_start_buf[tile_index]; bin_id < end; bin_id++)
      draw_triangle_pixels_tiled(r->shadow_depthbuffer, NULL, NULL, NULL, NULL,
                                 SHADOW_MAP_SIZE, &tris[bin_buf[bin_id]], x0,
                                 y0, x1, y1, &_iter, &_edge, &_depth, false,
                                 false, false, false);
  }
}

INTERNAL_INLINE bool instance_potentially_shadowing(const mesh *m,
                                                    const mesh_instance *inst,
                                                    const renderer *r) {
  vec3f local_center = vec_scale(vec_add(m->aabb_min, m->aabb_max), 0.5f);
  vec3f half_extent = vec_scale(vec_sub(m->aabb_max, m->aabb_min), 0.5f);
  float max_scale = fmaxf(inst->scale.x, fmaxf(inst->scale.y, inst->scale.z));
  float radius =
      sqrtf(half_extent.x * half_extent.x + half_extent.y * half_extent.y +
            half_extent.z * half_extent.z) *
      max_scale;

  vec3f c = {local_center.x * inst->scale.x, local_center.y * inst->scale.y,
             local_center.z * inst->scale.z};
  c = rotate_z_snapped(c, inst->rotation.z * DEG_TO_RAD);
  c = rotate_x_snapped(c, inst->rotation.x * DEG_TO_RAD);
  c = rotate_y_snapped(c, inst->rotation.y * DEG_TO_RAD);
  c = vec_add(c, inst->pos);

  float lx = vec_dot(c, r->light_right);
  float ly = vec_dot(c, r->light_up);

  if (lx + radius < -SHADOW_ORTHO_HALF_EXTENT) return false;
  if (lx - radius > SHADOW_ORTHO_HALF_EXTENT) return false;
  if (ly + radius < -SHADOW_ORTHO_HALF_EXTENT) return false;
  if (ly - radius > SHADOW_ORTHO_HALF_EXTENT) return false;

  return true;
}

void draw_shadow_map(world *world) {
  renderer *r = world->renderer;
  compute_shadow_light_basis(world);
  // closeness (-lz) is signed (origin-centered projection), unlike inv_z which
  // is always positive — 0 is not a valid "infinitely far" sentinel here, use
  // -FLT_MAX so any real triangle wins the first depth test
  for (int i = 0; i < SHADOW_MAP_SIZE * SHADOW_MAP_SIZE; i++)
    r->shadow_depthbuffer[i] = -FLT_MAX;

  // static buffers: allocated once, reused every frame — separate from the
  // main pass's own tile buffers so the two passes never alias
  static screen_tri_t shadow_screen_tris[MAX_SCREEN_TRIS];
  static int shadow_tile_count_buf[MAX_TILES];
  static int shadow_tile_start_buf[MAX_TILES];
  static int shadow_bin_buf[MAX_BIN_ENTRIES];

  int shadow_tris_count = 0;

  memset(shadow_thread_tri_counts, 0, sizeof(shadow_thread_tri_counts));

  // Phase A: transform all triangles into light-space screen_tri_t entries
#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int local_count = 0;
    screen_tri_t *local_buf = shadow_thread_local_tris[tid];

#pragma omp for schedule(dynamic)
    for (size_t instance_id = 0; instance_id < world->instance_count;
         instance_id++) {
      const mesh_instance *instance = &world->instances[instance_id];
      const mesh *mesh = &world->mesh_data[instance->mesh_idx];

      if (!instance_potentially_shadowing(mesh, instance, r))
        continue;

      transform_basis basis;
      compute_instance_basis(instance, &basis);

      for (size_t vertex_id = 0; vertex_id + 2 < mesh->vertex_count &&
                                 local_count < MAX_SCREEN_TRIS;
           vertex_id += 3) {
        vec3f v_world[3];
        transform_triangle_with_basis(mesh, vertex_id, &basis, v_world);

        screen_tri_t st = {0};
        for (int j = 0; j < 3; j++) {
          float sx, sy, closeness;
          world_to_light_screen(r, v_world[j], &sx, &sy, &closeness);
          st.v[j] = (vec3f){sx, sy, 0};
          st.inv_z[j] = closeness;
        }
        st.tex = NULL;

        // reject near-degenerate light-space projections: without this,
        // triangles at grazing angles to the light (routine as the light
        // orbits) can drive setup_edge_walk's 1/signed_area toward infinity,
        // corrupting large regions of the shadow map
        if (edge_collapsed(st.v) || area_collapsed(st.v))
          continue;

        compute_bbox(st.v, 0, 0, SHADOW_MAP_SIZE - 1, SHADOW_MAP_SIZE - 1,
                     &st.bx0, &st.bx1, &st.by0, &st.by1);
        if (bbox_collapsed(&st.bx0, &st.bx1, &st.by0, &st.by1))
          continue;

        local_buf[local_count++] = st;
      }
    }
    shadow_thread_tri_counts[tid] = local_count;
  }

  int num_threads = omp_get_max_threads();
  if (num_threads > MAX_THREADS) {
    num_threads = MAX_THREADS;
  }
  int thread_offsets[MAX_THREADS];

  for (int t = 0; t < num_threads; t++) {
    thread_offsets[t] = shadow_tris_count;
    shadow_tris_count += shadow_thread_tri_counts[t];
  }

  if (shadow_tris_count > MAX_SCREEN_TRIS) {
    shadow_tris_count = MAX_SCREEN_TRIS;
  }

#pragma omp parallel for schedule(static)
  for (int t = 0; t < num_threads; t++) {
    int start = thread_offsets[t];
    int count = shadow_thread_tri_counts[t];

    if (start + count > MAX_SCREEN_TRIS) {
      count = MAX_SCREEN_TRIS - start;
    }
    if (count > 0) {
      memcpy(&shadow_screen_tris[start], shadow_thread_local_tris[t],
             count * sizeof(screen_tri_t));
    }
  }

  // Phase B: bin into the shadow map's own tile grid, render tiles in
  // parallel — identical machinery to the main render() pass, just sized
  // for SHADOW_MAP_SIZE instead of screen_width/screen_height
  int shadow_tiles_x = (SHADOW_MAP_SIZE + TILE_SIZE - 1) / TILE_SIZE;
  int shadow_tiles_y = shadow_tiles_x; // square map
  int num_shadow_tiles = shadow_tiles_x * shadow_tiles_y;

  compute_triangles_per_tile(shadow_tile_count_buf, num_shadow_tiles,
                             shadow_tris_count, shadow_screen_tris, 0, 0,
                             shadow_tiles_x, shadow_tiles_y);
  compute_tile_starts(shadow_tile_start_buf, shadow_tile_count_buf,
                      num_shadow_tiles);
  bin_triangles_into_tiles(shadow_tile_count_buf, shadow_screen_tris,
                           shadow_bin_buf, shadow_tile_start_buf,
                           num_shadow_tiles, shadow_tris_count, 0, 0,
                           shadow_tiles_x, shadow_tiles_y);

  draw_shadow_tiles_parallel(r, num_shadow_tiles, shadow_tile_count_buf,
                             shadow_tile_start_buf, shadow_bin_buf,
                             shadow_screen_tris, shadow_tiles_x);
}
