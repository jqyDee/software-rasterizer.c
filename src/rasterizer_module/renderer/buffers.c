#include "buffers.h"

#include <stdlib.h>
#include <string.h>

void clear_framebuffer(framebuffer *framebuffer, const int screen_width,
                       const int screen_height, const Color clearColor) {
  /* WHITE = {255,255,255,255} = 0xFFFFFFFF - memset covers this */
  if (clearColor.r == 255 && clearColor.g == 255 && clearColor.b == 255 &&
      clearColor.a == 255) {
    memset(framebuffer, 0xFF,
           (size_t)screen_width * screen_height * sizeof(Color));
  } else {
    int n = screen_width * screen_height;
    for (int i = 0; i < n; i++)
      framebuffer[i] = clearColor;
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
