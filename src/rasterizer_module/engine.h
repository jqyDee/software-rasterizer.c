#pragma once

#include <stdbool.h>

#include "types.h"

bool load_objs_files(world *world, char *obj_paths[], const size_t obj_count);

bool init_cam(cam *cam);
bool init_renderer(renderer *renderer);
void init_texture(renderer *renderer);
bool init_world(world *world, char *obj_paths[], const size_t obj_count);

void destroy_world(world *world);

int handle_user_input(cam *cam, const float delta_time);
