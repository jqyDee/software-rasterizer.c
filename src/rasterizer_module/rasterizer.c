#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "draw.h"
#include "engine.h"
#include "types.h"
#include "update.h"

void *rasterizer(void *saved_state) {
  srand(time(NULL));

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  if (GetMonitorHeight(0) > SCREEN_HEIGHT ||
      GetMonitorWidth(0) > SCREEN_WIDTH) {
    CloseWindow();
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  }

  char *obj_paths[] = {
      // "./obj/cube.obj",
      "./obj/Suzanne.obj",
  };
  size_t obj_count = sizeof(obj_paths) / sizeof(obj_paths[0]);

  world *world = saved_state;
  if (!world) {
    printf("reloaded. no saved state\n");
    world = malloc(sizeof(struct world_s));
    if (!init_world(world, obj_paths, obj_count)) {
      destroy_world(world);
      return 0;
    }
  } else {
    init_texture(world->renderer);
    if (!load_objs_files(world, obj_paths, obj_count)) {
      destroy_world(world);
      return NULL;
    }

    printf("reloaded!\n");
  }

#ifdef DEBUG
  int count = 0;
#endif

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      float delta_time = GetFrameTime();

      // SPECIAL COMMANDS
      switch (handle_user_input(world->cam, delta_time)) {
      case 0:
        break;

      case 1:
        printf("reloading...\n");
        CloseWindow();
        return world;

      case 2:
        printf("resetting camera\n");
        init_cam(world->cam);
        break;

      default:
        break;
      }

      // rotate_mesh_around_origin(&world.meshes[0], 0.02f, 0.01f);

      ClearBackground(WHITE);

      clear_framebuffer(world->renderer, WHITE);
      clear_depthbuffer(world->renderer);

      render_world(world);

      UpdateTexture(world->renderer->screen_texture,
                    world->renderer->framebuffer);
      DrawTexture(world->renderer->screen_texture, 0, 0, WHITE);

#ifdef DEBUG
      // FPS
      char fpsText[64];
      snprintf(fpsText, sizeof(fpsText), "FPS: %dfps, frametime: %dms",
               GetFPS(), (int)(delta_time*1000));
      DrawText(fpsText, 10, 10, 20, RED);

      if (count == -200) {
        printf("x: %f, y: %f, z: %f; pitch: %f; yaw: %f; fov: %f\n",
               world->cam->pos.x, world->cam->pos.y, world->cam->pos.z,
               world->cam->pitch, world->cam->yaw, world->cam->fov);
        count = 0;
      } else {
        count++;
      }
#endif
    }
    EndDrawing();
  }

  destroy_world(world);
  CloseWindow();
  return NULL;
}
