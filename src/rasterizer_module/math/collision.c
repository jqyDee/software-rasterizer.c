#include "collision.h"

#include "../math/collision.h"
#include "../math/transformation.h"
#include "../world.h"

#include <float.h>

void compute_mesh_aabb(mesh *m) {
  if (!m->vertices || m->vertex_count == 0) {
    m->aabb_min = m->aabb_max = (vec3f){0};
    return;
  }
  vec3f mn = {FLT_MAX, FLT_MAX, FLT_MAX};
  vec3f mx = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
  for (size_t i = 0; i < m->vertex_count; i++) {
    vec3f v = m->vertices[i];
    if (v.x < mn.x)
      mn.x = v.x;
    if (v.x > mx.x)
      mx.x = v.x;
    if (v.y < mn.y)
      mn.y = v.y;
    if (v.y > mx.y)
      mx.y = v.y;
    if (v.z < mn.z)
      mn.z = v.z;
    if (v.z > mx.z)
      mx.z = v.z;
  }
  m->aabb_min = mn;
  m->aabb_max = mx;
}

/* Sphere (world) becomes ellipsoid in object space under non-uniform scale.
   Test per-axis normalized distance against 1. */
bool ellipsoid_hits_aabb(vec3f center, vec3f radii, vec3f mn, vec3f mx) {
  float dx = fmaxf(0.0f, fmaxf(mn.x - center.x, center.x - mx.x));
  float dy = fmaxf(0.0f, fmaxf(mn.y - center.y, center.y - mx.y));
  float dz = fmaxf(0.0f, fmaxf(mn.z - center.z, center.z - mx.z));
  dx /= radii.x;
  dy /= radii.y;
  dz /= radii.z;
  return dx * dx + dy * dy + dz * dz < 1.0f;
}

bool camera_collides(const world *world, vec3f pos) {
  for (int i = 0; i < (int)world->instance_count; i++) {
    const mesh_instance *inst = &world->instances[i];
    const mesh *m = &world->mesh_data[inst->mesh_idx];
    vec3f obj_pos = to_object_space(inst, pos);
    vec3f radii = {
        world->settings.camera_radius /
            (fabsf(inst->scale.x) > 1e-6f ? fabsf(inst->scale.x) : 1e-6f),
        world->settings.camera_radius /
            (fabsf(inst->scale.y) > 1e-6f ? fabsf(inst->scale.y) : 1e-6f),
        world->settings.camera_radius /
            (fabsf(inst->scale.z) > 1e-6f ? fabsf(inst->scale.z) : 1e-6f),
    };
    if (ellipsoid_hits_aabb(obj_pos, radii, m->aabb_min, m->aabb_max))
      return true;
  }
  return false;
}
