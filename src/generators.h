#pragma once

#include "types.h"

typedef struct directions_s {
  vec3f *directions;
  size_t vertex_count;
} directions;

void generate_random_mesh(struct mesh_s *mesh, struct directions_s *directions,
                          size_t triangle_count);
