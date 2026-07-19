#include "gui_physics.h"
#include "gui_window.h"

void draw_physics_window(world *world, gui_state_t *gs) {
  const float PHYS_W = 260.0f;
  const float PHYS_CONTENT = PAD + 4.0f * ROW_H + PAD;

  if (do_window(&gs->physics_win, "Physics", PHYS_W, PHYS_CONTENT)) {
    float x = gs->physics_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = PHYS_W - PAD * 2.0f - LABEL_W - GAP;
    float y = gs->physics_win.pos.y + TITLE_H + PAD;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Gravity: %.1f", world->settings.gravity);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.gravity, 0.0f, 50.0f);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Jump: %.1f", world->settings.jump_speed);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.jump_speed, 0.5f, 20.0f);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Ground Y: %.1f", world->settings.ground_y);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.ground_y, -20.0f, 20.0f);
    y += ROW_H;

    {
      char lbl[32];
      snprintf(lbl, sizeof(lbl), "Cam R: %.2f", world->settings.camera_radius);
      GuiLabel((Rectangle){x, y, LABEL_W, ROW_H}, lbl);
    }
    GuiSlider((Rectangle){cx, y, cw, ROW_H - 4}, NULL, NULL,
              &world->settings.camera_radius, 0.05f, 2.0f);
  }
}
