#include "projection.h"

void project(const cam *cam, const renderer *renderer, vec3f cam_pos,
             vec3f *out) {
  // translate x and y between -1 and 1 clipping anything outside the view
  float x_proj = (cam_pos.x / cam_pos.z) * cam->focal_length;
  float y_proj =
      (cam_pos.y / cam_pos.z) * cam->focal_length * renderer->aspect_ratio;

  // map the translated x and y coordinates to actual screen space
  // x is positive to the right so add
  // y is positive to the bottom so subract
  out->x = (x_proj + 1.0f) * 0.5f * renderer->screen_width;
  out->y = (1.0f - y_proj) * 0.5f * renderer->screen_height;
  // z stays z (needed for the depthbuffer)
  out->z = cam_pos.z;
}
