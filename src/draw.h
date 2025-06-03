#pragma once

#include "raylib.h"
#include "types.h"

void render_mesh(const struct mesh_s mesh, Color *framebuffer,
                 float *depthbuffer, const cam cam);
void clear_framebuffer(Color *framebuffer, Color clearColor);
void clear_depthbuffer(float *depthbuffer);
