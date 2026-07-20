#include "geometry.h"
#include "../testing.h"

#include <float.h>
#include <math.h>
#include <omp.h>
#include <renderer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../math/color.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/transformation.h"
#include "lighting.h"
#include "shadow.h"
#include "transform.h"

#define MAX_THREADS 16

static screen_tri_t thread_local_tris[MAX_THREADS][MAX_SCREEN_TRIS];
static int thread_tri_counts[MAX_THREADS];
static double thread_transform_time[MAX_THREADS];

INTERNAL void package_screen_tri(screen_tri_t *screen_triangle,
                                 const vec3f proj[3], const vec2f uvs[3],
                                 const vec3f normals[3],
                                 const mesh_instance *instance,
                                 const bool has_tex, const Color flat_color,
                                 const vec3f light_rgb, const int instance_id,
                                 const int bx0, const int bx1, const int by0,
                                 const int by1, const float st_shadow[3]) {
  screen_triangle->v[0] = proj[0];
  screen_triangle->v[1] = proj[1];
  screen_triangle->v[2] = proj[2];
  screen_triangle->uv[0] = uvs[0];
  screen_triangle->uv[1] = uvs[1];
  screen_triangle->uv[2] = uvs[2];
  screen_triangle->normals[0] = normals[0];
  screen_triangle->normals[1] = normals[1];
  screen_triangle->normals[2] = normals[2];
  screen_triangle->inv_z[0] = 1.0f / proj[0].z;
  screen_triangle->inv_z[1] = 1.0f / proj[1].z;
  screen_triangle->inv_z[2] = 1.0f / proj[2].z;
  screen_triangle->tex = has_tex ? instance->tex_pixels : NULL;
  screen_triangle->tex_w = instance->tex_w;
  screen_triangle->tex_h = instance->tex_h;
  screen_triangle->flat_color = flat_color;
  screen_triangle->light_rgb = light_rgb;
  screen_triangle->instance_id = (int)instance_id;
  screen_triangle->bx0 = bx0;
  screen_triangle->by0 = by0;
  screen_triangle->bx1 = bx1;
  screen_triangle->by1 = by1;
  screen_triangle->shadow[0] = st_shadow[0];
  screen_triangle->shadow[1] = st_shadow[1];
  screen_triangle->shadow[2] = st_shadow[2];
}

bool area_collapsed(const vec3f proj[3]) {
  float area = (proj[2].x - proj[0].x) * (proj[1].y - proj[0].y) -
               (proj[2].y - proj[0].y) * (proj[1].x - proj[0].x);
  return (fabsf(area) < AREA_THRESHOLD);
}

bool bbox_collapsed(const int *bx0, const int *bx1, const int *by0,
                    const int *by1) {
  return *bx0 > *bx1 || *by0 > *by1;
}

bool edge_collapsed(const vec3f proj[3]) {
  float dx01 = proj[0].x - proj[1].x, dy01 = proj[0].y - proj[1].y;
  float dx12 = proj[1].x - proj[2].x, dy12 = proj[1].y - proj[2].y;
  float dx20 = proj[2].x - proj[0].x, dy20 = proj[2].y - proj[0].y;
  return ((dx01 * dx01 + dy01 * dy01 < EDGE_THRESHOLD) ||
          (dx12 * dx12 + dy12 * dy12 < EDGE_THRESHOLD) ||
          (dx20 * dx20 + dy20 * dy20 < EDGE_THRESHOLD));
}

void compute_bbox(const vec3f proj[3], int x0, int y0, int x1, int y1, int *bx0,
                  int *bx1, int *by0, int *by1) {
  float mnx = fminf(fminf(proj[0].x, proj[1].x), proj[2].x);
  float mny = fminf(fminf(proj[0].y, proj[1].y), proj[2].y);
  float mxx = fmaxf(fmaxf(proj[0].x, proj[1].x), proj[2].x);
  float mxy = fmaxf(fmaxf(proj[0].y, proj[1].y), proj[2].y);
  *bx0 = (int)fmaxf((float)x0, floorf(mnx));
  *by0 = (int)fmaxf((float)y0, floorf(mny));
  *bx1 = (int)fminf((float)x1, ceilf(mxx));
  *by1 = (int)fminf((float)y1, ceilf(mxy));
}

INTERNAL_INLINE void get_uvs(const vec2f *uvs, size_t triangle_id,
                             vec2f out[3]) {
  if (uvs) {
    out[0] = uvs[triangle_id];
    out[1] = uvs[triangle_id + 1];
    out[2] = uvs[triangle_id + 2];
  }
}

