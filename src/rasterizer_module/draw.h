#pragma once

#include "raylib.h"
#include "types.h"

void render_world(world *world);

void render_mesh(world *world, const mesh_instance *inst);

void clear_framebuffer(renderer *renderer, const Color clearColor);
void clear_depthbuffer(renderer *renderer);
