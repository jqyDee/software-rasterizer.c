#pragma once

#include <stdbool.h>

#include "../world.h"

bool scene_save(const world *world, const char *path);
bool scene_load(world *world, const char *path);
