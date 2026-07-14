#include "transform.h"

#include <stdbool.h>

#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../math/vec.h"

void transform_triangle_to_world(const mesh *mesh_data, size_t triangle_index,
                                 const mesh_instance *inst, vec3f out[3]) {
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

    out[j] = vec_add(v, inst->pos); // translate
  }
}

void transform_triangle_to_camera(const mesh *mesh_data, size_t triangle_index,
                                  const mesh_instance *inst, const cam *cam,
                                  vec3f out[3]) {
  vec3f world_pos[3];
  transform_triangle_to_world(mesh_data, triangle_index, inst, world_pos);

  for (int j = 0; j < 3; j++) {
    // WORLD SPACE -> CAMERA SPACE
    vec3f relative_pos = vec_sub(world_pos[j], cam->pos); // translate

    relative_pos = rotate_y(relative_pos, -cam->yaw); // rotate yaw
    out[j] = rotate_x(relative_pos, -cam->pitch);     // rotate pitch
  }
}

vec3f transform_normal_to_camera(vec3f normal, const mesh_instance *instance,
                                 const cam *cam) {
  vec3f v = (vec3f){
      instance->scale.x != 0.0f ? normal.x / instance->scale.x : normal.x,
      instance->scale.y != 0.0f ? normal.y / instance->scale.y : normal.y,
      instance->scale.z != 0.0f ? normal.z / instance->scale.z : normal.z,
  };
  v = rotate_z_snapped(v, instance->rotation.z * DEG_TO_RAD); // roll
  v = rotate_x_snapped(v, instance->rotation.x * DEG_TO_RAD); // pith
  v = rotate_y_snapped(v, instance->rotation.y * DEG_TO_RAD); // yaw
  // no translation as only direction, and not position

  // WORLD SPACE -> CAM SPACE
  v = rotate_y(v, -cam->yaw);
  v = rotate_x(v, -cam->pitch);

  return vec_normalize(v);
}

int clip_triangle_near_plane_uv(const vec3f verts[3], const vec2f uvs_in[3],
                                const vec3f normals_in[3],
                                const float shadow_in[3], float near,
                                vec3f out_pos[4], vec2f out_uvs[4],
                                vec3f out_normals[4], float out_shadow[4]) {
  int out_count = 0;
  for (int i = 0; i < 3; i++) {
    const int prev_idx = (i + 2) % 3;

    vec3f curr = verts[i],
          prev = verts[prev_idx]; // prev vertex without going neg.

    vec2f uv_curr = uvs_in[i],
          uv_prev = uvs_in[prev_idx]; // prev vertex without going neg.

    vec3f n_curr = normals_in[i],
          n_prev = normals_in[prev_idx]; // prev normal without going neg.

    float shadow_curr = shadow_in[i],
          shadow_prev = shadow_in[prev_idx]; // prev shadow without going neg.

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
        out_normals[out_count] = vec_normalize(((vec3f){
            n_prev.x + clip_t * (n_curr.x - n_prev.x),
            n_prev.y + clip_t * (n_curr.y - n_prev.y),
            n_prev.z + clip_t * (n_curr.z - n_prev.z),
        }));
        out_shadow[out_count] = shadow_prev + clip_t * (shadow_curr - shadow_prev);
        out_count++;
      }
      // add the curr vert as on near plane
      out_pos[out_count] = curr;
      out_uvs[out_count] = uv_curr;
      out_normals[out_count] = n_curr;
      out_shadow[out_count] = shadow_curr;
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
      out_normals[out_count] = vec_normalize(((vec3f){
          n_prev.x + clip_t * (n_curr.x - n_prev.x),
          n_prev.y + clip_t * (n_curr.y - n_prev.y),
          n_prev.z + clip_t * (n_curr.z - n_prev.z),
      }));
      out_shadow[out_count] = shadow_prev + clip_t * (shadow_curr - shadow_prev);
      out_count++;
    }
  }
  return out_count;
}
