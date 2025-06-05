#pragma once

#include "types.h"

void x_rotation_around_origin(vec3f *v, float angle_rad);
void y_rotation_around_origin(vec3f *v, float angle_rad);
void rotate_mesh_around_origin(mesh *mesh, float x_angle_rad,
                               float y_angle_rad);
