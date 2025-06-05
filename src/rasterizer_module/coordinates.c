#include <math.h>
#include <stdio.h>

#include "types.h"
#include "vec.h"

vec3f rotate_vector(const vec3f v, const float pitch, const float yaw) {
  // Calculate cosines and sines
  float cosPitch = cosf(pitch);
  float sinPitch = sinf(pitch);
  float cosYaw = cosf(yaw);
  float sinYaw = sinf(yaw);

  // Yaw rotation around Y axis
  vec3f vYaw = {v.x * cosYaw + v.z * sinYaw, v.y, -v.x * sinYaw + v.z * cosYaw};

  // Pitch rotation around X axis
  vec3f vPitch = {vYaw.x, vYaw.y * cosPitch - vYaw.z * sinPitch,
                  vYaw.y * sinPitch + vYaw.z * cosPitch};

  return vPitch;
}

static inline float edgeFunction(vec3f a, vec3f b, vec3f c) {
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static inline bool is_top_left_edge(vec3f a, vec3f b) {
  return (a.y == b.y && a.x > b.x) || (a.y < b.y);
}

bool point_in_triangle(const vec3f p, const vec3f a, const vec3f b,
                       const vec3f c, float *u, float *v, float *w) {
  float area = edgeFunction(a, b, c);

  if (fabsf(area) < EPSILON)
    return false; // Degenerate triangle

  float w0 = edgeFunction(b, c, p);
  float w1 = edgeFunction(c, a, p);
  float w2 = edgeFunction(a, b, p);

  *u = w0 / area;
  *v = w1 / area;
  *w = w2 / area;

  bool inside = (*u > 0 || (*u == 0 && is_top_left_edge(b, c))) &&
                (*v > 0 || (*v == 0 && is_top_left_edge(c, a))) &&
                (*w > 0 || (*w == 0 && is_top_left_edge(a, b)));

  return inside;
}

bool project(const world *world, const vec3f world_pos, vec3f *out) {
  const vec3f relative_pos = vec_sub(world_pos, world->cam->pos);
  vec3f cam_space_pos =
      rotate_vector(relative_pos, -(world->cam->pitch), -(world->cam->yaw));

  const float near_plane = 0.001f;

  if (cam_space_pos.z <= near_plane) {
    cam_space_pos.z = near_plane;
  }

  float x_proj = (cam_space_pos.x / cam_space_pos.z) * world->cam->focal_length;
  float y_proj = (cam_space_pos.y / cam_space_pos.z) *
                 world->cam->focal_length * world->renderer->aspect_ratio;

  if (x_proj < -1 || x_proj > 1 || y_proj < -1 || y_proj > 1) {
    return false;
  }

  // Use floats for subpixel precision
  out->x = (x_proj + 1.0f) * 0.5f * world->renderer->screen_width;
  out->y = (1.0f - y_proj) * 0.5f * world->renderer->screen_height;
  out->z = cam_space_pos.z; // Pass camera space depth for interpolation

  return true;
}
