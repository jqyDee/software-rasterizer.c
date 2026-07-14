#include "buffers.h"

#include <stdlib.h>
#include <string.h>

#include "renderer.h"

void clear_color_buffer(Color *color_buffer, const int screen_width,
                        const int screen_height, const Color clearColor) {
  /* WHITE = {255,255,255,255} = 0xFFFFFFFF - memset covers this */
  if (clearColor.r == 255 && clearColor.g == 255 && clearColor.b == 255 &&
      clearColor.a == 255) {
    memset(color_buffer, 0xFF,
           (size_t)screen_width * screen_height * sizeof(Color));
  } else {
    int n = screen_width * screen_height;
    for (int i = 0; i < n; i++)
      color_buffer[i] = clearColor;
  }
}

void clear_depthbuffer(depthbuffer *depthbuffer, const int screen_width,
                       const int screen_height) {
  /* depthbuffer stores inv_z (closer = larger); clear to 0 = infinitely far */
  memset(depthbuffer, 0, (size_t)screen_width * screen_height * sizeof(float));
}

void clear_idbuffer(idbuffer *idbuffer, const int screen_width,
                    const int screen_height) {
  /* -1 = 0xFFFFFFFF as unsigned */
  memset(idbuffer, 0xFF, (size_t)screen_width * screen_height * sizeof(int));
}

void init_buffers(renderer *renderer) {
  int new_w = renderer->screen_width, new_h = renderer->screen_height;

  renderer->framebuffer = malloc(new_w * new_h * sizeof(Color));
  if (!renderer->framebuffer)
    exit(2);
  renderer->depthbuffer = malloc(new_w * new_h * sizeof(float));
  if (!renderer->depthbuffer)
    exit(2);
  renderer->idbuffer = malloc(new_w * new_h * sizeof(int));
  if (!renderer->idbuffer)
    exit(2);
  renderer->albedobuffer = malloc(new_w * new_h * sizeof(Color));
  if (!renderer->albedobuffer)
    exit(2);
  renderer->normalbuffer = malloc(new_w * new_h * sizeof(Color));
  if (!renderer->normalbuffer)
    exit(2);
  renderer->shadow_depthbuffer =
      malloc(SHADOW_MAP_SIZE * SHADOW_MAP_SIZE * sizeof(float));
  if (!renderer->shadow_depthbuffer)
    exit(2);
}

void destroy_buffers(renderer *renderer) {
  free(renderer->framebuffer);
  free(renderer->depthbuffer);
  free(renderer->idbuffer);
  free(renderer->albedobuffer);
  free(renderer->normalbuffer);
  free(renderer->shadow_depthbuffer);
}
