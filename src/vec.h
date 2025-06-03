#pragma once

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

