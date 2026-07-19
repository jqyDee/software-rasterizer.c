#pragma once

#include "../world.h"
#include "gui_state.h"

/* returns false if the hub window was just closed */
bool draw_main_window(world *world, gui_state_t *gs, float delta_time);
