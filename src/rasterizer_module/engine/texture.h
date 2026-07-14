#pragma once
#include "../world.h"

void free_texture_library(world *world);
void load_texture_library(world *world);
void apply_lib_texture(world *world, int inst_idx, int lib_idx);

void init_textures(renderer *renderer);
void destroy_textures(renderer *renderer);
