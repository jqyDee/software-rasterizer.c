#pragma once

#include <stdbool.h>

typedef struct world_s world;

#define Y_SLOP 0.01f
#define Y_SLOP_X2 Y_SLOP * 2.0f

typedef enum user_input_response_e {
  UIR_NONE,
  UIR_RELOAD_PLUGIN,
  UIR_RESET_CAM_AND_POSITION,
} user_input_response;

typedef struct controller_input_s {
    float steering_input; // -1..1
    float accel_input;    // -1(brake); 1(accel)
    bool jump_requested;
    bool drift_requested;
} controller_input;

user_input_response handle_user_input(world *world, const float delta_time);
