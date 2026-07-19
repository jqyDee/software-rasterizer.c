#include "gui_renderer.h"
#include "gui_window.h"

#include "../renderer/renderer.h"

void draw_renderer_window(world *world, gui_state_t *gs) {
  const float REND_W = 260.0f;
  const float REND_CONTENT = PAD + 6.0f * ROW_H + PAD;

  if (do_window(&gs->rend_win, "Renderer", REND_W, REND_CONTENT)) {
    float x = gs->rend_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = REND_W - PAD * 2.0f - LABEL_W - GAP;
    float y = gs->rend_win.pos.y + TITLE_H + PAD;

    static bool rw_edit = false;
    int prev_rw = world->settings.render_width;
    GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "Render width");
    if (GuiSpinner((Rectangle){cx, y, cw, ROW_H - 4}, NULL,
                   &world->settings.render_width, 64, GetMonitorWidth(0),
                   rw_edit))
      rw_edit = !rw_edit;
    if (world->settings.render_width != prev_rw)
      resize_renderer(world);
    y += ROW_H;

    {
      bool gpu_lighting =
          (world->settings.lighting_mode == LIGHTING_GPU_PHONG);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "Lighting");
      GuiToggle((Rectangle){cx, y, cw, ROW_H - 4},
               gpu_lighting ? "GPU (Phong)" : "CPU (Lambertian)",
               &gpu_lighting);
      world->settings.lighting_mode =
          gpu_lighting ? LIGHTING_GPU_PHONG : LIGHTING_CPU_LAMBERTIAN;
    }
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Near: %.3f", world->settings.near_plane);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.near_plane, 0.01f, 5.0f);
    y += ROW_H;

    static bool omp_edit = false;
    GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "OMP cutoff");
    if (GuiSpinner((Rectangle){cx, y, cw, ROW_H - 4}, NULL,
                   &world->settings.parallel_cutoff_rows, 0, 800, omp_edit))
      omp_edit = !omp_edit;
    y += ROW_H;

    GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "Show normals");
    GuiCheckBox((Rectangle){cx, y + 4, ROW_H - 8, ROW_H - 8}, NULL,
                &world->settings.show_normals);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Seam: %.2f", world->settings.seam_bias);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.seam_bias, 0.0f, 8.0f);
  }
}
