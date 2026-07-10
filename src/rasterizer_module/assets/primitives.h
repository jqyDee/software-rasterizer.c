#pragma once
#include "mesh.h"

#define NUM_PRIMITIVES 6

mesh make_cube_mesh(void);
mesh make_sphere_mesh(int rings, int segs);
mesh make_plane_mesh(void);
mesh make_cylinder_mesh(int segs);
mesh make_cone_mesh(int segs);
mesh make_torus_mesh(int rings, int segs);
