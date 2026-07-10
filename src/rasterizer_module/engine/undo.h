#pragma once

#include <stddef.h>

#define MAX_UNDO 50

#include "../assets/mesh.h"

typedef struct world_s world;

typedef struct {
  size_t instance_count;
  mesh_instance *instances; /* heap copy, tex_pixels = NULL */
  int selected_instance;
} undo_frame_t;

void scene_push_undo(world *world);
void scene_undo(world *world);
void scene_redo(world *world);