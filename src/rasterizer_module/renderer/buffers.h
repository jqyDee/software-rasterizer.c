#pragma once

#include "raylib.h"

typedef struct renderer_s renderer;

typedef Color framebuffer;
typedef float depthbuffer;
typedef int idbuffer;
typedef Color normalbuffer;
typedef Color albedobuffer;

void clear_color_buffer(Color *color_buffer, const int screen_width,
                       const int screen_height, const Color clearColor);
void clear_depthbuffer(depthbuffer *depthbuffer, const int screen_width,
                       const int screen_height);
void clear_idbuffer(idbuffer *idbuffer, const int screen_width,
                    const int screen_height);

void init_buffers(renderer *renderer);
void destroy_buffers(renderer *renderer);
