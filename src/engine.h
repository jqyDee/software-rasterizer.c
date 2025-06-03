#pragma once

#include <stdbool.h>

#include "types.h"

bool init_cam(cam *cam);
bool init_renderer(renderer *renderer);
bool init_world(world *world, const char *obj_paths[], const size_t obj_count);

void destroy_world(world *world);

void handle_user_input(cam *cam, const float delta_time);
