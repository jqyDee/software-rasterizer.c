#include "perf.h"

#include "../world.h"

void profile(world *world) {
#define PROFILE(field, code)                                                   \
  do {                                                                         \
    double _t0 = GetTime();                                                    \
    code;                                                                      \
    float _ms = (float)((GetTime() - _t0) * 1000.0);                           \
    world->perf.field = world->perf.field * 0.9f + _ms * 0.1f;                 \
  } while (0)

  PROFILE(clear_ms, {
    clear_framebuffer(world->renderer->framebuffer,
                      world->renderer->screen_width,
                      world->renderer->screen_height, BLACK);
    clear_depthbuffer(world->renderer->depthbuffer,
                      world->renderer->screen_width,
                      world->renderer->screen_height);
    clear_idbuffer(world->renderer->idbuffer, world->renderer->screen_width,
                   world->renderer->screen_height);
  });

  PROFILE(render_ms, render(world));

  PROFILE(upload_ms, UpdateTexture(world->renderer->screen_texture,
                                   world->renderer->framebuffer));

  PROFILE(blit_ms,
          DrawTexturePro(
              world->renderer->screen_texture,
              (Rectangle){0, 0, world->renderer->screen_texture.width,
                          world->renderer->screen_texture.height},
              (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
              (Vector2){0, 0}, 0.0f, WHITE));

#undef PROFILE
}
