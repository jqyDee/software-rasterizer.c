#include "cam.h"

bool init_cam(cam *cam) {
  if (!cam)
    return false;

  cam->pos = (vec3f){0, 0, 5};

  cam->pitch = 0.0f;
  cam->yaw = M_PI;

  cam->fov = 60.0f;

  float fov_rad = cam->fov * (M_PI / 180.0f);
  cam->focal_length = (1.0f / tanf(fov_rad / 2.0f));

  return true;
}
