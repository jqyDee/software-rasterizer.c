#include "../../math/vec.h"
#include "../../renderer/renderer.h"
#include "../unity.h"
#include <float.h>
#include <string.h>

/* Exposed via -DUNIT_TEST in test build */
extern void draw_triangle_pixels_tiled(float *depthbuffer, Color *framebuffer,
                                       int *idbuffer, Color *albedobuffer,
                                       Color *normalbuffer, int screen_width,
                                       const screen_tri_t *st, int clip_x0,
                                       int clip_y0, int clip_x1, int clip_y1);

#define W 8
#define H 8

static float depth[W * H];
static Color fb[W * H];

static const Color COL_BLACK = {0, 0, 0, 255};
static const Color COL_RED = {255, 0, 0, 255};
static const Color COL_GREEN = {0, 255, 0, 255};

void setUp(void) {
  for (int i = 0; i < W * H; i++) {
    depth[i] = 0.0f;
    fb[i] = COL_BLACK;
  }
}

void tearDown(void) {}

/* Triangle: v0=(1,1), v1=(1,6), v2=(6,1) — upper-left half of 8x8 grid.
   Pixel (2,2) is inside; pixel (6,6) is outside.
   area = (6-1)*(6-1) - (1-1)*(1-1) = 25 → CCW, no sign flip needed. */
static screen_tri_t base_tri(Color color) {
  screen_tri_t st;
  memset(&st, 0, sizeof(st));
  st.v[0] = (vec3f){1.0f, 1.0f, 2.0f};
  st.v[1] = (vec3f){1.0f, 6.0f, 2.0f};
  st.v[2] = (vec3f){6.0f, 1.0f, 2.0f};
  st.inv_z[0] = st.inv_z[1] = st.inv_z[2] = 0.5f; /* z=2 */
  st.tex = NULL;
  st.flat_color = color;
  st.light_rgb = (vec3f){1.0f, 1.0f, 1.0f}; /* neutral: color_scale is a no-op */
  st.shadow[0] = st.shadow[1] = st.shadow[2] = 1.0f; /* neutral: fully lit */
  st.bx0 = 1;
  st.by0 = 1;
  st.bx1 = 6;
  st.by1 = 6;
  st.fp_bias = 0.0f;
  return st;
}

/* ---- coverage ---- */

void test_pixel_inside_colored(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(0, fb[2 * W + 2].g);
}

void test_pixel_outside_unchanged(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  /* (6,6) outside triangle */
  TEST_ASSERT_EQUAL_UINT8(0, fb[6 * W + 6].r);
}

void test_depth_written_for_covered_pixel(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_TRUE(depth[2 * W + 2] > 0.0f);
}

/* ---- depth test ---- */

void test_depth_test_rejects_farther_triangle(void) {
  screen_tri_t near = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &near, 0, 0,
                             W - 1, H - 1);

  screen_tri_t far = base_tri(COL_GREEN);
  far.inv_z[0] = far.inv_z[1] = far.inv_z[2] = 0.25f; /* z=4, farther */
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &far, 0, 0,
                             W - 1, H - 1);

  /* red (first draw) should survive */
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(0, fb[2 * W + 2].g);
}

void test_depth_test_accepts_closer_triangle(void) {
  screen_tri_t far = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &far, 0, 0,
                             W - 1, H - 1);

  screen_tri_t near = base_tri(COL_GREEN);
  near.inv_z[0] = near.inv_z[1] = near.inv_z[2] = 1.0f; /* z=1, closer */
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &near, 0, 0,
                             W - 1, H - 1);

  /* green (closer) should overwrite */
  TEST_ASSERT_EQUAL_UINT8(0, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].g);
}

/* ---- clip bounds ---- */

void test_clip_excludes_pixel_left_of_bound(void) {
  screen_tri_t st = base_tri(COL_RED);
  /* clip_x0=4 excludes pixel column 2 */
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 4, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(0, fb[2 * W + 2].r);
}

void test_clip_excludes_pixel_above_bound(void) {
  screen_tri_t st = base_tri(COL_RED);
  /* clip_y0=4 excludes pixel row 2 */
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 4, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(0, fb[2 * W + 2].r);
}

/* ---- setup_edge_walk: CW winding normalization ---- */