INTERNAL float compute_fp_edge_bias(const vec3f proj[3],
                                    const int viewport_width,
                                    const int viewport_height,
                                    const float seam_bias) {
  float dx_w0 = proj[2].y - proj[1].y, dy_w0 = proj[1].x - proj[2].x;
  float dx_w1 = proj[0].y - proj[2].y, dy_w1 = proj[2].x - proj[0].x;
  float dx_w2 = proj[1].y - proj[0].y, dy_w2 = proj[0].x - proj[1].x;

  float b0 = (fabsf(proj[1].x) + (float)viewport_width) * fabsf(dx_w0) +
             (fabsf(proj[1].y) + (float)viewport_height) * fabsf(dy_w0);
  float b1 = (fabsf(proj[2].x) + (float)viewport_width) * fabsf(dx_w1) +
             (fabsf(proj[2].y) + (float)viewport_height) * fabsf(dy_w1);
  float b2 = (fabsf(proj[0].x) + (float)viewport_width) * fabsf(dx_w2) +
             (fabsf(proj[0].y) + (float)viewport_height) * fabsf(dy_w2);

  return fmaxf(fmaxf(b0, b1), b2) * 2.0f * (float)DBL_EPSILON + seam_bias;
}

vec3f compute_face_normal(const vec3f cam_space_verts[3]) {
  vec3f edge1 = vec3f_sub(cam_space_verts[1], cam_space_verts[0]);
  vec3f edge2 = vec3f_sub(cam_space_verts[2], cam_space_verts[0]);
  return vec_normalize(vec_cross(edge1, edge2));
}

bool is_backfacing(const vec3f cam_space_verts[3], const vec3f normal) {
  // find center of triangle
  vec3f centroid = {
      (cam_space_verts[0].x + cam_space_verts[1].x + cam_space_verts[2].x) /
          3.0f,
      (cam_space_verts[0].y + cam_space_verts[1].y + cam_space_verts[2].y) /
          3.0f,
      (cam_space_verts[0].z + cam_space_verts[1].z + cam_space_verts[2].z) /
          3.0f,
  };

  // as cam is at origin (we are in cam space), we can calculate the view
  // direction from cam to triangle center
  vec3f view_dir = vec_normalize(vec3f_scale(centroid, -1.0f));

  // if normal of triangle faces away from cam (< 0) it is facing away from cam
  return vec_dot(normal, view_dir) < 0.0f;
}

// Conservative bounding-sphere-vs-frustum test, run once per instance
// instead of once per triangle: split-screen means this whole function runs
// once per kart, and without this check every instance's full triangle list
// gets transformed/lit/clipped/projected for every viewport regardless of
// whether that viewport can even see it. False positives (keeping something
// actually out of view) are fine — this only needs to be safe against false
// negatives (culling something that should be visible).
INTERNAL_INLINE bool instance_potentially_visible(const mesh *m,
                                                  const mesh_instance *inst,
                                                  const cam *cam, float near,
                                                  const viewport *vp) {
  vec3f local_center = vec_scale(vec_add(m->aabb_min, m->aabb_max), 0.5f);
  vec3f half_extent = vec_scale(vec_sub(m->aabb_max, m->aabb_min), 0.5f);
  float max_scale = fmaxf(inst->scale.x, fmaxf(inst->scale.y, inst->scale.z));
  float radius =
      sqrtf(half_extent.x * half_extent.x + half_extent.y * half_extent.y +
            half_extent.z * half_extent.z) *
      max_scale;

  // OBJECT -> WORLD (center point only — same order as
  // transform_triangle_to_world, just skipping the per-vertex loop)
  vec3f c = {local_center.x * inst->scale.x, local_center.y * inst->scale.y,
             local_center.z * inst->scale.z};
  c = rotate_z_snapped(c, inst->rotation.z * DEG_TO_RAD);
  c = rotate_x_snapped(c, inst->rotation.x * DEG_TO_RAD);
  c = rotate_y_snapped(c, inst->rotation.y * DEG_TO_RAD);
  c = vec_add(c, inst->pos);

  // WORLD -> CAMERA
  vec3f rel = vec_sub(c, cam->pos);
  rel = rotate_y(rel, -cam->yaw);
  rel = rotate_x(rel, -cam->pitch);

  if (rel.z + radius < near) {
    return false; // entirely behind the near plane
  }

  // per-axis cone test: a point at depth z is inside the horizontal FOV if
  // |x| <= z/focal_length (from project()'s own x_proj formula). Expanding
  // the threshold by `radius` directly (rather than projecting the sphere
  // properly) only ever over-includes near the frustum edges, never
  // under-includes.
  float z = fmaxf(rel.z, near);
  float x_limit = z / cam->focal_length + radius;
  float y_limit = z / (cam->focal_length * vp->aspect_ratio) + radius;
  if (fabsf(rel.x) > x_limit) {
    return false;
  }
  if (fabsf(rel.y) > y_limit) {
    return false;
  }
  return true;
}

