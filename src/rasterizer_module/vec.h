#pragma once

#include <math.h>

#include "types.h"

#define vec_add(a, b)                                                          \
  _Generic((a),                                                                \
      vec3f: vec3f_add)(a, b)

#define vec_sub(a, b)                                                          \
  _Generic((a),                                                                \
      vec3f: vec3f_sub)(a, b)

#define vec_dot(a, b)                                                          \
  _Generic((a),                                                                \
      vec3f: vec3f_dot)(a, b)

#define vec_scale(a, s)                                                        \
  _Generic((a),                                                                \
      vec3f: vec3f_scale)(a, s)

#define vec_normalize(a)                                                    \
  _Generic((a),                                                                \
      vec3f: vec3f_normalize)(a)

#define vec_cross(a, s)                                                        \
  _Generic((a),                                                                \
      vec3f: vec3f_cross)(a, s)

// -------------- VECTOR MATH --------------

static inline vec3f vec3f_add(vec3f a, vec3f b) {
  return (vec3f){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3f vec3f_sub(vec3f a, vec3f b) {
  return (vec3f){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline float vec3f_dot(vec3f a, vec3f b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3f vec3f_scale(vec3f v, float s) {
  return (vec3f){v.x * s, v.y * s, v.z * s};
}

static inline vec3f vec3f_cross(vec3f a, vec3f b) {
  return (vec3f){
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x,
  };
}

static inline vec3f vec3f_normalize(vec3f v) {
  float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  if (length > 0.00001f) {
    return (vec3f){v.x / length, v.y / length, v.z / length};
  }
  return (vec3f){0, 0, 0};  // Avoid division by zero
}
