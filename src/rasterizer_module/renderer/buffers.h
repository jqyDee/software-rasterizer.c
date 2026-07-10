#pragma once

#include "raylib.h"

typedef Color framebuffer;
typedef float depthbuffer;
typedef int idbuffer;

void clear_framebuffer(framebuffer *framebuffer, const int screen_width,
                       const int screen_height, const Color clearColor);
void clear_depthbuffer(depthbuffer *depthbuffer, const int screen_width,
                       const int screen_height);
void clear_idbuffer(idbuffer *idbuffer, const int screen_width,
                    const int screen_height);
