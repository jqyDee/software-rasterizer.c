#include "../../renderer/geometry.h"
#include "../../renderer/tiling.h"
#include "../unity.h"
#include <string.h>

extern void package_screen_tri(screen_tri_t *screen_triangle,
                               const vec3f proj[3], const vec2f uvs[3],
                               const vec3f normals[3],
                               const mesh_instance *instance,
                               const bool has_tex, const Color flat_color,
                               const vec3f light_rgb, const int instance_id,
                               const int bx0, const int bx1, const int by0,
                               const int by1, const float st_shadow[3]);
extern bool area_collapsed(const vec3f proj[3]);
extern bool edge_collapsed(const vec3f proj[3]);
extern bool bbox_collapsed(const int *bx0, const int *bx1,
                                  const int *by0, const int *by1);
extern int clip_triangle_near_plane_uv(const vec3f verts[3],
                                       const vec2f uvs_in[3],
                                       const vec3f normals_in[3],
                                       const float shadow_in[3], float near,
                                       vec3f out_pos[4], vec2f out_uvs[4],
                                       vec3f out_normals[4],
                                       float out_shadow[4]);
extern void compute_bbox(const vec3f proj[3], const int screen_width,
                         const int screen_height, int *bx0, int *bx1, int *by0,
                         int *by1);
extern void get_uvs(const vec2f *uvs, size_t triangle_id, vec2f out[3]);
extern void triangle_coordinates_to_tile_coordinates(
    const screen_tri_t *screen_triangle, const int tiles_x, const int tiles_y,
    int *tx0, int *tx1, int *ty0, int *ty1);
extern float compute_fp_edge_bias(const vec3f proj[3], const int screen_width,
                           const int screen_height, const float seam_bias);

void setUp(void) {}
void tearDown(void) {}

/* ---- clip_triangle_near_plane_uv ---- */

