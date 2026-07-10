#include "../../math/color.h"
#include "../unity.h"

void setUp(void) {}
void tearDown(void) {}

/* HSV expected values: s=1 v=1 unless stated.
   x = c*(1 - |fmod(h/60, 2) - 1|), m = v - c.
   Each branch: r/g/b assigned from c/x/0 in different order. */

void test_hsv_hue_0_is_red(void) {
  /* h<60: r=c=1, g=x=0, b=0 */
  Color c = hsv_to_rgb(0.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(255, c.r);
  TEST_ASSERT_EQUAL_UINT8(0,   c.g);
  TEST_ASSERT_EQUAL_UINT8(0,   c.b);
}

void test_hsv_hue_90_yellow_green(void) {
  /* 60≤h<120: r=x=0.5, g=c=1, b=0 → (127,255,0) */
  Color c = hsv_to_rgb(90.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(127, c.r);
  TEST_ASSERT_EQUAL_UINT8(255, c.g);
  TEST_ASSERT_EQUAL_UINT8(0,   c.b);
}

void test_hsv_hue_150_cyan(void) {
  /* 120≤h<180: r=0, g=c=1, b=x=0.5 → (0,255,127) */
  Color c = hsv_to_rgb(150.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(0,   c.r);
  TEST_ASSERT_EQUAL_UINT8(255, c.g);
  TEST_ASSERT_EQUAL_UINT8(127, c.b);
}

void test_hsv_hue_210_azure(void) {
  /* 180≤h<240: r=0, g=x=0.5, b=c=1 → (0,127,255) */
  Color c = hsv_to_rgb(210.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(0,   c.r);
  TEST_ASSERT_EQUAL_UINT8(127, c.g);
  TEST_ASSERT_EQUAL_UINT8(255, c.b);
}

void test_hsv_hue_270_violet(void) {
  /* 240≤h<300: r=x=0.5, g=0, b=c=1 → (127,0,255) */
  Color c = hsv_to_rgb(270.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(127, c.r);
  TEST_ASSERT_EQUAL_UINT8(0,   c.g);
  TEST_ASSERT_EQUAL_UINT8(255, c.b);
}

void test_hsv_hue_330_rose(void) {
  /* h≥300: r=c=1, g=0, b=x=0.5 → (255,0,127) */
  Color c = hsv_to_rgb(330.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(255, c.r);
  TEST_ASSERT_EQUAL_UINT8(0,   c.g);
  TEST_ASSERT_EQUAL_UINT8(127, c.b);
}

void test_hsv_zero_saturation_is_gray(void) {
  /* s=0: c=0, m=v=1 → r=g=b=255 regardless of hue */
  Color c = hsv_to_rgb(180.f, 0.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(255, c.r);
  TEST_ASSERT_EQUAL_UINT8(255, c.g);
  TEST_ASSERT_EQUAL_UINT8(255, c.b);
}

void test_hsv_zero_value_is_black(void) {
  /* v=0: c=0, m=0 → r=g=b=0 */
  Color c = hsv_to_rgb(120.f, 1.f, 0.f);
  TEST_ASSERT_EQUAL_UINT8(0, c.r);
  TEST_ASSERT_EQUAL_UINT8(0, c.g);
  TEST_ASSERT_EQUAL_UINT8(0, c.b);
}

void test_hsv_alpha_always_255(void) {
  Color c = hsv_to_rgb(0.f, 1.f, 1.f);
  TEST_ASSERT_EQUAL_UINT8(255, c.a);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hsv_hue_0_is_red);
  RUN_TEST(test_hsv_hue_90_yellow_green);
  RUN_TEST(test_hsv_hue_150_cyan);
  RUN_TEST(test_hsv_hue_210_azure);
  RUN_TEST(test_hsv_hue_270_violet);
  RUN_TEST(test_hsv_hue_330_rose);
  RUN_TEST(test_hsv_zero_saturation_is_gray);
  RUN_TEST(test_hsv_zero_value_is_black);
  RUN_TEST(test_hsv_alpha_always_255);
  return UNITY_END();
}
