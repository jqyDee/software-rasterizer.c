#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "draw.h"
#include "engine.h"
#include "gui.h"
#include "types.h"
#include "update.h"

void *rasterizer(void *saved_state) {
  srand(time(NULL));

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);

  int intended_w = SCREEN_WIDTH;
  int intended_h = SCREEN_HEIGHT;
  int skip_resize_events = 0;

  if (saved_state) {
    world *prev = saved_state;
    intended_w = prev->renderer->display_width;
    intended_h = prev->renderer->display_height;
    SetWindowSize(intended_w, intended_h);
    SetWindowPosition(prev->renderer->window_x, prev->renderer->window_y);
    skip_resize_events++;
  }

  char *obj_paths[] = {
      "./obj/cube.obj",
      "./obj/Suzanne.obj",
  };
  size_t obj_count = sizeof(obj_paths) / sizeof(obj_paths[0]);

  world *world = saved_state;
  if (!world) {
    world = malloc(sizeof(struct world_s));
    if (!init_world(world, obj_paths, obj_count, intended_w, intended_h)) {
      destroy_world(world);
      return 0;
    }
  } else {
    world->renderer->screen_texture = (Texture2D){0};
    resize_renderer_to(world, intended_w, intended_h);
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

      if (IsKeyPressed(KEY_F3))
        world->settings.show_debug_gui = !world->settings.show_debug_gui;

      // SPECIAL COMMANDS
      switch (handle_user_input(world, delta_time)) {
      case 0:
        break;

      case 1:
        printf("reloading...\n");
        world->renderer->window_x = GetWindowPosition().x;
        world->renderer->window_y = GetWindowPosition().y;
        CloseWindow();
        return world;

      case 2:
        printf("resetting camera\n");
        init_cam(world->cam);
        break;

      default:
        break;
      }

      rotate_mesh_around_origin(world->instances[0].mesh, 0.2f * delta_time,
                                0.5f * delta_time);
      rotate_mesh_around_origin(world->instances[1].mesh, 0.2f * delta_time,
                                0.5f * delta_time);

      ClearBackground(WHITE);

      clear_framebuffer(world->renderer, WHITE);
      clear_depthbuffer(world->renderer);

      if (IsWindowResized()) {
        if (skip_resize_events > 0)
          skip_resize_events--;
        else
          resize_renderer(world);
      }

      render_world(world);

      UpdateTexture(world->renderer->screen_texture,
                    world->renderer->framebuffer);

      DrawTexturePro(world->renderer->screen_texture,
                     (Rectangle){0, 0, world->renderer->screen_texture.width,
                                 world->renderer->screen_texture.height},
                     (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                     (Vector2){0, 0}, 0.0f, WHITE);

      draw_debug_gui(world, delta_time);
    }
    EndDrawing();
  }

  destroy_world(world);
  CloseWindow();
  return NULL;
}
