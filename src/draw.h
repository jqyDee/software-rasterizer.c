#pragma once

#include "raylib.h"
#include "types.h"

void render_mesh(const struct mesh_s mesh, Color *framebuffer,
                 float *depthbuffer, const cam cam,
                 int screen_width, int screen_height);
void clear_framebuffer(Color *framebuffer, int width, int height, Color clearColor);
void clear_depthbuffer(float *depthbuffer, int width, int height);
