#include "primitives.h"
#include "../testing.h"

#include <stdlib.h>
#include <stdio.h>

mesh make_cube_mesh(void) {
  static const vec3f verts[36] = {
      /* near face  (z=-0.5) normal (0,0,-1) */
      {-0.5f, -0.5f, -0.5f},
      {-0.5f, 0.5f, -0.5f},
      {0.5f, 0.5f, -0.5f},
      {-0.5f, -0.5f, -0.5f},
      {0.5f, 0.5f, -0.5f},
      {0.5f, -0.5f, -0.5f},
      /* far face   (z=+0.5) normal (0,0,+1) */
      {-0.5f, -0.5f, 0.5f},
      {0.5f, -0.5f, 0.5f},
      {0.5f, 0.5f, 0.5f},
      {-0.5f, -0.5f, 0.5f},
      {0.5f, 0.5f, 0.5f},
      {-0.5f, 0.5f, 0.5f},
      /* right face (x=+0.5) normal (+1,0,0) */
      {0.5f, -0.5f, -0.5f},
      {0.5f, 0.5f, -0.5f},
      {0.5f, 0.5f, 0.5f},
      {0.5f, -0.5f, -0.5f},
      {0.5f, 0.5f, 0.5f},
      {0.5f, -0.5f, 0.5f},
      /* left face  (x=-0.5) normal (-1,0,0) */
      {-0.5f, -0.5f, -0.5f},
      {-0.5f, -0.5f, 0.5f},
      {-0.5f, 0.5f, 0.5f},
      {-0.5f, -0.5f, -0.5f},
      {-0.5f, 0.5f, 0.5f},
      {-0.5f, 0.5f, -0.5f},
      /* top face   (y=+0.5) normal (0,+1,0) */
      {-0.5f, 0.5f, -0.5f},
      {-0.5f, 0.5f, 0.5f},
      {0.5f, 0.5f, -0.5f},
      {-0.5f, 0.5f, 0.5f},
      {0.5f, 0.5f, 0.5f},
      {0.5f, 0.5f, -0.5f},
      /* bottom face(y=-0.5) normal (0,-1,0) */
      {-0.5f, -0.5f, -0.5f},
      {0.5f, -0.5f, -0.5f},
      {-0.5f, -0.5f, 0.5f},
      {0.5f, -0.5f, -0.5f},
      {0.5f, -0.5f, 0.5f},
      {-0.5f, -0.5f, 0.5f},
  };

  /* same UV pattern per face: bl,tl,tr then bl,tr,br */
  static const vec2f face_uv[6] = {
      {0, 1}, {0, 0}, {1, 0}, {0, 1}, {1, 0}, {1, 1},
  };
  vec3f *v = malloc(36 * sizeof(vec3f));
  vec2f *uv = malloc(36 * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  for (int i = 0; i < 36; i++) {
    v[i] = verts[i];
    uv[i] = face_uv[i % 6];
  }
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = 36};
  snprintf(out.name, sizeof(out.name), "Cube");
  return out;
}

mesh make_sphere_mesh(int rings, int segs) {
  size_t vcount = (size_t)(rings * segs * 6);
  vec3f *v = malloc(vcount * sizeof(vec3f));
  vec2f *uv = malloc(vcount * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  size_t vi = 0;
  for (int r = 0; r < rings; r++) {
    float t0 = (float)r * (float)M_PI / (float)rings;
    float t1 = (float)(r + 1) * (float)M_PI / (float)rings;
    float y0 = cosf(t0), yr0 = sinf(t0);
    float y1 = cosf(t1), yr1 = sinf(t1);
    float v0 = (float)r / (float)rings;
    float v1 = (float)(r + 1) / (float)rings;
    for (int s = 0; s < segs; s++) {
      float p0 = (float)s * 2.0f * (float)M_PI / (float)segs;
      float p1 = (float)(s + 1) * 2.0f * (float)M_PI / (float)segs;
      float u0 = (float)s / (float)segs;
      float u1 = (float)(s + 1) / (float)segs;
      vec3f v00 = {yr0 * cosf(p0), y0, yr0 * sinf(p0)};
      vec3f v01 = {yr0 * cosf(p1), y0, yr0 * sinf(p1)};
      vec3f v10 = {yr1 * cosf(p0), y1, yr1 * sinf(p0)};
      vec3f v11 = {yr1 * cosf(p1), y1, yr1 * sinf(p1)};
      /* tri A */ v[vi] = v00;
      uv[vi++] = (vec2f){u0, v0};
      v[vi] = v01;
      uv[vi++] = (vec2f){u1, v0};
      v[vi] = v11;
      uv[vi++] = (vec2f){u1, v1};
      /* tri B */ v[vi] = v00;
      uv[vi++] = (vec2f){u0, v0};
      v[vi] = v11;
      uv[vi++] = (vec2f){u1, v1};
      v[vi] = v10;
      uv[vi++] = (vec2f){u0, v1};
    }
  }
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = vi};
  snprintf(out.name, sizeof(out.name), "Sphere");
  return out;
}

