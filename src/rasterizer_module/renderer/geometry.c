#include "geometry.h"
#include "../testing.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "../math/color.h"
#include "../math/projection.h"
#include "lighting.h"
#include "shadow.h"
#include "transform.h"

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

// not INTERNAL/static: also used by shadow.c's draw_shadow_map
bool area_collapsed(const vec3f proj[3]) {
  float area = (proj[2].x - proj[0].x) * (proj[1].y - proj[0].y) -
               (proj[2].y - proj[0].y) * (proj[1].x - proj[0].x);
  return (fabsf(area) < AREA_THRESHOLD);
}

// not INTERNAL/static: also used by shadow.c's draw_shadow_map
bool bbox_collapsed(const int *bx0, const int *bx1,
                    const int *by0, const int *by1) {
  return *bx0 > *bx1 || *by0 > *by1;
}

// not INTERNAL/static: also used by shadow.c's draw_shadow_map
bool edge_collapsed(const vec3f proj[3]) {
  float dx01 = proj[0].x - proj[1].x, dy01 = proj[0].y - proj[1].y;
  float dx12 = proj[1].x - proj[2].x, dy12 = proj[1].y - proj[2].y;
  float dx20 = proj[2].x - proj[0].x, dy20 = proj[2].y - proj[0].y;
  return ((dx01 * dx01 + dy01 * dy01 < EDGE_THRESHOLD) ||
          (dx12 * dx12 + dy12 * dy12 < EDGE_THRESHOLD) ||
          (dx20 * dx20 + dy20 * dy20 < EDGE_THRESHOLD));
}

// not INTERNAL/static: also used by shadow.c's draw_shadow_map
void compute_bbox(const vec3f proj[3], const int screen_width,
                  const int screen_height, int *bx0, int *bx1,
                  int *by0, int *by1) {
  float mnx = fminf(fminf(proj[0].x, proj[1].x), proj[2].x);
  float mny = fminf(fminf(proj[0].y, proj[1].y), proj[2].y);
  float mxx = fmaxf(fmaxf(proj[0].x, proj[1].x), proj[2].x);
  float mxy = fmaxf(fmaxf(proj[0].y, proj[1].y), proj[2].y);
  *bx0 = (int)fmaxf(0.0f, floorf(mnx));
  *by0 = (int)fmaxf(0.0f, floorf(mny));
  *bx1 = (int)fminf((float)(screen_width - 1), ceilf(mxx));
  *by1 = (int)fminf((float)(screen_height - 1), ceilf(mxy));
}

INTERNAL_INLINE void get_uvs(const vec2f *uvs, size_t triangle_id,
                             vec2f out[3]) {
  if (uvs) {
    out[0] = uvs[triangle_id];
    out[1] = uvs[triangle_id + 1];
    out[2] = uvs[triangle_id + 2];
  }
}

