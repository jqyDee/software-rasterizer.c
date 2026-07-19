#pragma once

#include "../world.h"
#include "gui_state.h"

void draw_object_window(world *world, gui_state_t *gs);
void draw_gizmo(world *world);
void draw_collision_boxes(const world *world);
void draw_normals(const world *world);
