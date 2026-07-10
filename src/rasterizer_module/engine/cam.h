#pragma once

#include <stdbool.h>

#include "../math/vec.h"

typedef struct cam_s {
  vec3f pos;
  float pitch, yaw;
  float fov;
  float focal_length;
} cam;

bool init_cam(cam *cam);
