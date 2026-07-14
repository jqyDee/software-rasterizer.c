#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../assets/mesh.h"
#include "../engine/cam.h"
#include "../math/vec.h"
#include "../world.h"
#include "renderer.h"

#define BB_PAD 0.0f
#define AREA_THRESHOLD 0.5f
#define EDGE_THRESHOLD 1e-2f

#define EPSILON 1e-2f

vec3f compute_face_normal(const vec3f cam_space_verts[3]);
bool is_backfacing(const vec3f cam_space_verts[3], const vec3f normal);

// not INTERNAL/static: shared with shadow.c's draw_shadow_map
void compute_bbox(const vec3f proj[3], const int screen_width,
                  const int screen_height, int *bx0, int *bx1, int *by0,
                  int *by1);
bool bbox_collapsed(const int *bx0, const int *bx1, const int *by0,
                    const int *by1);
bool area_collapsed(const vec3f proj[3]);
bool edge_collapsed(const vec3f proj[3]);

int build_screen_tris(world *world,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS]);