void test_clip_all_behind_returns_zero(void) {
  vec3f verts[3] = {{0, 0, 0.5f}, {1, 0, 0.5f}, {0, 1, 0.5f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  TEST_ASSERT_EQUAL_INT(0, n);
}

void test_clip_all_in_front_returns_three(void) {
  vec3f verts[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  TEST_ASSERT_EQUAL_INT(3, n);
}

void test_clip_one_behind_returns_four(void) {
  vec3f verts[3] = {{0, 0, 0.5f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  TEST_ASSERT_EQUAL_INT(4, n);
}

void test_clip_two_behind_returns_three(void) {
  vec3f verts[3] = {{0, 0, 0.5f}, {1, 0, 0.5f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  TEST_ASSERT_EQUAL_INT(3, n);
}

void test_clip_clipped_vertex_is_at_near_plane(void) {
  vec3f verts[3] = {{0, 0, 0.5f}, {2, 0, 2.0f}, {0, 2, 2.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f, out_pos,
                              out_uvs, out_normals, out_shadow);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, out_pos[0].z);
}

void test_clip_all_outputs_in_front_of_near(void) {
  vec3f verts[3] = {{0, 0, 0.5f}, {2, 0, 2.0f}, {0, 2, 2.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  for (int i = 0; i < n; i++)
    TEST_ASSERT_TRUE(out_pos[i].z >= 1.0f - 1e-5f);
}

void test_clip_uv_lerped_at_clip_point(void) {
  /* v0 at z=0 (behind near=1), v1 at z=3, v2 at z=3
     edge v0→v1: clip_t = (1-0)/(3-0) = 1/3 → uv lerp of (0,0)→(1,0) = (1/3, 0)
   */
  vec3f verts[3] = {{0, 0, 0.0f}, {3, 0, 3.0f}, {0, 3, 3.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  TEST_ASSERT_EQUAL_INT(4, n);
  /* find the clipped vertex on the v0→v1 edge (z=1, y≈0) */
  int found = 0;
  for (int i = 0; i < n; i++) {
    if (fabsf(out_pos[i].z - 1.0f) < 1e-4f && fabsf(out_pos[i].y) < 1e-4f) {
      TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f / 3.0f, out_uvs[i].x);
      found = 1;
    }
  }
  TEST_ASSERT_TRUE(found);
}

void test_clip_normal_lerped_at_clip_point(void) {
  /* both endpoints share the same normal (a flat face) — a lerp must return
     that same normal at the clip point. This is a regression test: a buggy
     interpolation that multiplies instead of adds collapses to (0,0,0) here,
     since (n_curr - n_prev) is zero when both endpoints match. */
  vec3f verts[3] = {{0, 0, 0.0f}, {3, 0, 3.0f}, {0, 3, 3.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  vec3f out_pos[4];
  vec2f out_uvs[4];
  vec3f out_normals[4];
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  float out_shadow[4];
  int n = clip_triangle_near_plane_uv(verts, uvs, normals, shadow, 1.0f,
                                      out_pos, out_uvs, out_normals,
                                      out_shadow);
  int found = 0;
  for (int i = 0; i < n; i++) {
    if (fabsf(out_pos[i].z - 1.0f) < 1e-4f && fabsf(out_pos[i].y) < 1e-4f) {
      TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out_normals[i].x);
      TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out_normals[i].y);
      TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, out_normals[i].z);
      found = 1;
    }
  }
  TEST_ASSERT_TRUE(found);
}

/* ---- is_backfacing ---- */

void test_is_backfacing_true_for_away_facing(void) {
  /* normal = +Z, view_dir = -Z → dot < 0 → backfacing */
  vec3f verts[3] = {{-1, 0, 5}, {1, 0, 5}, {0, 1, 5}};
  vec3f normal = {0, 0, 1};
  TEST_ASSERT_TRUE(is_backfacing(verts, normal));
}

void test_is_backfacing_false_for_front_facing(void) {
  /* normal = -Z, view_dir = -Z → dot > 0 → front facing */
  vec3f verts[3] = {{1, 0, 5}, {-1, 0, 5}, {0, 1, 5}};
  vec3f normal = {0, 0, -1};
  TEST_ASSERT_FALSE(is_backfacing(verts, normal));
}

/* ---- compute_face_normal ---- */

void test_compute_face_normal_matches_expected_axis(void) {
  /* CCW-in-screen-space triangle in the z=5 plane should produce a normal
     purely along +/-Z */
  vec3f verts[3] = {{-1, 0, 5}, {1, 0, 5}, {0, 1, 5}};
  vec3f normal = compute_face_normal(verts);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, normal.x);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, normal.y);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, fabsf(normal.z));
}

/* ---- compute_bbox ---- */

void test_compute_bbox_basic(void) {
  vec3f proj[3] = {{10, 20, 1}, {30, 5, 1}, {20, 40, 1}};
  int bx0, by0, bx1, by1;
  compute_bbox(proj, 800, 600, &bx0, &bx1, &by0, &by1);
  TEST_ASSERT_EQUAL_INT(10, bx0);
  TEST_ASSERT_EQUAL_INT(5, by0);
  TEST_ASSERT_EQUAL_INT(30, bx1);
  TEST_ASSERT_EQUAL_INT(40, by1);
}

void test_compute_bbox_clamped_to_screen(void) {
  vec3f proj[3] = {{-10, -5, 1}, {900, 0, 1}, {400, 700, 1}};
  int bx0, by0, bx1, by1;
  compute_bbox(proj, 800, 600, &bx0, &bx1, &by0, &by1);
  TEST_ASSERT_EQUAL_INT(0, bx0);
  TEST_ASSERT_EQUAL_INT(0, by0);
  TEST_ASSERT_EQUAL_INT(799, bx1);
  TEST_ASSERT_EQUAL_INT(599, by1);
}

/* ---- bbox_collapsed ---- */

void test_bbox_collapsed_zero_width(void) {
  int bx0 = 5, bx1 = 3, by0 = 0, by1 = 10;
  TEST_ASSERT_TRUE(bbox_collapsed(&bx0, &bx1, &by0, &by1));
}

void test_bbox_collapsed_zero_height(void) {
  int bx0 = 0, bx1 = 10, by0 = 8, by1 = 3;
  TEST_ASSERT_TRUE(bbox_collapsed(&bx0, &bx1, &by0, &by1));
}

void test_bbox_not_collapsed(void) {
  int bx0 = 0, bx1 = 10, by0 = 0, by1 = 10;
  TEST_ASSERT_FALSE(bbox_collapsed(&bx0, &bx1, &by0, &by1));
}

/* ---- get_uvs ---- */

void test_get_uvs_copies_correct_slice(void) {
  vec2f uvs[6] = {{0, 0}, {1, 0}, {0, 1}, {0.5f, 0}, {1, 1}, {0, 0.5f}};
  vec2f out[3] = {0};
  get_uvs(uvs, 3, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, out[1].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, out[2].y);
}

void test_get_uvs_null_no_crash(void) {
  vec2f out[3] = {{9, 9}, {9, 9}, {9, 9}};
  get_uvs(NULL, 0, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 9.0f, out[0].x); /* unchanged */
}

/* ---- edge_collapsed ---- */

void test_edge_collapsed_identical_verts(void) {
  vec3f proj[3] = {{10, 10, 1}, {10, 10, 1}, {20, 30, 1}};
  TEST_ASSERT_TRUE(edge_collapsed(proj));
}

void test_edge_not_collapsed_normal_triangle(void) {
  vec3f proj[3] = {{0, 0, 1}, {100, 0, 1}, {50, 100, 1}};
  TEST_ASSERT_FALSE(edge_collapsed(proj));
}

/* ---- area_collapsed ---- */

void test_area_collapsed_collinear(void) {
  vec3f proj[3] = {{0, 0, 1}, {100, 0, 1}, {200, 0, 1}};
  TEST_ASSERT_TRUE(area_collapsed(proj));
}

void test_area_not_collapsed_normal_triangle(void) {
  vec3f proj[3] = {{0, 0, 1}, {100, 0, 1}, {50, 100, 1}};
  TEST_ASSERT_FALSE(area_collapsed(proj));
}

/* ---- tile_coordinates_to_triangle_coordinates ---- */

void test_tile_coords_origin_tile(void) {
  int x0, x1, y0, y1;
  tile_coordinates_to_triangle_coordinates(0, 10, 320, 240, &x0, &x1, &y0,
                                           &y1);
  TEST_ASSERT_EQUAL_INT(0, x0);
  TEST_ASSERT_EQUAL_INT(0, y0);
  TEST_ASSERT_EQUAL_INT(TILE_SIZE - 1, x1);
  TEST_ASSERT_EQUAL_INT(TILE_SIZE - 1, y1);
}

void test_tile_coords_second_tile_x(void) {
  int x0, x1, y0, y1;
  tile_coordinates_to_triangle_coordinates(1, 10, 320, 240, &x0, &x1, &y0,
                                           &y1);
  TEST_ASSERT_EQUAL_INT(TILE_SIZE, x0);
  TEST_ASSERT_EQUAL_INT(0, y0);
}

void test_tile_coords_second_row(void) {
  int x0, x1, y0, y1;
  /* tiles_x=2 → tile index 2 = row 1, col 0 */
  tile_coordinates_to_triangle_coordinates(2, 2, 320, 240, &x0, &x1, &y0,
                                           &y1);
  TEST_ASSERT_EQUAL_INT(0, x0);
  TEST_ASSERT_EQUAL_INT(TILE_SIZE, y0);
}

void test_tile_coords_clamps_at_screen_edge(void) {
  /* screen 33px wide: tile 1 x1 would be 63, clamped to 32 */
  int x0, x1, y0, y1;
  tile_coordinates_to_triangle_coordinates(1, 2, 33, 32, &x0, &x1, &y0, &y1);
  TEST_ASSERT_EQUAL_INT(32, x1);
}

/* ---- compute_fp_edge_bias ---- */

void test_fp_bias_nonnegative(void) {
  vec3f proj[3] = {{10, 10, 2}, {200, 50, 3}, {100, 200, 4}};
  float bias = compute_fp_edge_bias(proj, 800, 600, 0.0f);
  TEST_ASSERT_TRUE(bias >= 0.0f);
}

void test_fp_bias_seam_bias_added(void) {
  vec3f proj[3] = {{10, 10, 2}, {200, 50, 3}, {100, 200, 4}};
  float b0 = compute_fp_edge_bias(proj, 800, 600, 0.0f);
  float b1 = compute_fp_edge_bias(proj, 800, 600, 0.5f);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, b0 + 0.5f, b1);
}

/* ---- compute_tile_starts (prefix sum) ---- */

void test_tile_starts_basic(void) {
  int counts[4] = {3, 1, 5, 2};
  int starts[4] = {0};
  compute_tile_starts(starts, counts, 4);
  TEST_ASSERT_EQUAL_INT(0, starts[0]);
  TEST_ASSERT_EQUAL_INT(3, starts[1]);
  TEST_ASSERT_EQUAL_INT(4, starts[2]);
  TEST_ASSERT_EQUAL_INT(9, starts[3]);
}

void test_tile_starts_with_empty_tiles(void) {
  int counts[4] = {0, 2, 0, 3};
  int starts[4] = {0};
  compute_tile_starts(starts, counts, 4);
  TEST_ASSERT_EQUAL_INT(0, starts[0]);
  TEST_ASSERT_EQUAL_INT(0, starts[1]);
  TEST_ASSERT_EQUAL_INT(2, starts[2]);
  TEST_ASSERT_EQUAL_INT(2, starts[3]);
}

void test_tile_starts_all_zero(void) {
  int counts[4] = {0, 0, 0, 0};
  int starts[4] = {0};
  compute_tile_starts(starts, counts, 4);
  TEST_ASSERT_EQUAL_INT(0, starts[0]);
  TEST_ASSERT_EQUAL_INT(0, starts[3]);
}

/* ---- compute_triangles_per_tile ---- */

static screen_tri_t make_tri(int bx0, int by0, int bx1, int by1) {
  screen_tri_t t;
  memset(&t, 0, sizeof(t));
  t.bx0 = bx0;
  t.by0 = by0;
  t.bx1 = bx1;
  t.by1 = by1;
  return t;
}

void test_count_single_tri_single_tile(void) {
  screen_tri_t tris[1] = {make_tri(0, 0, TILE_SIZE - 1, TILE_SIZE - 1)};
  int counts[4] = {0};
  compute_triangles_per_tile(counts, 4, 1, tris, 2, 2);
  TEST_ASSERT_EQUAL_INT(1, counts[0]);
  TEST_ASSERT_EQUAL_INT(0, counts[1]);
  TEST_ASSERT_EQUAL_INT(0, counts[2]);
  TEST_ASSERT_EQUAL_INT(0, counts[3]);
}

void test_count_tri_spanning_all_tiles(void) {
  screen_tri_t tris[1] = {make_tri(0, 0, TILE_SIZE * 2 - 1, TILE_SIZE * 2 - 1)};
  int counts[4] = {0};
  compute_triangles_per_tile(counts, 4, 1, tris, 2, 2);
  TEST_ASSERT_EQUAL_INT(1, counts[0]);
  TEST_ASSERT_EQUAL_INT(1, counts[1]);
  TEST_ASSERT_EQUAL_INT(1, counts[2]);
  TEST_ASSERT_EQUAL_INT(1, counts[3]);
}

void test_count_two_tris_different_tiles(void) {
  screen_tri_t tris[2] = {
      make_tri(0, 0, TILE_SIZE - 1, TILE_SIZE - 1),
      make_tri(TILE_SIZE, 0, TILE_SIZE * 2 - 1, TILE_SIZE - 1),
  };
  int counts[4] = {0};
  compute_triangles_per_tile(counts, 4, 2, tris, 2, 2);
  TEST_ASSERT_EQUAL_INT(1, counts[0]); /* tile (0,0) */
  TEST_ASSERT_EQUAL_INT(1, counts[1]); /* tile (1,0) */
  TEST_ASSERT_EQUAL_INT(0, counts[2]);
  TEST_ASSERT_EQUAL_INT(0, counts[3]);
}

/* ---- bin_triangles_into_tiles ---- */

void test_bin_single_tri_lands_in_tile0(void) {
  screen_tri_t tris[1] = {make_tri(0, 0, TILE_SIZE - 1, TILE_SIZE - 1)};
  int counts[4] = {0};
  int starts[4] = {0};
  int bin[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
  compute_triangles_per_tile(counts, 4, 1, tris, 2, 2);
  compute_tile_starts(starts, counts, 4);
  bin_triangles_into_tiles(counts, tris, bin, starts, 4, 1, 2, 2);
  TEST_ASSERT_EQUAL_INT(0, bin[0]); /* tile 0 → triangle index 0 */
}

void test_bin_tri_spanning_all_tiles_appears_in_each(void) {
  screen_tri_t tris[1] = {make_tri(0, 0, TILE_SIZE * 2 - 1, TILE_SIZE * 2 - 1)};
  int counts[4] = {0};
  int starts[4] = {0};
  int bin[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
  compute_triangles_per_tile(counts, 4, 1, tris, 2, 2);
  compute_tile_starts(starts, counts, 4);
  bin_triangles_into_tiles(counts, tris, bin, starts, 4, 1, 2, 2);
  /* each tile has 1 entry pointing at triangle 0 */
  for (int ti = 0; ti < 4; ti++)
    TEST_ASSERT_EQUAL_INT(0, bin[starts[ti]]);
}

void test_bin_two_tris_correct_slots(void) {
  screen_tri_t tris[2] = {
      make_tri(0, 0, TILE_SIZE - 1, TILE_SIZE - 1),
      make_tri(TILE_SIZE, 0, TILE_SIZE * 2 - 1, TILE_SIZE - 1),
  };
  int counts[4] = {0};
  int starts[4] = {0};
  int bin[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
  compute_triangles_per_tile(counts, 4, 2, tris, 2, 2);
  compute_tile_starts(starts, counts, 4);
  bin_triangles_into_tiles(counts, tris, bin, starts, 4, 2, 2, 2);
  TEST_ASSERT_EQUAL_INT(0, bin[starts[0]]); /* tile 0 → tri 0 */
  TEST_ASSERT_EQUAL_INT(1, bin[starts[1]]); /* tile 1 → tri 1 */
}

/* ---- package_screen_tri ---- */

void test_package_positions_and_inv_z(void) {
  vec3f proj[3] = {{1, 2, 4.0f}, {3, 4, 2.0f}, {5, 6, 1.0f}};
  vec2f uvs[3] = {{0, 0}, {1, 0}, {0, 1}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  Color col = {255, 0, 0, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, false, col, light_rgb, 0,
                     0, 10, 0, 10, shadow);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, st.v[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 4.0f, st.v[0].z);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.25f, st.inv_z[0]); /* 1/4 */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f,  st.inv_z[1]); /* 1/2 */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f,  st.inv_z[2]); /* 1/1 */
}

void test_package_uvs_copied(void) {
  vec3f proj[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {{0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f}};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  Color col = {0, 0, 0, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, false, col, light_rgb, 0,
                     0, 10, 0, 10, shadow);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.1f, st.uv[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.2f, st.uv[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, st.uv[2].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.6f, st.uv[2].y);
}

void test_package_normals_copied(void) {
  vec3f proj[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {0};
  vec3f normals[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  Color col = {0, 0, 0, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, false, col, light_rgb, 0,
                     0, 10, 0, 10, shadow);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, st.normals[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, st.normals[1].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, st.normals[2].z);
}

void test_package_has_tex_false_sets_null(void) {
  vec3f proj[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {0};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  Color pixels[4];
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  inst.tex_pixels = pixels;
  inst.tex_w = 2;
  inst.tex_h = 2;
  Color col = {0, 0, 0, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, false, col, light_rgb, 0,
                     0, 10, 0, 10, shadow);
  TEST_ASSERT_NULL(st.tex);
}

void test_package_has_tex_true_sets_ptr(void) {
  vec3f proj[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {0};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  Color pixels[4];
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  inst.tex_pixels = pixels;
  inst.tex_w = 2;
  inst.tex_h = 2;
  Color col = {0, 0, 0, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, true, col, light_rgb, 0,
                     0, 10, 0, 10, shadow);
  TEST_ASSERT_EQUAL_PTR(pixels, st.tex);
  TEST_ASSERT_EQUAL_INT(2, st.tex_w);
  TEST_ASSERT_EQUAL_INT(2, st.tex_h);
}

void test_package_bbox_and_metadata(void) {
  vec3f proj[3] = {{0, 0, 2.0f}, {1, 0, 2.0f}, {0, 1, 2.0f}};
  vec2f uvs[3] = {0};
  vec3f normals[3] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  Color col = {7, 8, 9, 255};
  vec3f light_rgb = {1, 1, 1};
  screen_tri_t st;
  float shadow[3] = {1.0f, 1.0f, 1.0f};
  package_screen_tri(&st, proj, uvs, normals, &inst, false, col, light_rgb,
                     42, 5, 15, 10, 20, shadow);
  TEST_ASSERT_EQUAL_INT(42, st.instance_id);
  TEST_ASSERT_EQUAL_INT(5,  st.bx0);
  TEST_ASSERT_EQUAL_INT(15, st.bx1);
  TEST_ASSERT_EQUAL_INT(10, st.by0);
  TEST_ASSERT_EQUAL_INT(20, st.by1);
  TEST_ASSERT_EQUAL_UINT8(7, st.flat_color.r);
  TEST_ASSERT_EQUAL_UINT8(8, st.flat_color.g);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_clip_all_behind_returns_zero);
  RUN_TEST(test_clip_all_in_front_returns_three);
  RUN_TEST(test_clip_one_behind_returns_four);
  RUN_TEST(test_clip_two_behind_returns_three);
  RUN_TEST(test_clip_clipped_vertex_is_at_near_plane);
  RUN_TEST(test_clip_all_outputs_in_front_of_near);
  RUN_TEST(test_clip_uv_lerped_at_clip_point);
  RUN_TEST(test_clip_normal_lerped_at_clip_point);
  RUN_TEST(test_is_backfacing_true_for_away_facing);
  RUN_TEST(test_is_backfacing_false_for_front_facing);
  RUN_TEST(test_compute_face_normal_matches_expected_axis);
  RUN_TEST(test_compute_bbox_basic);
  RUN_TEST(test_compute_bbox_clamped_to_screen);
  RUN_TEST(test_bbox_collapsed_zero_width);
  RUN_TEST(test_bbox_collapsed_zero_height);
  RUN_TEST(test_bbox_not_collapsed);
  RUN_TEST(test_get_uvs_copies_correct_slice);
  RUN_TEST(test_get_uvs_null_no_crash);
  RUN_TEST(test_edge_collapsed_identical_verts);
  RUN_TEST(test_edge_not_collapsed_normal_triangle);
  RUN_TEST(test_area_collapsed_collinear);
  RUN_TEST(test_area_not_collapsed_normal_triangle);
  RUN_TEST(test_tile_coords_origin_tile);
  RUN_TEST(test_tile_coords_second_tile_x);
  RUN_TEST(test_tile_coords_second_row);
  RUN_TEST(test_tile_coords_clamps_at_screen_edge);
  RUN_TEST(test_fp_bias_nonnegative);
  RUN_TEST(test_fp_bias_seam_bias_added);
  RUN_TEST(test_tile_starts_basic);
  RUN_TEST(test_tile_starts_with_empty_tiles);
  RUN_TEST(test_tile_starts_all_zero);
  RUN_TEST(test_count_single_tri_single_tile);
  RUN_TEST(test_count_tri_spanning_all_tiles);
  RUN_TEST(test_count_two_tris_different_tiles);
  RUN_TEST(test_bin_single_tri_lands_in_tile0);
  RUN_TEST(test_bin_tri_spanning_all_tiles_appears_in_each);
  RUN_TEST(test_bin_two_tris_correct_slots);
  RUN_TEST(test_package_positions_and_inv_z);
  RUN_TEST(test_package_uvs_copied);
  RUN_TEST(test_package_normals_copied);
  RUN_TEST(test_package_has_tex_false_sets_null);
  RUN_TEST(test_package_has_tex_true_sets_ptr);
  RUN_TEST(test_package_bbox_and_metadata);
  return UNITY_END();
}
