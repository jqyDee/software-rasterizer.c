#pragma once

#include <stdbool.h>

#include "types.h"

bool load_objs_files(world *world, char *obj_paths[], const size_t obj_count);

bool init_cam(cam *cam);
void init_texture(renderer *renderer);
void resize_renderer(world *world);
void resize_renderer_to(world *world, int display_w, int display_h);
bool init_world(world *world, char *obj_paths[], const size_t obj_count,
                int display_w, int display_h);

void destroy_world(world *world);

int handle_user_input(world *world, const float delta_time);
