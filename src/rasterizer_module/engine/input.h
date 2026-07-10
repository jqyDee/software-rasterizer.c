#pragma once

#include "../world.h"

typedef enum user_input_response_e {
  UIR_NONE,
  UIR_RELOAD_PLUGIN,
  UIR_RESET_CAM_AND_POSITION,
} user_input_response;

#define Y_SLOP 0.01f
#define Y_SLOP_X2 Y_SLOP * 2.0f

user_input_response handle_user_input(world *world, const float delta_time);
