#include <math.h>

#include "raylib.h"

#include "types.h"
#include "vec.h"

#define ASPECT_RATIO ((float)SCREEN_WIDTH / SCREEN_HEIGHT)
#define FOV_RAD (FOV * (PI / 180.0f))
#define FOCAL_LENGTH (1.0f / tanf(FOV_RAD / 2.0f))

bool project(const vec3f cam_pos, const vec3f world_pos, vec3f *out) {
  const vec3f cam_space_pos = vec_sub(world_pos, cam_pos);

  if (cam_space_pos.z <= 0.001f)
    return false;

  float x_proj = (cam_space_pos.x / cam_space_pos.z) * FOCAL_LENGTH;
  float y_proj =
      (cam_space_pos.y / cam_space_pos.z) * FOCAL_LENGTH * ASPECT_RATIO;

  if (x_proj < -1 || x_proj > 1 || y_proj < -1 || y_proj > 1)
    return false;

  out->x = (int)((x_proj + 1.0f) * 0.5f * SCREEN_WIDTH);
  out->y = (int)((1.0f - y_proj) * 0.5f * SCREEN_HEIGHT);
  out->z = 0.0f;

  return true;
}
