#include "rotation.h"
#include "transformation.h"

#include <math.h>

static void sincos_snap(float rad, float *sine, float *cosine) {
  float deg = rad * RAD_TO_DEG;
  // snap to nearest 90 deg rotation
  float nearest_snap = roundf(deg / 90.0f) * 90.0f;

  // if not snapped just calc sine and cosine directly
  if (fabsf(deg - nearest_snap) > SNAP_THRESHOLD) {
    *sine = sinf(rad);
    *cosine = cosf(rad);
    return;
  }

  // check which quadrant we snapped to
  int quadrant = ((int)roundf(nearest_snap / 90.0f)) % 4;

  // negative check for negative rotation
  if (quadrant < 0)
    quadrant += 4;

  // lookup snapped values
  *sine = sine_lookup[quadrant];
  *cosine = cosine_lookup[quadrant];
}

// ============== CAM ROTATIONS ==============
vec3f rotate_x(vec3f vec, float pitch) {
  float cosine = cosf(pitch), sine = sinf(pitch);
  return (vec3f){
      vec.x,
      vec.y * cosine - vec.z * sine,
      vec.y * sine + vec.z * cosine,
  };
}

vec3f rotate_y(vec3f vec, float yaw) {
  float cosine = cosf(yaw), sine = sinf(yaw);
  return (vec3f){
      vec.x * cosine + vec.z * sine,
      vec.y,
      -vec.x * sine + vec.z * cosine,
  };
}

vec3f rotate_z(vec3f vec, float roll) {
  float cosine = cosf(roll), sine = sinf(roll);
  return (vec3f){
      vec.x * cosine - vec.y * sine,
      vec.x * sine + vec.y * cosine,
      vec.z,
  };
}

// ============== INSTANCE ROTATIONS ==============
vec3f rotate_x_snapped(vec3f vec, float pitch) {
  float cosine, sine;
  sincos_snap(pitch, &sine, &cosine);

  return (vec3f){
      vec.x,
      vec.y * cosine - vec.z * sine,
      vec.y * sine + vec.z * cosine,
  };
}

vec3f rotate_y_snapped(vec3f vec, float yaw) {
  float cosine, sine;
  sincos_snap(yaw, &sine, &cosine);

  return (vec3f){
      vec.x * cosine + vec.z * sine,
      vec.y,
      -vec.x * sine + vec.z * cosine,
  };
}

vec3f rotate_z_snapped(vec3f vec, float roll) {
  float cosine, sine;
  sincos_snap(roll, &sine, &cosine);
  return (vec3f){
      vec.x * cosine - vec.y * sine,
      vec.x * sine + vec.y * cosine,
      vec.z,
  };
}
