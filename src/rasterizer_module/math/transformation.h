#pragma once

#include "vec.h"
#include "../assets/mesh.h"

#define DEG_TO_RAD 0.017453292f // pi / 180deg
#define RAD_TO_DEG 57.29577951f // 180deg / pi

vec3f to_object_space(const mesh_instance *inst, vec3f p);
