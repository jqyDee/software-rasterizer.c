#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "draw.h"
#include "engine.h"
#include "types.h"

int main(void) {
  srand(time(NULL));

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  if (GetMonitorHeight(0) > SCREEN_HEIGHT ||
      GetMonitorWidth(0) > SCREEN_WIDTH) {
    CloseWindow();
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  }

  const char *obj_paths[] = {
      "./obj/Suzanne.obj",
  };

  world world;
  if (!init_world(&world, obj_paths, 1)) {
    return 0;
  }

  int count = 0;
  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      float deltaTime = GetFrameTime();
      handle_user_input(world.cam, deltaTime);
      if (count == 200) {
        printf("x: %f, y: %f, z: %f; pitch: %f; yaw: %f; fov: %f\n",
               world.cam->pos.x, world.cam->pos.y, world.cam->pos.z,
               world.cam->pitch, world.cam->yaw, world.cam->fov);
        count = 0;
      } else {
        count++;
      }
      ClearBackground(WHITE);

      clear_framebuffer(world.renderer, WHITE);
      clear_depthbuffer(world.renderer);

      render_world(&world);

      UpdateTexture(world.renderer->screen_texture,
                    world.renderer->framebuffer);
      DrawTexture(world.renderer->screen_texture, 0, 0, WHITE);

      // FPS
      char fpsText[32];
      snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
      DrawText(fpsText, 10, 10, 20, RED);
    }
    EndDrawing();
  }

  destroy_world(&world);
  CloseWindow();
  return 0;
}
