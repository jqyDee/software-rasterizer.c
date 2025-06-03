#pragma once

#include "raylib.h"
#include "types.h"

void render_world(world *world);
void render_mesh(const struct mesh_s mesh, Color *framebuffer,
                 float *depthbuffer, const cam cam,
                 int screen_width, int screen_height);
void clear_framebuffer(renderer *renderer, Color clearColor);
void clear_depthbuffer(renderer *renderer);
