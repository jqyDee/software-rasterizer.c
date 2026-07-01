#include <assert.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

#include "coordinates.h"
#include "types.h"
#include "vec.h"

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

Color hsv_to_rgb(float h, float s, float v) {
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = v - c;
  float r, g, b;

  if (h < 60) {
    r = c;
    g = x;
    b = 0;
  } else if (h < 120) {
    r = x;
    g = c;
    b = 0;
  } else if (h < 180) {
    r = 0;
    g = c;
    b = x;
  } else if (h < 240) {
    r = 0;
    g = x;
    b = c;
  } else if (h < 300) {
    r = x;
    g = 0;
    b = c;
  } else {
    r = c;
    g = 0;
    b = x;
  }

  Color color = {(unsigned char)((r + m) * 255), (unsigned char)((g + m) * 255),
                 (unsigned char)((b + m) * 255), 255};
  return color;
}

static bool transform_triangle_to_camera(const mesh *mesh, size_t i,
                                         vec3f pos, const cam *cam,
                                         vec3f out[3]) {
  for (int j = 0; j < 3; j++) {
    vec3f relative_pos = vec_sub(vec_add(mesh->vertices[i + j], pos), cam->pos);
    out[j] = rotate_vector(relative_pos, -cam->pitch, -cam->yaw);
  }
  return true;
}

bool is_backfacing(const vec3f triangleVerts[3]) {
  vec3f edge1 = vec3f_sub(triangleVerts[1], triangleVerts[0]);
  vec3f edge2 = vec3f_sub(triangleVerts[2], triangleVerts[0]);

  vec3f normal = vec_cross(edge1, edge2);
  normal = vec_normalize(normal);

  vec3f centroid = {
      (triangleVerts[0].x + triangleVerts[1].x + triangleVerts[2].x) / 3.0f,
      (triangleVerts[0].y + triangleVerts[1].y + triangleVerts[2].y) / 3.0f,
      (triangleVerts[0].z + triangleVerts[1].z + triangleVerts[2].z) / 3.0f,
  };
  vec3f view_dir = vec_normalize(vec3f_scale(centroid, -1.0f));

  float dot_nv = vec_dot(normal, view_dir);

  return (dot_nv < 0.0f);
}

static void compute_triangle_bbox(const vec3f v1, const vec3f v2,
                                  const vec3f v3, int *startX, int *endX,
                                  int *startY, int *endY, int screen_width,
                                  int screen_height) {
  const float BIAS = 0.0f;

  float minX = fminf(fminf(v1.x, v2.x), v3.x);
  float maxX = fmaxf(fmaxf(v1.x, v2.x), v3.x);
  float minY = fminf(fminf(v1.y, v2.y), v3.y);
  float maxY = fmaxf(fmaxf(v1.y, v2.y), v3.y);

  *startX = (int)fmaxf(0.0f, floorf(fminf(minX, screen_width - 1.0f) - BIAS));
  *endX = (int)fminf(screen_width - 1.0f, ceilf(fmaxf(maxX, 0.0f) + BIAS));
  *startY = (int)fmaxf(0.0f, floorf(fminf(minY, screen_height - 1.0f) - BIAS));
  *endY = (int)fminf(screen_height - 1.0f, ceilf(fmaxf(maxY, 0.0f) + BIAS));
}

static void draw_triangle_pixels(world *world, const vec3f v1, const vec3f v2,
                                 const vec3f v3, Color color) {
  int screen_width = world->renderer->screen_width;
  int screen_height = world->renderer->screen_height;

  int startX, endX, startY, endY;
  compute_triangle_bbox(v1, v2, v3, &startX, &endX, &startY, &endY,
                        screen_width, screen_height);

  float *depthbuffer = world->renderer->depthbuffer;
  Color *framebuffer = world->renderer->framebuffer;

  #pragma omp parallel for schedule(static) if(endY - startY > world->settings.parallel_cutoff_rows)
  for (int row = startY; row <= endY; row++) {
    float u, v, w;
    for (int col = startX; col <= endX; col++) {
      vec3f p = {col + 0.5f, row + 0.5f, 0.f};

      if (point_in_triangle(p, v1, v2, v3, &u, &v, &w)) {
        float pixel_depth = u * v1.z + v * v2.z + w * v3.z;
        int pixel_index = row * screen_width + col;

        if (pixel_depth < depthbuffer[pixel_index]) {
          depthbuffer[pixel_index] = pixel_depth;
          framebuffer[pixel_index] = color;
        }
      }
    }
  }
}

void render_mesh(world *world, const mesh_instance *inst) {
  const mesh *mesh = inst->mesh;
  assert((mesh->vertex_count % 3) == 0);

  for (size_t i = 0; i + 2 < mesh->vertex_count; i += 3) {
    vec3f v_cam[3];
    if (!transform_triangle_to_camera(mesh, i, inst->pos, world->cam, v_cam))
      continue;

    if (is_backfacing(v_cam))
      continue;

    vec3f clipped[4];
    int clipped_count = clip_triangle_near_plane(v_cam, world->settings.near_plane, clipped);
    if (clipped_count < 3)
      continue;

    int triangle_index = i / 3;
    float hue = fmodf((float)triangle_index * 10.0f, 360.0f);
    Color color = hsv_to_rgb(hue, 1.0f, 1.0f);

    for (int t = 0; t + 2 < clipped_count; t++) {
      vec3f fan[3] = {clipped[0], clipped[t + 1], clipped[t + 2]};
      vec3f projected[3];

      for (int j = 0; j < 3; j++)
        project_cam(world, fan[j], &projected[j]);

      float dx01 = projected[0].x - projected[1].x;
      float dy01 = projected[0].y - projected[1].y;
      float dx12 = projected[1].x - projected[2].x;
      float dy12 = projected[1].y - projected[2].y;
      float dx20 = projected[2].x - projected[0].x;
      float dy20 = projected[2].y - projected[0].y;
      if (dx01 * dx01 + dy01 * dy01 < 1e-2f)
        continue;
      if (dx12 * dx12 + dy12 * dy12 < 1e-2f)
        continue;
      if (dx20 * dx20 + dy20 * dy20 < 1e-2f)
        continue;

      draw_triangle_pixels(world, projected[0], projected[1], projected[2],
                           color);
    }
  }
}

void render_world(world *world) {
  for (size_t i = 0; i < world->instance_count; i++) {
    render_mesh(world, &world->instances[i]);
  }
}
