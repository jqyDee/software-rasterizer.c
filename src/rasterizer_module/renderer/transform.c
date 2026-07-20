#include "transform.h"

#include <stdbool.h>

#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../math/vec.h"

void compute_instance_basis(const mesh_instance *inst, transform_basis *out) {
  float rx = inst->rotation.x * DEG_TO_RAD;
  float ry = inst->rotation.y * DEG_TO_RAD;
  float rz = inst->rotation.z * DEG_TO_RAD;

  out->right = (vec3f){inst->scale.x, 0, 0};
  out->right = rotate_z_snapped(out->right, rz);
  out->right = rotate_x_snapped(out->right, rx);
  out->right = rotate_y_snapped(out->right, ry);

  out->up = (vec3f){0, inst->scale.y, 0};
  out->up = rotate_z_snapped(out->up, rz);
  out->up = rotate_x_snapped(out->up, rx);
  out->up = rotate_y_snapped(out->up, ry);

  out->fwd = (vec3f){0, 0, inst->scale.z};
  out->fwd = rotate_z_snapped(out->fwd, rz);
  out->fwd = rotate_x_snapped(out->fwd, rx);
  out->fwd = rotate_y_snapped(out->fwd, ry);

  out->pos = inst->pos;

  out->n_right = (vec3f){inst->scale.x != 0.0f ? 1.0f / inst->scale.x : 1.0f, 0, 0};
  out->n_right = rotate_z_snapped(out->n_right, rz);
  out->n_right = rotate_x_snapped(out->n_right, rx);
  out->n_right = rotate_y_snapped(out->n_right, ry);

  out->n_up = (vec3f){0, inst->scale.y != 0.0f ? 1.0f / inst->scale.y : 1.0f, 0};
  out->n_up = rotate_z_snapped(out->n_up, rz);
  out->n_up = rotate_x_snapped(out->n_up, rx);
  out->n_up = rotate_y_snapped(out->n_up, ry);

  out->n_fwd = (vec3f){0, 0, inst->scale.z != 0.0f ? 1.0f / inst->scale.z : 1.0f};
  out->n_fwd = rotate_z_snapped(out->n_fwd, rz);
  out->n_fwd = rotate_x_snapped(out->n_fwd, rx);
  out->n_fwd = rotate_y_snapped(out->n_fwd, ry);
}

void transform_triangle_with_basis(const mesh *mesh_data, size_t triangle_index, const transform_basis *basis, vec3f out[3]) {
  for (int j = 0; j < 3; j++) {
    vec3f src = mesh_data->vertices[triangle_index + j];
    out[j] = (vec3f){
      src.x * basis->right.x + src.y * basis->up.x + src.z * basis->fwd.x + basis->pos.x,
      src.x * basis->right.y + src.y * basis->up.y + src.z * basis->fwd.y + basis->pos.y,
      src.x * basis->right.z + src.y * basis->up.z + src.z * basis->fwd.z + basis->pos.z
    };
  }
}

void transform_normal_with_basis(vec3f normal, const transform_basis *basis, const cam *cam, vec3f *out) {
  vec3f v = (vec3f){
    normal.x * basis->n_right.x + normal.y * basis->n_up.x + normal.z * basis->n_fwd.x,
    normal.x * basis->n_right.y + normal.y * basis->n_up.y + normal.z * basis->n_fwd.y,
    normal.x * basis->n_right.z + normal.y * basis->n_up.z + normal.z * basis->n_fwd.z
  };

  v = rotate_y(v, -cam->yaw);
  v = rotate_x(v, -cam->pitch);
  *out = vec_normalize(v);
}

void transform_triangle_to_world(const mesh *mesh_data, size_t triangle_index,
                                 const mesh_instance *inst, vec3f out[3]) {
  transform_basis basis;
  compute_instance_basis(inst, &basis);
  transform_triangle_with_basis(mesh_data, triangle_index, &basis, out);
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
