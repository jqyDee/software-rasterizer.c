#include "geometry.h"
#include "../testing.h"

#include <draw.h>
#include <float.h>
#include <math.h>
#include <renderer.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../math/color.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/transformation.h"

INTERNAL void package_screen_tri(screen_tri_t *screen_triangle,
                                 const vec3f proj[3], const vec2f uvs[3],
                                 const mesh_instance *instance,
                                 const bool has_tex, const Color flat_color,
                                 const int instance_id, const int bx0,
                                 const int bx1, const int by0, const int by1) {
  screen_triangle->v[0] = proj[0];
  screen_triangle->v[1] = proj[1];
  screen_triangle->v[2] = proj[2];
  screen_triangle->uv[0] = uvs[0];
  screen_triangle->uv[1] = uvs[1];
  screen_triangle->uv[2] = uvs[2];
  screen_triangle->inv_z[0] = 1.0f / proj[0].z;
  screen_triangle->inv_z[1] = 1.0f / proj[1].z;
  screen_triangle->inv_z[2] = 1.0f / proj[2].z;
  screen_triangle->tex = has_tex ? instance->tex_pixels : NULL;
  screen_triangle->tex_w = instance->tex_w;
  screen_triangle->tex_h = instance->tex_h;
  screen_triangle->flat_color = flat_color;
  screen_triangle->instance_id = (int)instance_id;
  screen_triangle->bx0 = bx0;
  screen_triangle->by0 = by0;
  screen_triangle->bx1 = bx1;
  screen_triangle->by1 = by1;
}

INTERNAL_INLINE bool area_collapsed(const vec3f proj[3]) {
  float area = (proj[2].x - proj[0].x) * (proj[1].y - proj[0].y) -
               (proj[2].y - proj[0].y) * (proj[1].x - proj[0].x);
  return (fabsf(area) < AREA_THRESHOLD);
}

INTERNAL_INLINE bool bbox_collapsed(const int *bx0, const int *bx1,
                                    const int *by0, const int *by1) {
  return *bx0 > *bx1 || *by0 > *by1;
}

INTERNAL_INLINE bool edge_collapsed(const vec3f proj[3]) {
  float dx01 = proj[0].x - proj[1].x, dy01 = proj[0].y - proj[1].y;
  float dx12 = proj[1].x - proj[2].x, dy12 = proj[1].y - proj[2].y;
  float dx20 = proj[2].x - proj[0].x, dy20 = proj[2].y - proj[0].y;
  return ((dx01 * dx01 + dy01 * dy01 < EDGE_THRESHOLD) ||
          (dx12 * dx12 + dy12 * dy12 < EDGE_THRESHOLD) ||
          (dx20 * dx20 + dy20 * dy20 < EDGE_THRESHOLD));
}

/* Clips both positions and UVs against the near plane simultaneously.
   Uses the same interpolation parameter t as the position clipper. */
INTERNAL int clip_triangle_near_plane_uv(const vec3f verts[3],
                                         const vec2f uvs_in[3], float near,
                                         vec3f out_pos[4], vec2f out_uvs[4]) {
  int out_count = 0;
  for (int i = 0; i < 3; i++) {
    vec3f curr = verts[i];
    vec3f prev = verts[(i + 2) % 3]; // prev vertex without going neg.
    vec2f uv_curr = uvs_in[i];
    vec2f uv_prev = uvs_in[(i + 2) % 3]; // prev vertex without going neg.

    bool curr_inside = curr.z >= near;
    bool prev_inside = prev.z >= near;

    if (curr_inside) {
      // curr is in front of near plane

      // edge from prev to curr is coming from off screen.
      // add a new vert at the screen edge, by finding the factor
      // clip_t where the line intersects with the near plane.
      if (!prev_inside) {
        // if this is the case we get a quad
        float clip_t = (near - prev.z) / (curr.z - prev.z);
        out_pos[out_count] = (vec3f){
            prev.x + clip_t * (curr.x - prev.x),
            prev.y + clip_t * (curr.y - prev.y),
            near,
        };
        out_uvs[out_count] = (vec2f){
            uv_prev.x + clip_t * (uv_curr.x - uv_prev.x),
            uv_prev.y + clip_t * (uv_curr.y - uv_prev.y),
        };
        out_count++;
      }
      // add the curr vert as on near plane
      out_pos[out_count] = curr;
      out_uvs[out_count] = uv_curr;
      out_count++;
    } else if (prev_inside) {
      // curr is off screen, prev on screen
      float clip_t = (near - prev.z) / (curr.z - prev.z);
      out_pos[out_count] = (vec3f){
          prev.x + clip_t * (curr.x - prev.x),
          prev.y + clip_t * (curr.y - prev.y),
          near,
      };
      out_uvs[out_count] = (vec2f){
          uv_prev.x + clip_t * (uv_curr.x - uv_prev.x),
          uv_prev.y + clip_t * (uv_curr.y - uv_prev.y),
      };
      out_count++;
    }
  }
  return out_count;
}

