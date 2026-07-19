#include <engine/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "engine/instance.h"
#include "engine/texture.h"
#include "game/track.h"
#include "game/track_editor.h"
#include "math/rotation.h"
#include "renderer/renderer.h"
#include "ui/gui.h"
#include "engine/update.h"

#define TITLE "rasterizer.c"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

void *rasterizer(void *saved_state) {
  srand(time(NULL));

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  // SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  // SetTargetFPS(60);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  SetExitKey(KEY_NULL);

  int intended_w = SCREEN_WIDTH;
  int intended_h = SCREEN_HEIGHT;
  int skip_resize_events = 0;

  if (saved_state) {
    world *prev = saved_state;
    intended_w = prev->renderer->display_width;
    intended_h = prev->renderer->display_height;
    SetWindowSize(intended_w, intended_h);
    SetWindowPosition(prev->renderer->window_pos_x,
                      prev->renderer->window_pos_y);
    skip_resize_events++;
  }

  world *world = saved_state;
  if (!world) {
    world = malloc(sizeof(struct world_s));
    if (!init_world(world, intended_w, intended_h)) {
      destroy_world(world);
      return 0;
    }
  } else {
    world->renderer->screen_texture = (Texture2D){0};
    /* GL context died with the old window — stale ids must not be unloaded */
    world->renderer->sky_texture = (Texture2D){0};
    resize_renderer_to(world, intended_w, intended_h);
    load_texture_library(world);
    if (!load_objs_files(world)) {
      destroy_world(world);
      return NULL;
    }
    /* load_objs_files rebuilds mesh_data from scratch, dropping the
     * procedural "__track__" mesh — regenerate it so the track instance's
     * stale mesh_idx doesn't point past the end of the new array */
    track_build_mesh(world);
    reload_instance_textures(world);

    printf("reloaded!\n");
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      float delta_time = GetFrameTime();

      double _input_t0 = GetTime();
      user_input_response _uir = handle_user_input(world, delta_time);
      perf_record(&world->perf.metrics[PERF_INPUT],
                  (float)((GetTime() - _input_t0) * 1000.0));

      switch (_uir) {
      case UIR_RELOAD_PLUGIN:
        printf("INFO: reloading plugin\n");
        world->renderer->window_pos_x = GetWindowPosition().x;
        world->renderer->window_pos_y = GetWindowPosition().y;
        EnableCursor();
        CloseWindow();
        return world;
      case UIR_RESET_CAM_AND_POSITION:
        printf("INFO:resetting camera and position\n");
        init_cam(&world->game_cam);
        world->debug_cam = world->game_cam;
        world->cam = world->settings.show_debug_gui ? &world->debug_cam
                                                    : &world->game_cam;
        world->player_vy = 0.0f;
        break;
      default:
        break;
      }

      if (IsWindowResized()) {
        if (skip_resize_events > 0)
          skip_resize_events--;
        else
          resize_renderer(world);
      }

      if (world->settings.track_editor_active) {
        // 2D top-down track editor replaces the 3D pipeline entirely
        track_editor_update_and_draw(world);
      } else {
        // Kart visuals track physics state in every mode (visible from the
        // debug free-fly cam too)
        kart_sync_instances(world);

        // Camera follow kart (game mode only): trails the kart's smoothed
        // TRAVEL heading (cam_yaw, eased in kart_update) — not the nose —
        // so drift rotation is visible on screen instead of cancelled out.
        if (!world->settings.show_debug_gui && world->kart_count > 0) {
          kart *player_kart = &world->karts[0];
          vec3f heading = rotate_y((vec3f){0, 0, 1}, player_kart->cam_yaw);
          world->game_cam.pos =
              vec_add(player_kart->pos, vec_scale(heading, -6.0f));
          world->game_cam.pos.y = player_kart->pos.y + 1.5f;
          world->game_cam.yaw = player_kart->cam_yaw;
          world->game_cam.pitch = 0.15f; // positive pitch = look down

          // FOV punch while boosting: wide angle = perceived speed.
          // focal_length must follow fov (init_cam computes it only once).
          float fov = 60.0f + 14.0f * player_kart->boost_visual;
          world->game_cam.fov = fov;
          world->game_cam.focal_length =
              1.0f / tanf(fov * ((float)M_PI / 180.0f) * 0.5f);
        }

        double _update_t0 = GetTime();
        update(world);
        perf_record(&world->perf.metrics[PERF_UPDATE],
                    (float)((GetTime() - _update_t0) * 1000.0));

        // this is not well named. This is right now the whole rendering
        // pipeline in here basically xD
        profile(world);

        // kart state overlay (drift/boost/speed feedback while tuning)
        if (world->kart_count > 0) {
          kart *k0 = &world->karts[0];
          float spd = sqrtf(k0->vel.x * k0->vel.x + k0->vel.z * k0->vel.z);
          const char *drift_txt = k0->drift_state == DRIFT_DRIFTING ? "DRIFT"
                                  : k0->drift_state == DRIFT_BOOST  ? "BOOST"
                                                                    : "-";
          bool on_road =
              track_is_on_road(&world->track_data, k0->pos.x, k0->pos.z);
          DrawText(TextFormat("kart  spd %5.1f  |  %s  boost %.2f  |  %s%s",
                              spd, drift_txt, k0->drift_boost_accumulated,
                              k0->is_grounded ? "ground" : "AIR",
                              on_road ? "" : "  |  GRASS"),
                   10, 34, 20, on_road ? GREEN : ORANGE);

          // race HUD: lap, running time, last/best lap
          if (k0->wrong_way) {
            DrawText("WRONG WAY!", 10, 58, 24, RED);
          } else if (k0->lap == 0) {
            DrawText("cross the line to start", 10, 58, 20, SKYBLUE);
          } else if (k0->last_lap_time > 0.0f) {
            DrawText(TextFormat("LAP %d   %6.2fs   last %6.2f   best %6.2f",
                                k0->lap, k0->lap_time, k0->last_lap_time,
                                k0->best_lap_time),
                     10, 58, 20, SKYBLUE);
          } else {
            DrawText(TextFormat("LAP %d   %6.2fs", k0->lap, k0->lap_time),
                     10, 58, 20, SKYBLUE);
          }
        }

        double _gui_t0 = GetTime();
        draw_debug_gui(world, delta_time);
        perf_record(&world->perf.metrics[PERF_GUI],
                    (float)((GetTime() - _gui_t0) * 1000.0));
      }
    }
    EndDrawing();
  }

  EnableCursor();
  destroy_world(world);
  CloseWindow();
  return NULL;
}