mesh make_plane_mesh(void) {
  vec3f *v = malloc(6 * sizeof(vec3f));
  vec2f *uv = malloc(6 * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  /* unit XZ plane at y=0, outward normal (0,+1,0) */
  v[0] = (vec3f){-0.5f, 0, -0.5f};
  uv[0] = (vec2f){0, 0};
  v[1] = (vec3f){0.5f, 0, 0.5f};
  uv[1] = (vec2f){1, 1};
  v[2] = (vec3f){0.5f, 0, -0.5f};
  uv[2] = (vec2f){1, 0};
  v[3] = (vec3f){-0.5f, 0, -0.5f};
  uv[3] = (vec2f){0, 0};
  v[4] = (vec3f){-0.5f, 0, 0.5f};
  uv[4] = (vec2f){0, 1};
  v[5] = (vec3f){0.5f, 0, 0.5f};
  uv[5] = (vec2f){1, 1};
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = 6};
  snprintf(out.name, sizeof(out.name), "Plane");
  return out;
}

mesh make_cylinder_mesh(int segs) {
  size_t vcount = (size_t)(segs * 12);
  vec3f *v = malloc(vcount * sizeof(vec3f));
  vec2f *uv = malloc(vcount * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  size_t vi = 0;
  for (int s = 0; s < segs; s++) {
    float a0 = (float)s * 2.0f * (float)M_PI / segs;
    float a1 = (float)(s + 1) * 2.0f * (float)M_PI / segs;
    float x0 = cosf(a0) * 0.5f, z0 = sinf(a0) * 0.5f;
    float x1 = cosf(a1) * 0.5f, z1 = sinf(a1) * 0.5f;
    float u0 = (float)s / segs, u1 = (float)(s + 1) / segs;
    /* side */
    v[vi] = (vec3f){x0, -0.5f, z0};
    uv[vi++] = (vec2f){u0, 1};
    v[vi] = (vec3f){x0, 0.5f, z0};
    uv[vi++] = (vec2f){u0, 0};
    v[vi] = (vec3f){x1, 0.5f, z1};
    uv[vi++] = (vec2f){u1, 0};
    v[vi] = (vec3f){x0, -0.5f, z0};
    uv[vi++] = (vec2f){u0, 1};
    v[vi] = (vec3f){x1, 0.5f, z1};
    uv[vi++] = (vec2f){u1, 0};
    v[vi] = (vec3f){x1, -0.5f, z1};
    uv[vi++] = (vec2f){u1, 1};
    /* top cap */
    v[vi] = (vec3f){0, 0.5f, 0};
    uv[vi++] = (vec2f){0.5f, 0.5f};
    v[vi] = (vec3f){x1, 0.5f, z1};
    uv[vi++] = (vec2f){cosf(a1) * 0.5f + 0.5f, sinf(a1) * 0.5f + 0.5f};
    v[vi] = (vec3f){x0, 0.5f, z0};
    uv[vi++] = (vec2f){cosf(a0) * 0.5f + 0.5f, sinf(a0) * 0.5f + 0.5f};
    /* bottom cap */
    v[vi] = (vec3f){0, -0.5f, 0};
    uv[vi++] = (vec2f){0.5f, 0.5f};
    v[vi] = (vec3f){x0, -0.5f, z0};
    uv[vi++] = (vec2f){cosf(a0) * 0.5f + 0.5f, sinf(a0) * 0.5f + 0.5f};
    v[vi] = (vec3f){x1, -0.5f, z1};
    uv[vi++] = (vec2f){cosf(a1) * 0.5f + 0.5f, sinf(a1) * 0.5f + 0.5f};
  }
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = vi};
  snprintf(out.name, sizeof(out.name), "Cylinder");
  return out;
}