INTERNAL float compute_fp_edge_bias(const vec3f proj[3], const int screen_width,
                                    const int screen_height,
                                    const float seam_bias) {
  float dx_w0 = proj[2].y - proj[1].y, dy_w0 = proj[1].x - proj[2].x;
  float dx_w1 = proj[0].y - proj[2].y, dy_w1 = proj[2].x - proj[0].x;
  float dx_w2 = proj[1].y - proj[0].y, dy_w2 = proj[0].x - proj[1].x;

  float b0 = (fabsf(proj[1].x) + (float)screen_width) * fabsf(dx_w0) +
             (fabsf(proj[1].y) + (float)screen_height) * fabsf(dy_w0);
  float b1 = (fabsf(proj[2].x) + (float)screen_width) * fabsf(dx_w1) +
             (fabsf(proj[2].y) + (float)screen_height) * fabsf(dy_w1);
  float b2 = (fabsf(proj[0].x) + (float)screen_width) * fabsf(dx_w2) +
             (fabsf(proj[0].y) + (float)screen_height) * fabsf(dy_w2);

  return fmaxf(fmaxf(b0, b1), b2) * 2.0f * FLT_EPSILON + seam_bias;
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

int build_screen_tris(world *world,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS]) {
  int screen_triangles_count = 0;

  compute_light_dirs_cam(world);

  for (size_t instance_id = 0; instance_id < world->instance_count;
       instance_id++) {
    const mesh_instance *instance = &world->instances[instance_id];
    const mesh *mesh = &world->mesh_data[instance->mesh_idx];

    bool has_tex = (mesh->uvs != NULL && instance->tex_pixels != NULL);

    for (size_t vertex_id = 0; vertex_id + 2 < mesh->vertex_count &&
                               screen_triangles_count < MAX_SCREEN_TRIS;
         vertex_id += 3) {

      // transform to cam space
      vec3f v_cam[3];
      transform_triangle_to_camera(world->mesh_data, vertex_id, instance,
                                   world->cam, v_cam);

      // shadow
      float st_shadow[3];
      if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
        vec3f v_world[3];
        transform_triangle_to_world(world->mesh_data, vertex_id, instance,
                                    v_world);
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
          vertex_normals[idx] = transform_normal_to_camera(
              mesh->normals[vertex_id + idx], instance, world->cam);
        }
      } else {
        vertex_normals[0] = vertex_normals[1] = vertex_normals[2] = normal;
      }

      // lighting (Lambertian)
      float ambient = world->settings.ambient_light;
      vec3f light_rgb = (vec3f){ambient, ambient, ambient};
      for (size_t light_id = 0; light_id < world->light_count; light_id++) {
        float ndotl =
            fmaxf(0.0f, vec_dot(normal, world->light_dirs_cam[light_id]));
        float diffuse = ndotl * world->lights[light_id].intensity;
        light_rgb.x += (world->lights[light_id].color.r / 255.0f) * diffuse;
        light_rgb.y += (world->lights[light_id].color.g / 255.0f) * diffuse;
        light_rgb.z += (world->lights[light_id].color.b / 255.0f) * diffuse;
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

      for (int t = 0; t + 2 < clipped_vertex_num &&
                      screen_triangles_count < MAX_SCREEN_TRIS;
           t++) {
        // QUAD
        //  x1 ------------ x2
        //   |              |       => triangle1 = [1, 2, 3]; triangle2 = [1, 3,
        //   4];
        //  x4 ------------ x3
        //
        // TRIANGLE
        //  x1 ------------ x2
        //   \_______        |       => triangle1 = [1, 2, 3]; triangle2 not
        //   needed and skipped
        //           \______x3
        vec3f positions[3] = {clipped_pos[0], clipped_pos[t + 1],
                              clipped_pos[t + 2]};
        vec2f uvs[3] = {clipped_uvs[0], clipped_uvs[t + 1], clipped_uvs[t + 2]};
        vec3f tri_normals[3] = {clipped_normals[0], clipped_normals[t + 1],
                                clipped_normals[t + 2]};
        float tri_shadow[3] = {clipped_shadow[0], clipped_shadow[t + 1],
                               clipped_shadow[t + 2]};

        // project to screen space
        vec3f proj[3];
        for (int j = 0; j < 3; j++)
          project(world->cam, world->renderer, positions[j], &proj[j]);

        // check triangle collapsed
        if (edge_collapsed(proj))
          continue;
        if (area_collapsed(proj))
          continue;

        int bx0, bx1, by0, by1;
        compute_bbox(proj, world->renderer->screen_width,
                     world->renderer->screen_height, &bx0, &bx1, &by0, &by1);
        if (bbox_collapsed(&bx0, &bx1, &by0, &by1))
          continue;

        screen_tri_t *st = &screen_triangles[screen_triangles_count++];
        package_screen_tri(st, proj, uvs, tri_normals, instance, has_tex,
                           flat_color, light_rgb, instance_id, bx0, bx1, by0,
                           by1, tri_shadow);

        st->fp_bias = compute_fp_edge_bias(proj, world->renderer->screen_width,
                                           world->renderer->screen_height,
                                           world->settings.seam_bias);
      }
    }
  }
  return screen_triangles_count;
}
