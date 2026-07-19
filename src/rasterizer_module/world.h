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

#define BASE_RENDER_WIDTH 1200
#define CUT_OFF_PARALLEL_DRAWING 60
#define NEAR_PLANE 0.1f

typedef struct world_s {
  cam game_cam;
  cam debug_cam;
  cam *cam;        /* active camera — points to game_cam or debug_cam */
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
  vec3f light_dirs_cam[MAX_LIGHT_SOURCES];
} world;

bool init_world(world *world, int display_w, int display_h);
void destroy_world(world *world);
