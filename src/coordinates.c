#include "types.h"
#include "vec.h"

bool project(const world *world, const vec3f world_pos, vec3f *out) {
  const vec3f cam_space_pos = vec_sub(world_pos, world->cam->pos);

  if (cam_space_pos.z <= 0.001f)
    return false;

  float x_proj = (cam_space_pos.x / cam_space_pos.z) * world->cam->focal_length;
  float y_proj = (cam_space_pos.y / cam_space_pos.z) * world->cam->focal_length *
                 world->renderer->aspect_ratio;

  if (x_proj < -1 || x_proj > 1 || y_proj < -1 || y_proj > 1)
    return false;

  out->x = (int)((x_proj + 1.0f) * 0.5f * world->renderer->screen_width);
  out->y = (int)((1.0f - y_proj) * 0.5f * world->renderer->screen_height);
  out->z = 0.0f;

  return true;
}