void test_cw_triangle_covers_interior_pixel(void) {
  /* base_tri is CCW; swap v1↔v2 to get CW — same triangle, opposite winding */
  screen_tri_t st = base_tri(COL_RED);
  vec3f tmp_v  = st.v[1];    st.v[1]    = st.v[2];    st.v[2]    = tmp_v;
  float tmp_iz = st.inv_z[1]; st.inv_z[1] = st.inv_z[2]; st.inv_z[2] = tmp_iz;
  vec2f tmp_uv = st.uv[1];   st.uv[1]   = st.uv[2];   st.uv[2]   = tmp_uv;
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  /* pixel (2,2) is inside regardless of winding — normalization must fix it */
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r);
}

void test_cw_triangle_outside_pixel_unchanged(void) {
  screen_tri_t st = base_tri(COL_RED);
  vec3f tmp_v  = st.v[1];    st.v[1]    = st.v[2];    st.v[2]    = tmp_v;
  float tmp_iz = st.inv_z[1]; st.inv_z[1] = st.inv_z[2]; st.inv_z[2] = tmp_iz;
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(0, fb[6 * W + 6].r);
}

/* ---- fetch_pixel_albedo: texture sampling ---- */

void test_texture_1x1_sampled_for_any_uv(void) {
  /* 1×1 texture always returns same texel regardless of UV */
  Color tex[1] = {{11, 22, 33, 255}};
  screen_tri_t st = base_tri(COL_BLACK);
  st.tex = tex; st.tex_w = 1; st.tex_h = 1;
  st.uv[0] = (vec2f){0.0f, 0.0f};
  st.uv[1] = (vec2f){1.0f, 0.0f};
  st.uv[2] = (vec2f){0.0f, 1.0f};
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(11, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(22, fb[2 * W + 2].g);
  TEST_ASSERT_EQUAL_UINT8(33, fb[2 * W + 2].b);
}

void test_texture_overrides_flat_color(void) {
  Color tex[1] = {{99, 99, 99, 255}};
  screen_tri_t st = base_tri(COL_RED);
  st.tex = tex; st.tex_w = 1; st.tex_h = 1;
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  /* flat_color is red, but texture should win */
  TEST_ASSERT_EQUAL_UINT8(99, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(99, fb[2 * W + 2].g);
}

void test_texture_uv_wrapping(void) {
  /* u=1.5 → wraps to 0.5 → tex_x=1 (in a 2×2 texture)
     v=0.0 → flipped to 1.0 → tex_y=1
     → tex[1*2+1] = yellow */
  Color tex[4] = {
      {255, 0,   0,   255}, /* row0 col0 red    */
      {0,   255, 0,   255}, /* row0 col1 green  */
      {0,   0,   255, 255}, /* row1 col0 blue   */
      {255, 255, 0,   255}, /* row1 col1 yellow */
  };
  screen_tri_t st = base_tri(COL_BLACK);
  st.tex = tex; st.tex_w = 2; st.tex_h = 2;
  vec2f uv = {1.5f, 0.0f};
  st.uv[0] = st.uv[1] = st.uv[2] = uv;
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].g);
  TEST_ASSERT_EQUAL_UINT8(0,   fb[2 * W + 2].b);
}