INTERNAL void compute_bbox(const vec3f proj[3], const int screen_width,
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

INTERNAL void triangle_coordinates_to_tile_coordinates(
    const screen_tri_t *screen_triangle, const int tiles_x, const int tiles_y,
    int *tx0, int *tx1, int *ty0, int *ty1) {
  // convert tri bbox to tile coordinates
  *tx0 = screen_triangle->bx0 / TILE_SIZE;
  *ty0 = screen_triangle->by0 / TILE_SIZE;
  *tx1 = screen_triangle->bx1 / TILE_SIZE,
  *ty1 = screen_triangle->by1 / TILE_SIZE;

  // clamp to valid range
  if (*tx1 >= tiles_x) {
    *tx1 = tiles_x - 1;
  }
  if (*ty1 >= tiles_y) {
    *ty1 = tiles_y - 1;
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

void tile_coordinates_to_triangle_coordinates(const int tile_index,
                                              const int tiles_x,
                                              const int screen_width,
                                              const int screen_height, int *x0,
                                              int *x1, int *y0, int *y1) {
  int x = tile_index % tiles_x, y = tile_index / tiles_x;

  *x0 = x * TILE_SIZE;
  *y0 = y * TILE_SIZE;
  *x1 = *x0 + TILE_SIZE - 1;

  if (*x1 >= screen_width) {
    *x1 = screen_width - 1;
  }

  *y1 = *y0 + TILE_SIZE - 1;
  if (*y1 >= screen_height) {
    *y1 = screen_height - 1;
  }
}

bool is_backfacing(const vec3f cam_space_verts[3]) {
  // calculate orthogonal vec to the triangle
  vec3f edge1 = vec3f_sub(cam_space_verts[1], cam_space_verts[0]);
  vec3f edge2 = vec3f_sub(cam_space_verts[2], cam_space_verts[0]);
  vec3f normal = vec_cross(edge1, edge2);

  normal = vec_normalize(normal);

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

void transform_triangle_to_camera(const mesh *mesh_data, size_t triangle_index,
                                  const mesh_instance *inst, const cam *cam,
                                  vec3f out[3]) {
  const mesh *mesh = &mesh_data[inst->mesh_idx];

  for (int j = 0; j < 3; j++) {
    vec3f src = mesh->vertices[triangle_index + j];

    // OBJECT SPACE -> WORLD SPACE
    // scale objects vectors first
    vec3f v = {src.x * inst->scale.x, src.y * inst->scale.y,
               src.z * inst->scale.z};
    // roll -> yaw -> pitch
    v = rotate_z_snapped(v, inst->rotation.z * DEG_TO_RAD); // roll
    v = rotate_x_snapped(v, inst->rotation.x * DEG_TO_RAD); // pitch
    v = rotate_y_snapped(v, inst->rotation.y * DEG_TO_RAD); // yaw
    vec3f world_pos = vec_add(v, inst->pos);                // translate

    // WORLD SPACE -> CAMERA SPACE
    vec3f relative_pos = vec_sub(world_pos, cam->pos); // translate

    relative_pos = rotate_y(relative_pos, -cam->yaw); // rotate yaw
    out[j] = rotate_x(relative_pos, -cam->pitch);     // rotate pitch
  }
}

void compute_triangles_per_tile(
    int tile_count_buf[MAX_TILES], const int num_tiles,
    const int screen_triangles_count,
    const screen_tri_t screen_triangles[MAX_SCREEN_TRIS], const int tiles_x,
    const int tiles_y) {
  memset(tile_count_buf, 0, (size_t)num_tiles * sizeof(int));
  for (int screen_triangle_id = 0; screen_triangle_id < screen_triangles_count;
       screen_triangle_id++) {
    int tx0, tx1, ty0, ty1;
    triangle_coordinates_to_tile_coordinates(
        &screen_triangles[screen_triangle_id], tiles_x, tiles_y, &tx0, &tx1,
        &ty0, &ty1);

    // compute the num of triangles for each tile
    for (int ty = ty0; ty <= ty1; ty++)
      for (int tx = tx0; tx <= tx1; tx++)
        tile_count_buf[ty * tiles_x + tx]++;
  }
}

void compute_tile_starts(int tile_start_buf[MAX_TILES],
                         const int tile_count_buf[MAX_TILES],
                         const int num_tiles) {
  int total_bin = 0;
  for (int tile_index = 0; tile_index < num_tiles; tile_index++) {
    tile_start_buf[tile_index] = total_bin;
    total_bin += tile_count_buf[tile_index];
    if (total_bin > MAX_BIN_ENTRIES) {
      total_bin = MAX_BIN_ENTRIES;
      break;
    }
  }
}

void bin_triangles_into_tiles(int tile_count_buf[MAX_TILES],
                              screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                              int bin_buf[MAX_BIN_ENTRIES],
                              const int tile_start_buf[MAX_TILES],
                              const int num_tiles,
                              const int screen_triangles_count,
                              const int tiles_x, const int tiles_y) {
  memset(tile_count_buf, 0, (size_t)num_tiles * sizeof(int));
  for (int screen_triangle_id = 0; screen_triangle_id < screen_triangles_count;
       screen_triangle_id++) {
    int tx0, tx1, ty0, ty1;
    triangle_coordinates_to_tile_coordinates(
        &screen_triangles[screen_triangle_id], tiles_x, tiles_y, &tx0, &tx1,
        &ty0, &ty1);

    for (int ty = ty0; ty <= ty1; ty++) {
      for (int tx = tx0; tx <= tx1; tx++) {
        int tile_id = ty * tiles_x + tx;
        int idx = tile_start_buf[tile_id] + tile_count_buf[tile_id];
        if (idx < MAX_BIN_ENTRIES) {
          bin_buf[idx] = screen_triangle_id;
          tile_count_buf[tile_id]++;
        }
      }
    }
  }
}

int build_screen_tris(world *world,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS]) {
  int screen_triangles_count = 0;
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

      if (is_backfacing(v_cam))
        continue;

      vec2f tri_uvs[3] = {{0, 0}, {0, 0}, {0, 0}};
      get_uvs(mesh->uvs, vertex_id, tri_uvs);

      vec3f clipped_pos[4];
      vec2f clipped_uvs[4];
      int clipped_vertex_num = clip_triangle_near_plane_uv(
          v_cam, tri_uvs, world->settings.near_plane, clipped_pos, clipped_uvs);

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
        package_screen_tri(st, proj, uvs, instance, has_tex, flat_color,
                           instance_id, bx0, bx1, by0, by1);

        st->fp_bias = compute_fp_edge_bias(proj, world->renderer->screen_width,
                                           world->renderer->screen_height,
                                           world->settings.seam_bias);
      }
    }
  }
  return screen_triangles_count;
}
