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

void compute_bbox(const vec3f proj[3], int x0, int y0, int x1, int y1, int *bx0,
                  int *bx1, int *by0, int *by1);
bool bbox_collapsed(const int *bx0, const int *bx1, const int *by0,
                    const int *by1);
bool area_collapsed(const vec3f proj[3]);
bool edge_collapsed(const vec3f proj[3]);

// xform_time is accumulated into (+=), not overwritten — caller resets once
// per frame and sums across every viewport's call, mirroring t_geom/t_bin/
// t_raster in render().
// max_out is the number of slots remaining in screen_triangles starting at
// the pointer passed in (not MAX_SCREEN_TRIS) — callers append multiple
// viewports' triangles into one shared array via a running offset, so this
// must reflect only what's actually left, or a later viewport's call could
// write past the real end of the array.
int build_screen_tris(world *world, const viewport *vp, int viewport_index,
                      struct screen_tri_s screen_triangles[MAX_SCREEN_TRIS],
                      int max_out, double *xform_time);

// call once per frame, before the first build_screen_tris call: (re)sizes
// world's object->world transform cache and marks every instance's slot
// stale so the first viewport that touches it this frame refills it.
void reset_world_vertex_cache(world *world);
