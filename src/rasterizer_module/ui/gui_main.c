#include <stdio.h>

#include "gui_main.h"
#include "gui_window.h"

#include "../engine/instance.h"
#include "../engine/texture.h"

bool draw_main_window(world *world, gui_state_t *gs, float delta_time) {
  const float MAIN_W = 220.0f;
  const float MAIN_CONTENT =
      PAD + ROW_H + GAP + ROW_H + GAP + ROW_H + GAP + ROW_H + PAD;

  if (do_window(&gs->main_win, "Debug", MAIN_W, MAIN_CONTENT)) {
    float x = gs->main_win.pos.x + PAD;
    float y = gs->main_win.pos.y + TITLE_H + PAD;
    float aw = MAIN_W - PAD * 2.0f;

    char fps_buf[48];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %d   %.2f ms",
             delta_time > 0.0f ? (int)(1.0f / delta_time) : 0,
             delta_time * 1000.0f);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, fps_buf);
    y += ROW_H + GAP;

    float bw = (aw - GAP * 4.0f) / 5.0f;
    GuiToggle((Rectangle){x, y, bw, ROW_H}, "Stats", &gs->stats_win.open);
    GuiToggle((Rectangle){x + (bw + GAP), y, bw, ROW_H}, "Renderer",
              &gs->rend_win.open);
    GuiToggle((Rectangle){x + (bw + GAP) * 2.0f, y, bw, ROW_H}, "Camera",
              &gs->cam_win.open);
    GuiToggle((Rectangle){x + (bw + GAP) * 3.0f, y, bw, ROW_H}, "Physics",
              &gs->physics_win.open);
    GuiToggle((Rectangle){x + (bw + GAP) * 4.0f, y, bw, ROW_H}, "Kart",
              &gs->kart_win.open);
    y += ROW_H + GAP;

    GuiToggle((Rectangle){x, y, aw, ROW_H}, "Scene", &gs->scene_win.open);
    y += ROW_H + GAP;

    float rw = (aw - GAP) * 0.5f;
    if (GuiButton((Rectangle){x, y, rw, ROW_H}, "Reload OBJs")) {
      load_objs_files(world);
      reload_instance_textures(world);
    }
    if (GuiButton((Rectangle){x + rw + GAP, y, rw, ROW_H}, "Reload Textures"))
      load_texture_library(world);
  }
  return gs->main_win.open;
}
