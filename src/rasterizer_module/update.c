#include <math.h>

#include "types.h"

void y_rotation_around_origin(vec3f *v, float angle_rad) {
  float x = v->x;
  float z = v->z;

  v->x = x * cosf(angle_rad) + z * sinf(angle_rad);
  v->z = -x * sinf(angle_rad) + z * cosf(angle_rad);
}

void x_rotation_around_origin(vec3f *v, float angle_rad) {
  float y = v->y;
  float z = v->z;

  v->y = y * cosf(angle_rad) - z * sinf(angle_rad);
  v->z = y * sinf(angle_rad) + z * cosf(angle_rad);
}

void rotate_mesh_around_origin(mesh *mesh, float x_angle_rad,
                               float y_angle_rad) {
  for (size_t i = 0; i < mesh->vertex_count; i++) {
    x_rotation_around_origin(&mesh->vertices[i], x_angle_rad);
    y_rotation_around_origin(&mesh->vertices[i], y_angle_rad);
  }
}
