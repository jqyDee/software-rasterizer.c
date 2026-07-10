#include "transformation.h"
#include "../testing.h"
#include "rotation.h"

/* Transform world-space point into instance object space (inverse TRS). */
vec3f to_object_space(const mesh_instance *inst, vec3f p) {
  // undo translate
  p = (vec3f){p.x - inst->pos.x, p.y - inst->pos.y, p.z - inst->pos.z};
  // undo rotation: inverse of (roll -> pitch -> yaw) is (-yaw -> -pitch ->
  // -roll)
  p = rotate_y(p, -inst->rotation.y * DEG_TO_RAD);
  p = rotate_x(p, -inst->rotation.x * DEG_TO_RAD);
  p = rotate_z(p, -inst->rotation.z * DEG_TO_RAD);

  // undo scale
  float sx = fabsf(inst->scale.x) > 1e-6f ? inst->scale.x : 1e-6f;
  float sy = fabsf(inst->scale.y) > 1e-6f ? inst->scale.y : 1e-6f;
  float sz = fabsf(inst->scale.z) > 1e-6f ? inst->scale.z : 1e-6f;
  return (vec3f){p.x / sx, p.y / sy, p.z / sz};
}
