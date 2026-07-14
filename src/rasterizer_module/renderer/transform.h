#pragma once

#include <stddef.h>

#include "../assets/mesh.h"
#include "../engine/cam.h"
#include "../math/vec.h"

void transform_triangle_to_world(const mesh *mesh_data, size_t triangle_index,
                                 const mesh_instance *inst, vec3f out[3]);
void transform_triangle_to_camera(const mesh *mesh_data, size_t triangle_index,
                                  const mesh_instance *inst, const cam *cam,
                                  vec3f out[3]);
vec3f transform_normal_to_camera(vec3f normal, const mesh_instance *instance,
                                 const cam *cam);

/* Clips positions, UVs, normals, and shadow factors against the near plane
   simultaneously. Uses the same interpolation parameter t as the position
   clipper. */
int clip_triangle_near_plane_uv(const vec3f verts[3], const vec2f uvs_in[3],
                                const vec3f normals_in[3],
                                const float shadow_in[3], float near,
                                vec3f out_pos[4], vec2f out_uvs[4],
                                vec3f out_normals[4], float out_shadow[4]);
