#include "../../math/vec.h"
#include "../unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- vec3f_add ---- */

void test_add_components(void) {
  vec3f r = vec3f_add((vec3f){1, 2, 3}, (vec3f){4, 5, 6});
  TEST_ASSERT_EQUAL_FLOAT(5.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(7.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(9.f, r.z);
}

/* ---- vec3f_sub ---- */

void test_sub_components(void) {
  vec3f r = vec3f_sub((vec3f){5, 7, 9}, (vec3f){1, 2, 3});
  TEST_ASSERT_EQUAL_FLOAT(4.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(5.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(6.f, r.z);
}

/* ---- vec3f_dot ---- */

void test_dot_orthogonal_is_zero(void) {
  TEST_ASSERT_EQUAL_FLOAT(0.f, vec3f_dot((vec3f){1, 0, 0}, (vec3f){0, 1, 0}));
}

void test_dot_parallel(void) {
  /* {1,2,3}·{1,2,3} = 1+4+9 = 14 */
  TEST_ASSERT_EQUAL_FLOAT(14.f, vec3f_dot((vec3f){1, 2, 3}, (vec3f){1, 2, 3}));
}

/* ---- vec3f_scale ---- */

void test_scale_by_two(void) {
  vec3f r = vec3f_scale((vec3f){1, 2, 3}, 2.f);
  TEST_ASSERT_EQUAL_FLOAT(2.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(4.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(6.f, r.z);
}

/* ---- vec3f_cross ---- */

void test_cross_x_y_gives_z(void) {
  vec3f r = vec3f_cross((vec3f){1, 0, 0}, (vec3f){0, 1, 0});
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(1.f, r.z);
}

void test_cross_anticommutative(void) {
  /* y×x = -(x×y) = {0,0,-1} */
  vec3f r = vec3f_cross((vec3f){0, 1, 0}, (vec3f){1, 0, 0});
  TEST_ASSERT_EQUAL_FLOAT(0.f,  r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f,  r.y);
  TEST_ASSERT_EQUAL_FLOAT(-1.f, r.z);
}

void test_cross_self_is_zero(void) {
  vec3f r = vec3f_cross((vec3f){1, 2, 3}, (vec3f){1, 2, 3});
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.z);
}

/* ---- vec3f_normalize ---- */

void test_normalize_produces_unit_length(void) {
  vec3f r = vec3f_normalize((vec3f){3, 4, 0});
  float len = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.f, len);
}

void test_normalize_zero_vector_returns_zero(void) {
  vec3f r = vec3f_normalize((vec3f){0, 0, 0});
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.z);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_add_components);
  RUN_TEST(test_sub_components);
  RUN_TEST(test_dot_orthogonal_is_zero);
  RUN_TEST(test_dot_parallel);
  RUN_TEST(test_scale_by_two);
  RUN_TEST(test_cross_x_y_gives_z);
  RUN_TEST(test_cross_anticommutative);
  RUN_TEST(test_cross_self_is_zero);
  RUN_TEST(test_normalize_produces_unit_length);
  RUN_TEST(test_normalize_zero_vector_returns_zero);
  return UNITY_END();
}
