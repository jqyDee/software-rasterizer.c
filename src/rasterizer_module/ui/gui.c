#include <stdio.h>

#include "raylib.h"

#include "gui.h"
#include "gui_camera.h"
#include "gui_kart.h"
#include "gui_main.h"
#include "gui_object.h"
#include "gui_physics.h"
#include "gui_renderer.h"
#include "gui_scene.h"
#include "gui_state.h"
#include "gui_stats.h"
#include "gui_window.h"

void draw_debug_gui(world *world, float delta_time) {
  if (world->settings.show_fps && !world->settings.show_debug_gui) {
    char buf[32];
    int fps = delta_time > 0.0f ? (int)(1.0f / delta_time) : 0;
    snprintf(buf, sizeof(buf), "%d FPS", fps);
    DrawText(buf, 9, 9, 16, BLACK);
    DrawText(buf, 8, 8, 16, WHITE);
  }

  /* kart debug overlay + checkpoints + triangle normals: gated only on
   * their own toggles, so they stay visible after the debug GUI hub is
   * closed (matches show_normals' old behavior, which used to run inside
   * the CPU render pipeline every frame regardless of GUI state) */
  if (world->settings.show_kart_debug) {
    draw_kart_debug_overlay(world);
    draw_track_checkpoints(world);
  }
  draw_normals(world);

  if (!world->settings.show_debug_gui) {
    world->settings.mouse_over_gui = false;
    world->settings.text_input_active = false;
    world->settings.dragging_gui = false;
    return;
  }
  world->settings.text_input_active = false;

  static gui_state_t gs = {
      .main_win = {.pos = {10, 10}, .open = true},
      .stats_win = {.pos = {10, 180}, .open = false},
      .rend_win = {.pos = {240, 10}, .open = false},
      .cam_win = {.pos = {240, 220}, .open = false},
      .physics_win = {.pos = {510, 10}, .open = false},
      .kart_win = {.pos = {470, 220}, .open = false},
      .scene_win = {.pos = {10, 180}, .open = false},
      .obj_win = {.pos = {290, 10}, .open = false},
      .sel_mesh = 0,
      .dd_open = false,
  };

  if (gs.sel_mesh >= (int)world->mesh_data_count)
    gs.sel_mesh = 0;

  /* mouse-over-gui (1-frame lookahead) */
  {
    const float MAIN_C =
        PAD + ROW_H + GAP + ROW_H + GAP + ROW_H + GAP + ROW_H + PAD;
    const float STATS_C = PAD + 8.0f * ROW_H + PAD;
    const float REND_C = PAD + 6.0f * ROW_H + PAD;
    const float CAM_C = PAD + 6.0f * ROW_H + PAD;
    const float PHYS_C = PAD + 4.0f * ROW_H + PAD;
    const float KART_C = PAD + 30.0f * ROW_H + PAD;
    float scene_c = PAD + 2.0f * ROW_H + GAP +
                    (float)world->instance_count * ROW_H + GAP + 4.0f * ROW_H +
                    PAD;
    const float OBJ_C = PAD + 16.0f * ROW_H + 128.0f + GAP + PAD;
    Vector2 m = GetMousePosition();
    bool over = point_over_win(&gs.main_win, 220.0f, MAIN_C, m) ||
                point_over_win(&gs.stats_win, 270.0f, STATS_C, m) ||
                point_over_win(&gs.rend_win, 260.0f, REND_C, m) ||
                point_over_win(&gs.cam_win, 220.0f, CAM_C, m) ||
                point_over_win(&gs.scene_win, 220.0f, scene_c, m) ||
                point_over_win(&gs.obj_win, 300.0f, OBJ_C, m) ||
                point_over_win(&gs.physics_win, 260.0f, PHYS_C, m) ||
                point_over_win(&gs.kart_win, 340.0f, KART_C, m) ||
                gs.dd_open;
    world->settings.mouse_over_gui = over;

    /* latch drag: if mouse pressed over GUI, stay "over GUI" until released */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && over)
      world->settings.dragging_gui = true;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
      world->settings.dragging_gui = false;
    if (world->settings.dragging_gui)
      world->settings.mouse_over_gui = true;
  }

  if (world->settings.show_collision_boxes)
    draw_collision_boxes(world);

  draw_gizmo(world);

  if (!draw_main_window(world, &gs, delta_time)) {
    world->settings.show_debug_gui = false;
    world->cam = &world->game_cam;
    gs.main_win.open = true;
    return;
  }

  draw_stats_window(world, &gs, delta_time);
  draw_renderer_window(world, &gs);

  draw_camera_window(world, &gs);
  draw_physics_window(world, &gs);
  draw_kart_window(world, &gs);

  draw_scene_window(world, &gs, delta_time);
  draw_object_window(world, &gs);
}
