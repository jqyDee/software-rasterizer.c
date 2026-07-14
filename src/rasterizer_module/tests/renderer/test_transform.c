#include "../../renderer/transform.h"
#include "../../math/vec.h"
#include "../unity.h"

#include <math.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static mesh_instance identity_instance(void) {
  mesh_instance inst;
  memset(&inst, 0, sizeof(inst));
  inst.mesh_idx = 0;
  inst.scale = (vec3f){1.0f, 1.0f, 1.0f};
  return inst;
}

static cam identity_cam(void) {
  cam c;
  memset(&c, 0, sizeof(c));
  return c;
}

/* ---- transform_triangle_to_world ---- */

void test_world_identity_instance_passthrough(void) {
  vec3f verts[3] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  vec3f out[3];
  transform_triangle_to_world(&m, 0, &inst, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, out[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 3.0f, out[0].z);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 7.0f, out[2].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 9.0f, out[2].z);
}

void test_world_translation_applied(void) {
  vec3f verts[3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  inst.pos = (vec3f){5.0f, 10.0f, -3.0f};
  vec3f out[3];
  transform_triangle_to_world(&m, 0, &inst, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 5.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 10.0f, out[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, -3.0f, out[0].z);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 6.0f, out[1].x); /* 1 + 5 */
}

void test_world_scale_applied_before_translate(void) {
  vec3f verts[3] = {{1, 1, 1}, {0, 0, 0}, {0, 0, 0}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  inst.scale = (vec3f){2.0f, 3.0f, 4.0f};
  inst.pos = (vec3f){1.0f, 1.0f, 1.0f};
  vec3f out[3];
  transform_triangle_to_world(&m, 0, &inst, out);
  /* scale first (1*2, 1*3, 1*4), then translate (+1,+1,+1) */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 3.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 4.0f, out[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 5.0f, out[0].z);
}

void test_world_yaw_90_rotates_xz(void) {
  /* roll->pitch->yaw with only yaw set: rotate_y(90deg) on (1,0,0) gives
     x'=x*cos+z*sin=0, z'=-x*sin+z*cos=-1 (math/rotation.c convention) */
  vec3f verts[3] = {{1, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  inst.rotation = (vec3f){0.0f, 90.0f, 0.0f};
  vec3f out[3];
  transform_triangle_to_world(&m, 0, &inst, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.0f, out[0].z);
}

/* ---- transform_triangle_to_camera ---- */

void test_camera_identity_matches_world(void) {
  vec3f verts[3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  cam c = identity_cam();
  vec3f out[3];
  transform_triangle_to_camera(&m, 0, &inst, &c, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 9.0f, out[2].z);
}

void test_camera_translation_subtracts_cam_pos(void) {
  vec3f verts[3] = {{5, 5, 5}, {0, 0, 0}, {0, 0, 0}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  cam c = identity_cam();
  c.pos = (vec3f){2.0f, 1.0f, 1.0f};
  vec3f out[3];
  transform_triangle_to_camera(&m, 0, &inst, &c, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 3.0f, out[0].x); /* 5 - 2 */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 4.0f, out[0].y); /* 5 - 1 */
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 4.0f, out[0].z); /* 5 - 1 */
}

void test_camera_yaw_rotates_relative_pos(void) {
  /* cam->yaw is stored in radians (accumulated directly from mouse deltas,
     unlike instance rotation which is degrees). rotate_y(rel, -cam->yaw)
     on rel=(1,0,0) with yaw=+pi/2: x'=cos(-pi/2)=0, z'=-sin(-pi/2)=1 */
  vec3f verts[3] = {{1, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  mesh m = {0};
  m.vertices = verts;
  mesh_instance inst = identity_instance();
  cam c = identity_cam();
  c.yaw = (float)M_PI / 2.0f;
  vec3f out[3];
  transform_triangle_to_camera(&m, 0, &inst, &c, out);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, out[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, out[0].z);
}

/* ---- transform_normal_to_camera ---- */

void test_normal_identity_unchanged(void) {
  mesh_instance inst = identity_instance();
  cam c = identity_cam();
  vec3f n = transform_normal_to_camera((vec3f){0, 1, 0}, &inst, &c);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, n.x);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, n.y);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, n.z);
}

void test_normal_nonuniform_scale_tilts_direction(void) {
  /* normals transform by dividing by scale (inverse), not multiplying —
     this is a regression test for that sign/direction: scaling x by 2
     shrinks the normal's x-component relative to y, tilting it toward y. */
  mesh_instance inst = identity_instance();
  inst.scale = (vec3f){2.0f, 1.0f, 1.0f};
  cam c = identity_cam();
  float inv_sqrt2 = 0.70710678f;
  vec3f n =
      transform_normal_to_camera((vec3f){inv_sqrt2, inv_sqrt2, 0.0f}, &inst, &c);
  /* pre-normalize: (0.5/1 * inv_sqrt2, inv_sqrt2, 0) = (0.35355, 0.70711, 0)
     normalized: (0.4472, 0.8944, 0) */
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.4472f, n.x);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.8944f, n.y);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, n.z);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_world_identity_instance_passthrough);
  RUN_TEST(test_world_translation_applied);
  RUN_TEST(test_world_scale_applied_before_translate);
  RUN_TEST(test_world_yaw_90_rotates_xz);
  RUN_TEST(test_camera_identity_matches_world);
  RUN_TEST(test_camera_translation_subtracts_cam_pos);
  RUN_TEST(test_camera_yaw_rotates_relative_pos);
  RUN_TEST(test_normal_identity_unchanged);
  RUN_TEST(test_normal_nonuniform_scale_tilts_direction);
  return UNITY_END();
}