void reset_world_vertex_cache(world *world) {
  size_t n = world->instance_count;

  if (n > world->instance_offset_cap) {
    size_t new_cap = n * 2;
    world->instance_vert_offset =
        realloc(world->instance_vert_offset, new_cap * sizeof(size_t));
    world->instance_vert_cached =
        realloc(world->instance_vert_cached, new_cap * sizeof(bool));
    world->instance_offset_cap = new_cap;
  }

  size_t total = 0;
  for (size_t i = 0; i < n; i++) {
    world->instance_vert_offset[i] = total;
    total += world->mesh_data[world->instances[i].mesh_idx].vertex_count;
  }

  if (total > world->world_verts_cache_cap) {
    size_t new_cap = total * 2;
    world->world_verts_cache =
        realloc(world->world_verts_cache, new_cap * sizeof(vec3f));
    world->world_verts_cache_cap = new_cap;
  }

  if (n > 0) {
    memset(world->instance_vert_cached, 0, n * sizeof(bool));
  }
}

int build_screen_tris(world *world, const viewport *vp, int viewport_index,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS],
                      double *xform_time) {
  int screen_triangles_count = 0;

  compute_light_dirs_cam(world, viewport_index);
  memset(thread_tri_counts, 0, sizeof(thread_tri_counts));
  memset(thread_transform_time, 0, sizeof(thread_transform_time));

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int local_count = 0;
    screen_tri_t *local_buf = thread_local_tris[tid];

#pragma omp for schedule(dynamic)
    for (size_t instance_id = 0; instance_id < world->instance_count;
         instance_id++) {
      const mesh_instance *instance = &world->instances[instance_id];
      const mesh *mesh = &world->mesh_data[instance->mesh_idx];

      if (!instance_potentially_visible(mesh, instance, world->cam,
                                        world->settings.near_plane, vp))
        continue;

      bool has_tex = (mesh->uvs != NULL && instance->tex_pixels != NULL);

      transform_basis basis;
      compute_instance_basis(instance, &basis);

      // object->world transform is camera-independent — cache it per
      // instance per frame so split-screen's extra viewport passes don't
      // redo it (see PERF_RENDER_GEOM_XFORM; instance_potentially_visible
      // above already limits this to instances this viewport can see, but
      // an instance visible from multiple viewports is only transformed
      // by whichever one touches it first).
      vec3f *instance_world_verts =
          &world->world_verts_cache[world->instance_vert_offset[instance_id]];
      if (!world->instance_vert_cached[instance_id]) {
        double _xform_t0 = GetTime();
        for (size_t vertex_id = 0; vertex_id + 2 < mesh->vertex_count;
             vertex_id += 3) {
          transform_triangle_with_basis(mesh, vertex_id, &basis,
                                        &instance_world_verts[vertex_id]);
        }
        thread_transform_time[tid] += (GetTime() - _xform_t0);
        world->instance_vert_cached[instance_id] = true;
      }

      for (size_t vertex_id = 0;
           vertex_id + 2 < mesh->vertex_count && local_count < MAX_SCREEN_TRIS;
           vertex_id += 3) {

        const vec3f *v_world = &instance_world_verts[vertex_id];

        // 2. transform from world space to camera space
        vec3f v_cam[3];
        for (int j = 0; j < 3; j++) {
          vec3f relative_pos = vec_sub(v_world[j], world->cam->pos);
          relative_pos = rotate_y(relative_pos, -world->cam->yaw);
          v_cam[j] = rotate_x(relative_pos, -world->cam->pitch);
        }

        // 3. shadow
        float st_shadow[3];
        if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
          for (int k = 0; k < 3; k++) {
            st_shadow[k] = sample_shadow(world->renderer, v_world[k]);
          }
        } else {
          st_shadow[0] = st_shadow[1] = st_shadow[2] = 1.0f;
        }

        vec3f normal = compute_face_normal(v_cam);
        if (is_backfacing(v_cam, normal)) {
          continue;
        }

        vec3f vertex_normals[3];
        if (mesh->normals) {
          for (int idx = 0; idx < 3; idx++) {
            transform_normal_with_basis(mesh->normals[vertex_id + idx], &basis,
                                        world->cam, &vertex_normals[idx]);
          }
        } else {
          vertex_normals[0] = vertex_normals[1] = vertex_normals[2] = normal;
        }

        float ambient = world->settings.ambient_light;
        vec3f light_rgb = (vec3f){ambient, ambient, ambient};
        // lighting (Lambertian)
        if (world->settings.lighting_mode != LIGHTING_GPU_PHONG) {
          for (size_t light_id = 0; light_id < world->light_count; light_id++) {
            float ndotl = fmaxf(
                0.0f, vec_dot(normal,
                              world->light_dirs_cam[viewport_index][light_id]));
            float diffuse = ndotl * world->lights[light_id].intensity;
            light_rgb.x += (world->lights[light_id].color.r / 255.0f) * diffuse;
            light_rgb.y += (world->lights[light_id].color.g / 255.0f) * diffuse;
            light_rgb.z += (world->lights[light_id].color.b / 255.0f) * diffuse;
          }
        }

        vec2f tri_uvs[3] = {{0, 0}, {0, 0}, {0, 0}};
        get_uvs(mesh->uvs, vertex_id, tri_uvs);

        vec3f clipped_pos[4];
        vec2f clipped_uvs[4];
        vec3f clipped_normals[4];
        float clipped_shadow[4];
        int clipped_vertex_num = clip_triangle_near_plane_uv(
            v_cam, tri_uvs, vertex_normals, st_shadow,
            world->settings.near_plane, clipped_pos, clipped_uvs,
            clipped_normals, clipped_shadow);

        // skip if 2 or less (as 2 verts cannot draw triangle)
        if (clipped_vertex_num < 3)
          continue;

        // debug coloring (every triangle, slightly different hue)
        int triangle_idx = (int)(vertex_id / 3);
        Color flat_color =
            hsv_to_rgb(fmodf((float)triangle_idx * 10.0f, 360.0f), 1.0f, 1.0f);

        for (int t = 0; t + 2 < clipped_vertex_num; t++) {
          if (local_count >= MAX_SCREEN_TRIS)
            break;
          // QUAD
          //  x1 ------------ x2
          //   |              |       => triangle1 = [1, 2, 3]; triangle2 =
          //   [1, 3, 4];
          //  x4 ------------ x3
          //
          // TRIANGLE
          //  x1 ------------ x2
          //   \_______        |       => triangle1 = [1, 2, 3]; triangle2 not
          //   needed and skipped
          //           \______x3
          vec3f positions[3] = {clipped_pos[0], clipped_pos[t + 1],
                                clipped_pos[t + 2]};
          vec2f uvs[3] = {clipped_uvs[0], clipped_uvs[t + 1],
                          clipped_uvs[t + 2]};
          vec3f tri_normals[3] = {clipped_normals[0], clipped_normals[t + 1],
                                  clipped_normals[t + 2]};
          float tri_shadow[3] = {clipped_shadow[0], clipped_shadow[t + 1],
                                 clipped_shadow[t + 2]};

          // project to screen space
          vec3f proj[3];
          for (int j = 0; j < 3; j++) {
            project(world->cam, vp, positions[j], &proj[j]);
          }

          // check triangle collapsed
          if (edge_collapsed(proj))
            continue;
          if (area_collapsed(proj))
            continue;

          int bx0, bx1, by0, by1;
          compute_bbox(proj, vp->x, vp->y, vp->x + vp->w - 1, vp->y + vp->h - 1,
                       &bx0, &bx1, &by0, &by1);
          if (bbox_collapsed(&bx0, &bx1, &by0, &by1))
            continue;

          screen_tri_t *st = &local_buf[local_count++];
          package_screen_tri(st, proj, uvs, tri_normals, instance, has_tex,
                             flat_color, light_rgb, instance_id, bx0, bx1, by0,
                             by1, tri_shadow);

          st->fp_bias = compute_fp_edge_bias(proj, vp->w, vp->h,
                                             world->settings.seam_bias);
        }
      }
    }
    thread_tri_counts[tid] = local_count;
  }

  int num_threads = omp_get_max_threads();
  if (num_threads > MAX_THREADS) {
    num_threads = MAX_THREADS;
  }

  for (int t = 0; t < num_threads; t++) {
    *xform_time += thread_transform_time[t];
  }

  int thread_offsets[MAX_THREADS];

  // Compute global offsets sequentially
  for (int t = 0; t < num_threads; t++) {
    thread_offsets[t] = screen_triangles_count;
    screen_triangles_count += thread_tri_counts[t];
  }

  if (screen_triangles_count > MAX_SCREEN_TRIS) {
    screen_triangles_count = MAX_SCREEN_TRIS; // Clamp global total
  }

  // Parallel bulk copy
#pragma omp parallel for schedule(static)
  for (int t = 0; t < num_threads; t++) {
    int start = thread_offsets[t];
    int count = thread_tri_counts[t];

    if (start >= MAX_SCREEN_TRIS) {
      count = 0;
    } else if (start + count > MAX_SCREEN_TRIS) {
      count = MAX_SCREEN_TRIS - start;
    }

    if (count > 0) {
      memcpy(&screen_triangles[start], thread_local_tris[t],
             count * sizeof(screen_tri_t));
    }
  }

  return screen_triangles_count;
}
