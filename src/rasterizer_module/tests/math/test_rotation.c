#include "../../math/rotation.h"
#include "../unity.h"

#define PI 3.14159265f
#define EPS 1e-5f

void setUp(void) {}
void tearDown(void) {}

/* ---- rotate_x ---- */

void test_rotate_x_identity(void) {
  vec3f r = rotate_x((vec3f){1, 2, 3}, 0.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 1.f, r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 2.f, r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 3.f, r.z);
}

void test_rotate_x_90_deg(void) {
  /* {0,1,0} rotated 90° around X → {0,0,1} */
  vec3f r = rotate_x((vec3f){0, 1, 0}, PI / 2.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f, r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f, r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 1.f, r.z);
}

/* ---- rotate_y ---- */

void test_rotate_y_identity(void) {
  vec3f r = rotate_y((vec3f){1, 2, 3}, 0.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 1.f, r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 2.f, r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 3.f, r.z);
}

void test_rotate_y_90_deg(void) {
  /* {1,0,0} rotated 90° around Y → {0,0,-1} */
  vec3f r = rotate_y((vec3f){1, 0, 0}, PI / 2.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f,  r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f,  r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, -1.f, r.z);
}

/* ---- rotate_z ---- */

void test_rotate_z_identity(void) {
  vec3f r = rotate_z((vec3f){1, 2, 3}, 0.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 1.f, r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 2.f, r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 3.f, r.z);
}

void test_rotate_z_90_deg(void) {
  /* {1,0,0} rotated 90° around Z → {0,1,0} */
  vec3f r = rotate_z((vec3f){1, 0, 0}, PI / 2.f);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f, r.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 1.f, r.y);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f, r.z);
}

/* ---- rotate_x_snapped ---- */

void test_rotate_x_snapped_90_exact(void) {
  /* lookup: sin=1, cos=0 → y'=0 exactly, z'=1 exactly */
  vec3f r = rotate_x_snapped((vec3f){0, 1, 0}, PI / 2.f);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(1.f, r.z);
}

void test_rotate_x_snapped_0_exact(void) {
  /* lookup: sin=0, cos=1 → identity */
  vec3f r = rotate_x_snapped((vec3f){1, 2, 3}, 0.f);
  TEST_ASSERT_EQUAL_FLOAT(2.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(3.f, r.z);
}

/* ---- rotate_y_snapped ---- */

void test_rotate_y_snapped_180_exact(void) {
  /* lookup: sin=0, cos=-1 → {1,0,0} becomes {-1,0,0} */
  vec3f r = rotate_y_snapped((vec3f){1, 0, 0}, PI);
  TEST_ASSERT_EQUAL_FLOAT(-1.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f,  r.z);
}

/* ---- rotate_z_snapped ---- */

void test_rotate_z_snapped_neg90_exact(void) {
  /* -90° → quadrant=-1+4=3: sin=-1, cos=0 → {1,0,0} becomes {0,-1,0} */
  vec3f r = rotate_z_snapped((vec3f){1, 0, 0}, -PI / 2.f);
  TEST_ASSERT_EQUAL_FLOAT(0.f,  r.x);
  TEST_ASSERT_EQUAL_FLOAT(-1.f, r.y);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_rotate_x_identity);
  RUN_TEST(test_rotate_x_90_deg);
  RUN_TEST(test_rotate_y_identity);
  RUN_TEST(test_rotate_y_90_deg);
  RUN_TEST(test_rotate_z_identity);
  RUN_TEST(test_rotate_z_90_deg);
  RUN_TEST(test_rotate_x_snapped_90_exact);
  RUN_TEST(test_rotate_x_snapped_0_exact);
  RUN_TEST(test_rotate_y_snapped_180_exact);
  RUN_TEST(test_rotate_z_snapped_neg90_exact);
  return UNITY_END();
}
