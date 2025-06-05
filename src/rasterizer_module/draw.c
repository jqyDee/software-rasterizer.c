#include <assert.h>
#include <math.h>
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
                                         const cam *cam, vec3f out[3]) {
  for (int j = 0; j < 3; j++) {
    vec3f relative_pos = vec_sub(mesh->vertices[i + j], cam->pos);
    out[j] = rotate_vector(relative_pos, -cam->pitch, -cam->yaw);
  }
  return true;
}

bool is_backfacing(const vec3f triangleVerts[3]) {
  vec3f edge1 = vec3f_sub(triangleVerts[1], triangleVerts[0]);
  vec3f edge2 = vec3f_sub(triangleVerts[2], triangleVerts[0]);

  vec3f normal = vec_cross(edge1, edge2);
  normal = vec_normalize(normal);

  // Camera looks along negative Z in camera space:
  vec3f view_dir = {0.0f, 0.0f, 1.0f};

  float dot_nv =
      normal.x * view_dir.x + normal.y * view_dir.y + normal.z * view_dir.z;

  return (dot_nv >= 0.0f);
}

static bool project_triangle(world *world, const mesh *mesh, size_t i,
                             vec3f out[3]) {
  bool in = project(world, mesh->vertices[i], &out[0]) &&
            project(world, mesh->vertices[i + 1], &out[1]) &&
            project(world, mesh->vertices[i + 2], &out[2]);
  float dx01 = out[0].x - out[1].x;
  float dy01 = out[0].y - out[1].y;
  float dx12 = out[1].x - out[2].x;
  float dy12 = out[1].y - out[2].y;
  float dx20 = out[2].x - out[0].x;
  float dy20 = out[2].y - out[0].y;

  float dist2_01 = dx01 * dx01 + dy01 * dy01;
  float dist2_12 = dx12 * dx12 + dy12 * dy12;
  float dist2_20 = dx20 * dx20 + dy20 * dy20;

  if (dist2_01 < 1e-2f || dist2_12 < 1e-2f || dist2_20 < 1e-2f) {
    return false;
  }

  return in;
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

  float u, v, w;
  for (int row = startY; row <= endY; row++) {
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

void render_mesh(world *world, const mesh mesh) {
  assert((mesh.vertex_count % 3) == 0);

  for (size_t i = 0; i + 2 < mesh.vertex_count; i += 3) {
    vec3f v_cam[3];
    if (!transform_triangle_to_camera(&mesh, i, world->cam, v_cam))
      continue;
    if (is_backfacing(v_cam))
      continue;

    int triangle_index = i / 3;
    float hue = fmodf((float)triangle_index * 10.0f, 360.0f);
    Color color = hsv_to_rgb(hue, 1.0f, 1.0f);
    vec3f projected[3];
    if (!project_triangle(world, &mesh, i, projected)) {
      color = MAGENTA;
    } else if (is_backfacing(v_cam)) {
      color = RED;
    } else {
      color = GREEN;
    }


    draw_triangle_pixels(world, projected[0], projected[1], projected[2],
                         color);
  }
}

void render_world(world *world) {
  for (size_t i = 0; i < world->mesh_count; i++) {
    render_mesh(world, world->meshes[i]);
  }
}
