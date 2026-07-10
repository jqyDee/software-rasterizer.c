#pragma once

#include "vec.h"
#include "../assets/mesh.h"

void compute_mesh_aabb(mesh *m);

bool ellipsoid_hits_aabb(vec3f center, vec3f radii, vec3f mn, vec3f mx);
bool camera_collides(const world *world, vec3f pos);