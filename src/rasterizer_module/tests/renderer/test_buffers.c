#include "../../renderer/buffers.h"
#include "../unity.h"
#include <stdint.h>
#include <string.h>

#define W 4
#define H 3
#define N (W * H)

/* +1 sentinel slot: detect writes past end of buffer */
static Color fb[N + 1];
static float db[N + 1];
static int ib[N + 1];

static const Color COL_WHITE = {255, 255, 255, 255};
static const Color COL_RED = {255, 0, 0, 255};
static const Color COL_TEAL = {0, 128, 128, 255};

void setUp(void) {
  memset(fb, 0xAB, sizeof(fb));
  memset(db, 0xAB, sizeof(db));
  memset(ib, 0xAB, sizeof(ib));
}

void tearDown(void) {}

/* ---- clear_framebuffer: WHITE fast path (memset 0xFF) ---- */

void test_clear_fb_white_first_pixel(void) {
  clear_framebuffer(fb, W, H, COL_WHITE);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].r);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].g);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].b);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].a);
}

void test_clear_fb_white_last_pixel(void) {
  clear_framebuffer(fb, W, H, COL_WHITE);
  TEST_ASSERT_EQUAL_UINT8(255, fb[N - 1].r);
  TEST_ASSERT_EQUAL_UINT8(255, fb[N - 1].g);
}

void test_clear_fb_white_no_overflow(void) {
  clear_framebuffer(fb, W, H, COL_WHITE);
  /* sentinel bytes after the N-pixel region must still be 0xAB */
  uint8_t *guard = (uint8_t *)&fb[N];
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[1]);
}

/* ---- clear_framebuffer: non-white slow path (loop) ---- */

void test_clear_fb_color_first_pixel(void) {
  clear_framebuffer(fb, W, H, COL_RED);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].r);
  TEST_ASSERT_EQUAL_UINT8(0, fb[0].g);
  TEST_ASSERT_EQUAL_UINT8(0, fb[0].b);
}

void test_clear_fb_color_last_pixel(void) {
  clear_framebuffer(fb, W, H, COL_RED);
  TEST_ASSERT_EQUAL_UINT8(255, fb[N - 1].r);
  TEST_ASSERT_EQUAL_UINT8(0, fb[N - 1].g);
}

void test_clear_fb_color_no_overflow(void) {
  clear_framebuffer(fb, W, H, COL_RED);
  uint8_t *guard = (uint8_t *)&fb[N];
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[1]);
}

void test_clear_fb_arbitrary_color(void) {
  /* teal is not white, so it hits the loop path; verify all channels */
  clear_framebuffer(fb, W, H, COL_TEAL);
  TEST_ASSERT_EQUAL_UINT8(0, fb[0].r);
  TEST_ASSERT_EQUAL_UINT8(128, fb[0].g);
  TEST_ASSERT_EQUAL_UINT8(128, fb[0].b);
  TEST_ASSERT_EQUAL_UINT8(255, fb[0].a);
  TEST_ASSERT_EQUAL_UINT8(128, fb[N - 1].g);
}

/* ---- clear_depthbuffer ---- */

void test_clear_depth_all_zero(void) {
  /* inv_z = 0 means infinitely far; every slot must be 0.0f */
  clear_depthbuffer(db, W, H);
  for (int i = 0; i < N; i++)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, db[i]);
}

void test_clear_depth_no_overflow(void) {
  clear_depthbuffer(db, W, H);
  uint8_t *guard = (uint8_t *)&db[N];
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[1]);
}

/* ---- clear_idbuffer ---- */

void test_clear_id_all_minus_one(void) {
  /* -1 sentinel means "no instance here" */
  clear_idbuffer(ib, W, H);
  for (int i = 0; i < N; i++)
    TEST_ASSERT_EQUAL_INT(-1, ib[i]);
}

void test_clear_id_no_overflow(void) {
  clear_idbuffer(ib, W, H);
  uint8_t *guard = (uint8_t *)&ib[N];
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, guard[1]);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_clear_fb_white_first_pixel);
  RUN_TEST(test_clear_fb_white_last_pixel);
  RUN_TEST(test_clear_fb_white_no_overflow);
  RUN_TEST(test_clear_fb_color_first_pixel);
  RUN_TEST(test_clear_fb_color_last_pixel);
  RUN_TEST(test_clear_fb_color_no_overflow);
  RUN_TEST(test_clear_fb_arbitrary_color);
  RUN_TEST(test_clear_depth_all_zero);
  RUN_TEST(test_clear_depth_no_overflow);
  RUN_TEST(test_clear_id_all_minus_one);
  RUN_TEST(test_clear_id_no_overflow);
  return UNITY_END();
}