void test_texture_v_flip(void) {
  /* v=0 in UV space maps to BOTTOM row of texture (V flip convention)
     2×2: row0 = red, row1 = green; v=0 → flipped to 1 → row1 → green */
  Color tex[4] = {
      {255, 0,   0, 255}, /* row0 col0 */
      {255, 0,   0, 255}, /* row0 col1 */
      {0,   200, 0, 255}, /* row1 col0 */
      {0,   200, 0, 255}, /* row1 col1 */
  };
  screen_tri_t st = base_tri(COL_BLACK);
  st.tex = tex; st.tex_w = 2; st.tex_h = 2;
  vec2f uv = {0.25f, 0.0f}; /* v=0 should land on row1 after flip */
  st.uv[0] = st.uv[1] = st.uv[2] = uv;
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(0,   fb[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(200, fb[2 * W + 2].g);
}

/* ---- idbuffer ---- */

void test_idbuffer_written_for_covered_pixel(void) {
  int idbuf[W * H];
  memset(idbuf, -1, sizeof(idbuf));
  screen_tri_t st = base_tri(COL_RED);
  st.instance_id = 7;
  draw_triangle_pixels_tiled(depth, fb, idbuf, NULL, NULL, W, &st, 0, 0,
                             W - 1, H - 1);
  TEST_ASSERT_EQUAL_INT(7, idbuf[2 * W + 2]);
}

void test_idbuffer_null_no_crash(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r); /* still drew */
}

/* ---- G-buffer: albedobuffer / normalbuffer ---- */

void test_albedobuffer_written_unlit(void) {
  /* light_rgb scales the framebuffer but must not affect albedo */
  Color albedo[W * H];
  memset(albedo, 0, sizeof(albedo));
  screen_tri_t st = base_tri(COL_RED);
  st.light_rgb = (vec3f){0.2f, 0.2f, 0.2f}; /* dims the lit framebuffer */
  draw_triangle_pixels_tiled(depth, fb, NULL, albedo, NULL, W, &st, 0, 0,
                             W - 1, H - 1);
  /* albedo keeps the unlit flat_color regardless of light_rgb */
  TEST_ASSERT_EQUAL_UINT8(255, albedo[2 * W + 2].r);
  TEST_ASSERT_EQUAL_UINT8(0, albedo[2 * W + 2].g);
  /* framebuffer, in contrast, is dimmed by light_rgb */
  TEST_ASSERT_EQUAL_UINT8(51, fb[2 * W + 2].r); /* 255 * 0.2 = 51 */
}

void test_albedobuffer_null_no_crash(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r); /* still drew */
}

void test_normalbuffer_encodes_flat_normal(void) {
  /* all three vertex normals equal -> interpolated normal is that same
     vector everywhere inside the triangle. (0,0,1) encodes to (128,128,255)
     via n*0.5+0.5 packed into 8-bit channels. */
  Color normalbuf[W * H];
  memset(normalbuf, 0, sizeof(normalbuf));
  screen_tri_t st = base_tri(COL_RED);
  st.normals[0] = st.normals[1] = st.normals[2] = (vec3f){0.0f, 0.0f, 1.0f};
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, normalbuf, W, &st, 0, 0,
                             W - 1, H - 1);
  TEST_ASSERT_UINT8_WITHIN(1, 128, normalbuf[2 * W + 2].r);
  TEST_ASSERT_UINT8_WITHIN(1, 128, normalbuf[2 * W + 2].g);
  TEST_ASSERT_UINT8_WITHIN(1, 255, normalbuf[2 * W + 2].b);
}

void test_normalbuffer_null_no_crash(void) {
  screen_tri_t st = base_tri(COL_RED);
  draw_triangle_pixels_tiled(depth, fb, NULL, NULL, NULL, W, &st, 0, 0, W - 1,
                             H - 1);
  TEST_ASSERT_EQUAL_UINT8(255, fb[2 * W + 2].r); /* still drew */
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_pixel_inside_colored);
  RUN_TEST(test_pixel_outside_unchanged);
  RUN_TEST(test_depth_written_for_covered_pixel);
  RUN_TEST(test_depth_test_rejects_farther_triangle);
  RUN_TEST(test_depth_test_accepts_closer_triangle);
  RUN_TEST(test_clip_excludes_pixel_left_of_bound);
  RUN_TEST(test_clip_excludes_pixel_above_bound);
  RUN_TEST(test_idbuffer_written_for_covered_pixel);
  RUN_TEST(test_idbuffer_null_no_crash);
  RUN_TEST(test_cw_triangle_covers_interior_pixel);
  RUN_TEST(test_cw_triangle_outside_pixel_unchanged);
  RUN_TEST(test_texture_1x1_sampled_for_any_uv);
  RUN_TEST(test_texture_overrides_flat_color);
  RUN_TEST(test_texture_uv_wrapping);
  RUN_TEST(test_texture_v_flip);
  RUN_TEST(test_albedobuffer_written_unlit);
  RUN_TEST(test_albedobuffer_null_no_crash);
  RUN_TEST(test_normalbuffer_encodes_flat_normal);
  RUN_TEST(test_normalbuffer_null_no_crash);
  return UNITY_END();
}
