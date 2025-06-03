#include "raylib.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

float edgeFunction(vec3f a, vec3f b, vec3f c) {
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

bool point_in_triangle(vec3f p, vec3f a, vec3f b, vec3f c) {
  float area = edgeFunction(a, b, c);
  if (fabsf(area) < EPSILON)
    return false; // Degenerate triangle

  float w0 = edgeFunction(b, c, p);
  float w1 = edgeFunction(c, a, p);
  float w2 = edgeFunction(a, b, p);

  float u = w0 / area;
  float v = w1 / area;
  float w = w2 / area;

  if (u >= 0 && v >= 0 && w >= 0) {
    return true;
  }
  return false;
}

void draw_mesh(const struct mesh_s mesh, Color *framebuffer) {
  assert((mesh.vertex_count % 3) == 0);

  for (size_t veci = 0; veci + 2 < mesh.vertex_count; veci += 3) {

    vec3f v1 = mesh.positions[veci];
    vec3f v2 = mesh.positions[veci + 1];
    vec3f v3 = mesh.positions[veci + 2];

    // Bounding box
    float minX = fminf(fminf(v1.x, v2.x), v3.x);
    float maxX = fmaxf(fmaxf(v1.x, v2.x), v3.x);
    float minY = fminf(fminf(v1.y, v2.y), v3.y);
    float maxY = fmaxf(fmaxf(v1.y, v2.y), v3.y);

    // Clamp to screen bounds
    int startX = (int)fmaxf(0, floorf(minX));
    int endX = (int)fminf(SCREEN_WIDTH - 1, ceilf(maxX));
    int startY = (int)fmaxf(0, floorf(minY));
    int endY = (int)fminf(SCREEN_HEIGHT - 1, ceilf(maxY));

    for (int row = startY; row <= endY; row++) {
      for (int col = startX; col <= endX; col++) {
        vec3f p = {col + 0.5f, row + 0.5f, 0.f};

        if (point_in_triangle(p, v1, v2, v3)) {
          float r = (veci);
          float g = (veci + 1);
          float b = (veci + 2);
          Color color = {r * 255, g * 200, b * 100, 255};
          framebuffer[row * SCREEN_WIDTH + col] = color;
        }
      }
    }
  }
}
