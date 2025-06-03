#pragma once

#include "types.h"

#define vec_add(a, b)                                                          \
  _Generic((a),                                                                \
      vec2f: vec2f_add,                                                        \
      vec3f: vec3f_add,                                                        \
      vec2: vec2_add,                                                          \
      vec3: vec3_add)(a, b)

#define vec_sub(a, b)                                                          \
  _Generic((a),                                                                \
      vec2f: vec2f_sub,                                                        \
      vec3f: vec3f_sub,                                                        \
      vec2: vec2_sub,                                                          \
      vec3: vec3_sub)(a, b)

#define vec_dot(a, b)                                                          \
  _Generic((a),                                                                \
      vec2f: vec2f_dot,                                                        \
      vec3f: vec3f_dot,                                                        \
      vec2: vec2_dot,                                                          \
      vec3: vec3_dot)(a, b)

// -------------- vec2 --------------
static inline vec2f vec2f_add(vec2f a, vec2f b) {
  return (vec2f){a.x + b.x, a.y + b.y};
}

static inline vec2 vec2_add(vec2 a, vec2 b) {
  return (vec2){a.x + b.x, a.y + b.y};
}

static inline vec2f vec2f_sub(vec2f a, vec2f b) {
  return (vec2f){a.x - b.x, a.y - b.y};
}

static inline vec2 vec2_sub(vec2 a, vec2 b) {
  return (vec2){a.x - b.x, a.y - b.y};
}

static inline float vec2f_dot(vec2f a, vec2f b) {
  return a.x * b.x + a.y * b.y;
}

static inline int vec2_dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }

// -------------- vec3 --------------
static inline vec3f vec3f_add(vec3f a, vec3f b) {
  return (vec3f){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3 vec3_add(vec3 a, vec3 b) {
  return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3f vec3f_sub(vec3f a, vec3f b) {
  return (vec3f){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3 vec3_sub(vec3 a, vec3 b) {
  return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline float vec3f_dot(vec3f a, vec3f b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline int vec3_dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

