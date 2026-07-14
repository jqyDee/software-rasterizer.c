#include "../../renderer/shadow.h"
#include "../../math/vec.h"
#include "../../world.h"
#include "../unity.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* ---- compute_shadow_light_basis ---- */

void test_light_basis_is_orthonormal(void) {
  world w;
  renderer r;
  memset(&w, 0, sizeof(w));
  memset(&r, 0, sizeof(r));
  w.renderer = &r;
  w.lights[0].light_dir = vec_normalize(((vec3f){-1.3f, 1.0f, -1.2f}));

  compute_shadow_light_basis(&w);

  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, vec_dot(r.light_right, r.light_up));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, vec_dot(r.light_right, r.light_fwd));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, vec_dot(r.light_up, r.light_fwd));

  float len_right = sqrtf(vec_dot(r.light_right, r.light_right));
  float len_up = sqrtf(vec_dot(r.light_up, r.light_up));
  float len_fwd = sqrtf(vec_dot(r.light_fwd, r.light_fwd));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, len_right);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, len_up);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, len_fwd);
}

void test_light_basis_fwd_is_negated_light_dir(void) {
  world w;
  renderer r;
  memset(&w, 0, sizeof(w));
  memset(&r, 0, sizeof(r));
  w.renderer = &r;
  vec3f dir = vec_normalize(((vec3f){0.3f, 0.5f, -0.8f}));
  w.lights[0].light_dir = dir;

  compute_shadow_light_basis(&w);

  /* fwd = light travel direction = opposite of light_dir (which points
     surface -> light, matching the Lambertian dot(N,L) convention) */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, -dir.x, r.light_fwd.x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, -dir.y, r.light_fwd.y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, -dir.z, r.light_fwd.z);
}

void test_light_basis_near_vertical_stays_orthonormal(void) {
  /* near-vertical light_dir is the degenerate case the up_hint switch
     (0,0,1) vs (0,1,0) exists to avoid — must not produce NaN/degenerate
     vectors */
  world w;
  renderer r;
  memset(&w, 0, sizeof(w));
  memset(&r, 0, sizeof(r));
  w.renderer = &r;
  w.lights[0].light_dir = (vec3f){0.0f, 1.0f, 0.0f}; /* straight up */

  compute_shadow_light_basis(&w);

  float len_right = sqrtf(vec_dot(r.light_right, r.light_right));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, len_right);
  TEST_ASSERT_TRUE(len_right == len_right); /* NaN != NaN, so this fails on NaN */
}

/* ---- world_to_light_screen ---- */

static renderer axis_aligned_renderer(void) {
  renderer r;
  memset(&r, 0, sizeof(r));
  r.light_right = (vec3f){1, 0, 0};
  r.light_up = (vec3f){0, 1, 0};
  r.light_fwd = (vec3f){0, 0, 1};
  return r;
}

void test_world_to_light_screen_origin_maps_to_center(void) {
  renderer r = axis_aligned_renderer();
  float sx, sy, closeness;
  world_to_light_screen(&r, (vec3f){0, 0, 0}, &sx, &sy, &closeness);
  TEST_ASSERT_FLOAT_WITHIN(1e-2f, SHADOW_MAP_SIZE / 2.0f, sx);
  TEST_ASSERT_FLOAT_WITHIN(1e-2f, SHADOW_MAP_SIZE / 2.0f, sy);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, closeness);
}

void test_world_to_light_screen_positive_x_moves_right(void) {
  renderer r = axis_aligned_renderer();
  float sx, sy, closeness;
  world_to_light_screen(&r, (vec3f){SHADOW_ORTHO_HALF_EXTENT, 0, 0}, &sx, &sy,
                        &closeness);
  TEST_ASSERT_FLOAT_WITHIN(1e-1f, (float)SHADOW_MAP_SIZE, sx);
}

void test_world_to_light_screen_closer_to_light_has_larger_closeness(void) {
  /* fwd = (0,0,1) = light travel direction; a point with smaller lz is
     nearer the light, and closeness = -lz must be correspondingly larger */
  renderer r = axis_aligned_renderer();
  float sx, sy, c_near, c_far;
  world_to_light_screen(&r, (vec3f){0, 0, -5}, &sx, &sy, &c_near);
  world_to_light_screen(&r, (vec3f){0, 0, 5}, &sx, &sy, &c_far);
  TEST_ASSERT_TRUE(c_near > c_far);
}

/* ---- sample_shadow ---- */

void test_sample_shadow_outside_frustum_returns_lit(void) {
  renderer r = axis_aligned_renderer();
  static float depthbuf[SHADOW_MAP_SIZE * SHADOW_MAP_SIZE];
  r.shadow_depthbuffer = depthbuf;
  float shade =
      sample_shadow(&r, (vec3f){SHADOW_ORTHO_HALF_EXTENT * 100.0f, 0, 0});
  TEST_ASSERT_EQUAL_FLOAT(1.0f, shade);
}

void test_sample_shadow_occluded_returns_darkness(void) {
  renderer r = axis_aligned_renderer();
  static float depthbuf[SHADOW_MAP_SIZE * SHADOW_MAP_SIZE];
  for (int i = 0; i < SHADOW_MAP_SIZE * SHADOW_MAP_SIZE; i++)
    depthbuf[i] = 1000.0f; /* a very-close-to-light occluder recorded here */
  r.shadow_depthbuffer = depthbuf;
  float shade = sample_shadow(&r, (vec3f){0, 0, 0}); /* closeness=0 << 1000 */
  TEST_ASSERT_EQUAL_FLOAT(SHADOW_DARKNESS, shade);
}

void test_sample_shadow_not_occluded_returns_lit(void) {
  renderer r = axis_aligned_renderer();
  static float depthbuf[SHADOW_MAP_SIZE * SHADOW_MAP_SIZE];
  for (int i = 0; i < SHADOW_MAP_SIZE * SHADOW_MAP_SIZE; i++)
    depthbuf[i] = -1000.0f; /* recorded occluder far from the light */
  r.shadow_depthbuffer = depthbuf;
  float shade = sample_shadow(&r, (vec3f){0, 0, 0}); /* closeness=0 > -1000 */
  TEST_ASSERT_EQUAL_FLOAT(1.0f, shade);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_light_basis_is_orthonormal);
  RUN_TEST(test_light_basis_fwd_is_negated_light_dir);
  RUN_TEST(test_light_basis_near_vertical_stays_orthonormal);
  RUN_TEST(test_world_to_light_screen_origin_maps_to_center);
  RUN_TEST(test_world_to_light_screen_positive_x_moves_right);
  RUN_TEST(test_world_to_light_screen_closer_to_light_has_larger_closeness);
  RUN_TEST(test_sample_shadow_outside_frustum_returns_lit);
  RUN_TEST(test_sample_shadow_occluded_returns_darkness);
  RUN_TEST(test_sample_shadow_not_occluded_returns_lit);
  return UNITY_END();
}
