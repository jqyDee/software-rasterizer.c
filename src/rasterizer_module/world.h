#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "assets/mesh.h"
#include "engine/cam.h"
#include "engine/settings.h"
#include "engine/undo.h"
#include "math/vec.h"
#include "renderer/lighting.h"
#include "renderer/renderer.h"
#include "game/kart.h"
#include "game/kart_tuning.h"
#include "game/track.h"
#include "ui/perf.h"

#define BASE_RENDER_WIDTH 2000
#define CUT_OFF_PARALLEL_DRAWING 400
#define NEAR_PLANE 0.1f

typedef struct world_s {
  cam game_cams[MAX_KART_COUNT];
  cam debug_cam;
  cam *cam;        /* active camera — points to game_cams or debug_cam */
  float player_vy; /* vertical velocity for game-camera jump */

  kart *karts;
  size_t kart_count;
  kart_tuning kart_tuning;
  track track_data;

  renderer *renderer;

  mesh *mesh_data;
  size_t mesh_data_count;

  mesh_instance *instances;
  size_t instance_count;
  size_t instance_capacity;

  /* object->world transform cache: camera-independent, so split-screen's
   * extra viewport passes would otherwise redo it per kart. Reset once per
   * frame (reset_world_vertex_cache) and filled lazily per-instance on
   * first touch by build_screen_tris. */
  vec3f *world_verts_cache;
  size_t world_verts_cache_cap; /* capacity of world_verts_cache, in vec3f */
  size_t *instance_vert_offset; /* per-instance offset into world_verts_cache */
  bool *instance_vert_cached;   /* per-instance: filled this frame? */
  size_t instance_offset_cap;   /* capacity of the two arrays above */

  texture_entry *tex_lib;
  int tex_lib_count;

  int selected_instance; /* -1 = none */
  settings settings;
  perf_stats perf;

  undo_frame_t undo_stack[MAX_UNDO];
  int undo_len;
  undo_frame_t redo_stack[MAX_UNDO];
  int redo_len;

  light_t lights[MAX_LIGHT_SOURCES];
  size_t light_count;
  /* one row per active viewport (kart index) — split-screen renders each
   * viewport through its own camera, so each needs its own camera-space
   * light directions; row 0 is used for the single-viewport/debug-cam case */
  vec3f light_dirs_cam[MAX_KART_COUNT][MAX_LIGHT_SOURCES];
} world;

bool init_world(world *world, int display_w, int display_h);
void destroy_world(world *world);
