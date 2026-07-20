#include "../../math/projection.h"
#include "../unity.h"
#include <string.h>

/* project formula:
     x_proj = (x/z) * focal_length
     y_proj = (y/z) * focal_length * aspect_ratio
     out->x = vp->x + (x_proj + 1) * 0.5 * vp->w
     out->y = vp->y + (1 - y_proj) * 0.5 * vp->h
     out->z = z */

#define EPS 1e-4f

static cam make_cam(float focal_length) {
  cam c;
  memset(&c, 0, sizeof(c));
  c.focal_length = focal_length;
  return c;
}

static viewport make_viewport(int w, int h, float aspect) {
  viewport vp;
  memset(&vp, 0, sizeof(vp));
  vp.x = 0;
  vp.y = 0;
  vp.w = w;
  vp.h = h;
  vp.aspect_ratio = aspect;
  return vp;
}

void setUp(void) {}
void tearDown(void) {}

void test_project_center_lands_at_screen_center(void) {
  cam      c  = make_cam(1.f);
  viewport vp = make_viewport(4, 4, 1.f);
  vec3f out;
  project(&c, &vp, (vec3f){0, 0, 1}, &out);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 2.f, out.x);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 2.f, out.y);
}

void test_project_z_passthrough(void) {
  cam      c  = make_cam(1.f);
  viewport vp = make_viewport(4, 4, 1.f);
  vec3f out;
  project(&c, &vp, (vec3f){0, 0, 5}, &out);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 5.f, out.z);
}

void test_project_positive_x_moves_right(void) {
  cam      c  = make_cam(1.f);
  viewport vp = make_viewport(4, 4, 1.f);
  vec3f out;
  /* x=0.5, z=1, focal=1: x_proj=0.5 → out->x = 1.5*0.5*4 = 3 */
  project(&c, &vp, (vec3f){0.5f, 0, 1}, &out);
  TEST_ASSERT_FLOAT_WITHIN(EPS, 3.f, out.x);
}

void test_project_focal_length_scales_x(void) {
  /* doubling focal_length doubles displacement from center */
  cam      c1 = make_cam(1.f);
  cam      c2 = make_cam(2.f);
  viewport vp = make_viewport(4, 4, 1.f);
  vec3f out1, out2;
  project(&c1, &vp, (vec3f){0.5f, 0, 1}, &out1);
  project(&c2, &vp, (vec3f){0.5f, 0, 1}, &out2);
  /* displacement from center: (out->x - 2) doubles */
  float d1 = out1.x - 2.f;
  float d2 = out2.x - 2.f;
  TEST_ASSERT_FLOAT_WITHIN(EPS, d1 * 2.f, d2);
}

void test_project_positive_y_maps_above_center(void) {
  /* positive y in cam space = up = smaller screen y (Y-down screen) */
  cam      c  = make_cam(1.f);
  viewport vp = make_viewport(4, 4, 1.f);
  vec3f out;
  project(&c, &vp, (vec3f){0, 1, 1}, &out);
  /* y_proj=1 → out->y = (1-1)*0.5*4 = 0 (top of screen) */
  TEST_ASSERT_FLOAT_WITHIN(EPS, 0.f, out.y);
}

void test_project_aspect_ratio_scales_y(void) {
  /* aspect=2 doubles y displacement from center vs aspect=1 */
  cam      c   = make_cam(1.f);
  viewport vp1 = make_viewport(4, 4, 1.f);
  viewport vp2 = make_viewport(4, 4, 2.f);
  vec3f out1, out2;
  project(&c, &vp1, (vec3f){0, 0.5f, 1}, &out1);
  project(&c, &vp2, (vec3f){0, 0.5f, 1}, &out2);
  float d1 = 2.f - out1.y; /* distance above center */
  float d2 = 2.f - out2.y;
  TEST_ASSERT_FLOAT_WITHIN(EPS, d1 * 2.f, d2);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_project_center_lands_at_screen_center);
  RUN_TEST(test_project_z_passthrough);
  RUN_TEST(test_project_positive_x_moves_right);
  RUN_TEST(test_project_focal_length_scales_x);
  RUN_TEST(test_project_positive_y_maps_above_center);
  RUN_TEST(test_project_aspect_ratio_scales_y);
  return UNITY_END();
}
