#include <math.h>
#include <stdio.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "engine.h"
#include "gui.h"
#include "types.h"

void draw_debug_gui(world *world, float delta_time) {
  if (!world->settings.show_debug_gui)
    return;

  int fs = GuiGetStyle(DEFAULT, TEXT_SIZE);
  char row0[64], row1[64];
  snprintf(row0, sizeof(row0), "FPS: %d  frametime: %.2fms", GetFPS(),
           delta_time * 1000.0f);
  snprintf(row1, sizeof(row1), "buf: %dx%d  display: %dx%d  render: %dx%d",
           world->renderer->screen_width, world->renderer->screen_height,
           world->renderer->display_width, world->renderer->display_height,
           GetRenderWidth(), GetRenderHeight());
  int w0 = MeasureText(row0, fs) + 26;
  int w1 = MeasureText(row1, fs) + 26;
  GuiStatusBar((Rectangle){10, 10, w0, 24}, row0);
  GuiStatusBar((Rectangle){10, 36, w1, 24}, row1);

  // Settings panel
  const int px = 10, py = 68, pw = 260, rh = 28, pad = 20, lw = 90;
  GuiPanel((Rectangle){px, py, pw, 4 * rh + pad + 24}, "Settings");

  int y = py + 24 + pad / 2;
  int ctrl_x = px + pad + lw, ctrl_w = pw - lw - pad * 2;

  // Render width (triggers buffer resize on change)
  static bool rw_edit = false;
  int prev_rw = world->settings.render_width;
  GuiLabel((Rectangle){px + pad, y, lw, 20}, "Render width");
  if (GuiSpinner((Rectangle){ctrl_x, y, ctrl_w, 20}, NULL,
                 &world->settings.render_width, 64, 1200, rw_edit))
    rw_edit = !rw_edit;
  if (world->settings.render_width != prev_rw)
    resize_renderer(world);
  y += rh;

  // Near plane
  GuiLabel((Rectangle){px + pad, y, lw, 20}, "Near plane");
  GuiSlider((Rectangle){ctrl_x, y, ctrl_w, 20}, "0.01", "5",
            &world->settings.near_plane, 0.01f, 5.0f);
  y += rh;

  // OMP cutoff
  static bool omp_edit = false;
  GuiLabel((Rectangle){px + pad, y, lw, 20}, "OMP cutoff");
  if (GuiSpinner((Rectangle){ctrl_x, y, ctrl_w, 20}, NULL,
                 &world->settings.parallel_cutoff_rows, 0, 800, omp_edit))
    omp_edit = !omp_edit;
  y += rh;

  // FOV (recalc focal_length on change)
  float prev_fov = world->cam->fov;
  GuiLabel((Rectangle){px + pad, y, lw, 20}, "FOV");
  GuiSlider((Rectangle){ctrl_x, y, ctrl_w, 20}, "10", "170", &world->cam->fov,
            10.0f, 170.0f);
  if (world->cam->fov != prev_fov)
    world->cam->focal_length =
        1.0f / tanf(world->cam->fov * (PI / 180.0f) / 2.0f);
}
