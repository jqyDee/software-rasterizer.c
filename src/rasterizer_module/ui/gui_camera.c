#include <math.h>

#include "gui_camera.h"
#include "gui_window.h"

void draw_camera_window(world *world, gui_state_t *gs) {
  const float CAM_W = 220.0f;
  const float CAM_CONTENT = PAD + 6.0f * ROW_H + PAD;

  if (do_window(&gs->cam_win, "Camera", CAM_W, CAM_CONTENT)) {
    float x = gs->cam_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = CAM_W - PAD * 2.0f - LABEL_W - GAP;
    float y = gs->cam_win.pos.y + TITLE_H + PAD;

    float prev_fov = world->cam->fov;
    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "FOV: %.1f", world->cam->fov);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL, &world->cam->fov,
              10.0f, 170.0f);
    if (world->cam->fov != prev_fov)
      world->cam->focal_length =
          1.0f / tanf(world->cam->fov * (M_PI / 180.0f) / 2.0f);
    y += ROW_H;

    {
      char lbl[40];
      snprintf(lbl, sizeof(lbl), "Sens: %.4f",
               world->settings.mouse_sensitivity);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.mouse_sensitivity, 0.0005f, 0.02f);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Speed: %.1f", world->settings.move_speed);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.move_speed, 0.5f, 30.0f);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "DCam Factor: %.1f",
               world->settings.debug_cam_speed_factor);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.debug_cam_speed_factor, 0.0f, 10.0f);
    y += ROW_H;

    GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "Collision");
    GuiCheckBox((Rectangle){cx, y + (ROW_H - 14.0f) * 0.5f, 14, 14}, NULL,
                &world->settings.collision_enabled);
    y += ROW_H;

    GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, "Show Boxes");
    GuiCheckBox((Rectangle){cx, y + (ROW_H - 14.0f) * 0.5f, 14, 14}, NULL,
                &world->settings.show_collision_boxes);
  }
}