mesh make_cone_mesh(int segs) {
  size_t vcount = (size_t)(segs * 6);
  vec3f *v = malloc(vcount * sizeof(vec3f));
  vec2f *uv = malloc(vcount * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  size_t vi = 0;
  for (int s = 0; s < segs; s++) {
    float a0 = (float)s * 2.0f * (float)M_PI / segs;
    float a1 = (float)(s + 1) * 2.0f * (float)M_PI / segs;
    float x0 = cosf(a0) * 0.5f, z0 = sinf(a0) * 0.5f;
    float x1 = cosf(a1) * 0.5f, z1 = sinf(a1) * 0.5f;
    float u0 = (float)s / segs, u1 = (float)(s + 1) / segs;
    /* side: tip → base */
    v[vi] = (vec3f){0, 0.5f, 0};
    uv[vi++] = (vec2f){(u0 + u1) * 0.5f, 0};
    v[vi] = (vec3f){x1, -0.5f, z1};
    uv[vi++] = (vec2f){u1, 1};
    v[vi] = (vec3f){x0, -0.5f, z0};
    uv[vi++] = (vec2f){u0, 1};
    /* base cap */
    v[vi] = (vec3f){0, -0.5f, 0};
    uv[vi++] = (vec2f){0.5f, 0.5f};
    v[vi] = (vec3f){x0, -0.5f, z0};
    uv[vi++] = (vec2f){cosf(a0) * 0.5f + 0.5f, sinf(a0) * 0.5f + 0.5f};
    v[vi] = (vec3f){x1, -0.5f, z1};
    uv[vi++] = (vec2f){cosf(a1) * 0.5f + 0.5f, sinf(a1) * 0.5f + 0.5f};
  }
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = vi};
  snprintf(out.name, sizeof(out.name), "Cone");
  return out;
}

mesh make_torus_mesh(int rings, int segs) {
  size_t vcount = (size_t)(rings * segs * 6);
  vec3f *v = malloc(vcount * sizeof(vec3f));
  vec2f *uv = malloc(vcount * sizeof(vec2f));
  if (!v || !uv) {
    free(v);
    free(uv);
    return (mesh){0};
  }
  const float R = 0.35f, r = 0.15f;
  size_t vi = 0;
  for (int ri = 0; ri < rings; ri++) {
    float a0 = (float)ri * 2.0f * (float)M_PI / rings;
    float a1 = (float)(ri + 1) * 2.0f * (float)M_PI / rings;
    float ua0 = (float)ri / rings, ua1 = (float)(ri + 1) / rings;
    for (int si = 0; si < segs; si++) {
      float b0 = (float)si * 2.0f * (float)M_PI / segs;
      float b1 = (float)(si + 1) * 2.0f * (float)M_PI / segs;
      float vb0 = (float)si / segs, vb1 = (float)(si + 1) / segs;
#define TV(a, b)                                                               \
  (vec3f){(R + r * cosf(b)) * cosf(a), r * sinf(b), (R + r * cosf(b)) * sinf(a)}
      vec3f v00 = TV(a0, b0), v01 = TV(a0, b1), v10 = TV(a1, b0),
            v11 = TV(a1, b1);
#undef TV
      v[vi] = v00;
      uv[vi++] = (vec2f){ua0, vb0};
      v[vi] = v11;
      uv[vi++] = (vec2f){ua1, vb1};
      v[vi] = v10;
      uv[vi++] = (vec2f){ua1, vb0};
      v[vi] = v00;
      uv[vi++] = (vec2f){ua0, vb0};
      v[vi] = v01;
      uv[vi++] = (vec2f){ua0, vb1};
      v[vi] = v11;
      uv[vi++] = (vec2f){ua1, vb1};
    }
  }
  mesh out = {.vertices = v, .uvs = uv, .vertex_count = vi};
  snprintf(out.name, sizeof(out.name), "Torus");
  return out;
}
