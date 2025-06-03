#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

#include "coordinates.h"
#include "types.h"

#define EPSILON 1e-5f

void clear_framebuffer(renderer *renderer, const Color clearColor) {
  for (int i = 0; i < renderer->screen_width * renderer->screen_height; i++) {
    renderer->framebuffer[i] = clearColor;
  }
}

void clear_depthbuffer(renderer *renderer) {
  for (int i = 0; i < renderer->screen_width * renderer->screen_height; i++) {
    renderer->depthbuffer[i] = INFINITY;
  }
}

static inline float edgeFunction(vec3f a, vec3f b, vec3f c) {
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static inline bool point_in_triangle(const vec3f p, const vec3f a,
                                     const vec3f b, const vec3f c, float *u,
                                     float *v, float *w) {
  float area = edgeFunction(a, b, c);
  if (fabsf(area) < EPSILON)
    return false; // Degenerate triangle

  float w0 = edgeFunction(b, c, p);
  float w1 = edgeFunction(c, a, p);
  float w2 = edgeFunction(a, b, p);

  *u = w0 / area;
  *v = w1 / area;
  *w = w2 / area;

  if (*u >= 0 && *v >= 0 && *w >= 0) {
    return true;
  }
  return false;
}

void render_mesh(world *world, const mesh mesh) {
  assert((mesh.vertex_count % 3) == 0);

  for (size_t veci = 0; veci + 2 < mesh.vertex_count; veci += 3) {

    vec3f v1, v2, v3;
    if (!project(world, mesh.vertices[veci], &v1) ||
        !project(world, mesh.vertices[veci + 1], &v2) ||
        !project(world, mesh.vertices[veci + 2], &v3)) {
      continue;
    }

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

    // coloring
    int triangle_index = veci / 3;
    Color color = {
        (unsigned char)((triangle_index * 50) % 256),  // R
        (unsigned char)((triangle_index * 80) % 256),  // G
        (unsigned char)((triangle_index * 130) % 256), // B
        255                                            // A
    };

    float u, v, w;
    for (int row = startY; row <= endY; row++) {
      for (int col = startX; col <= endX; col++) {
        vec3f p = {col + 0.5f, row + 0.5f, 0.f};

        if (point_in_triangle(p, v1, v2, v3, &u, &v, &w)) {
          float pixel_depth = u * mesh.vertices[veci].z +
                              v * mesh.vertices[veci + 1].z +
                              w * mesh.vertices[veci + 2].z;

          int pixel_index = row * world->renderer->screen_width + col;

          if (pixel_depth < world->renderer->depthbuffer[pixel_index]) {
            world->renderer->depthbuffer[pixel_index] = pixel_depth;
            world->renderer->framebuffer[pixel_index] = color;
          }
        }
      }
    }
  }
}

void render_world(world *world) {
  for (size_t i = 0; i < world->mesh_count; i++) {
    render_mesh(world, world->meshes[i]);
  }
}

