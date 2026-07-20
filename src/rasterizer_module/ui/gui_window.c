#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

#include "gui_window.h"

bool do_window(WinState *w, const char *title, float width,
               float content_h) {
  if (!w->open)
    return false;

  float win_h = w->minimized ? TITLE_H : (TITLE_H + content_h);

  float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
  if (w->pos.x < 0)
    w->pos.x = 0;
  if (w->pos.y < 0)
    w->pos.y = 0;
  if (w->pos.x + width > sw)
    w->pos.x = sw - width;
  if (w->pos.y + win_h > sh)
    w->pos.y = sh - win_h;

  Rectangle drag_area = {w->pos.x, w->pos.y, width - TITLE_H * 2.0f, TITLE_H};
  Vector2 mouse = GetMousePosition();

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      CheckCollisionPointRec(mouse, drag_area)) {
    w->drag_started = true;
    w->drag_active = false;
    w->drag_origin = mouse;
    w->drag_offset = (Vector2){mouse.x - w->pos.x, mouse.y - w->pos.y};
  }
  if (w->drag_started && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    float dx = mouse.x - w->drag_origin.x;
    float dy = mouse.y - w->drag_origin.y;
    if (dx * dx + dy * dy > 16.0f)
      w->drag_active = true;
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    if (w->drag_started && !w->drag_active)
      w->minimized = !w->minimized;
    w->drag_started = false;
    w->drag_active = false;
  }
  if (w->drag_active) {
    w->pos.x = mouse.x - w->drag_offset.x;
    w->pos.y = mouse.y - w->drag_offset.y;
  }

  if (w->minimized) {
    GuiStatusBar((Rectangle){w->pos.x, w->pos.y, width - TITLE_H * 2.0f, TITLE_H},
                 title);
    if (GuiButton(
            (Rectangle){w->pos.x + width - TITLE_H * 2.0f, w->pos.y, TITLE_H, TITLE_H},
            w->pinned ? "P" : "-"))
      w->pinned = !w->pinned;
    if (GuiButton(
            (Rectangle){w->pos.x + width - TITLE_H, w->pos.y, TITLE_H, TITLE_H},
            "x"))
      w->open = false;
    return false;
  }

  if (GuiWindowBox((Rectangle){w->pos.x, w->pos.y, width, TITLE_H + content_h},
                   title)) {
    w->open = false;
    return false;
  }
  
  if (GuiButton((Rectangle){w->pos.x + width - TITLE_H * 2.0f, w->pos.y, TITLE_H, TITLE_H},
                w->pinned ? "P" : "-")) {
    w->pinned = !w->pinned;
  }
  
  return true;
}

bool point_over_win(WinState *w, float width, float content_h,
                     Vector2 p) {
  if (!w->open)
    return false;
  float h = w->minimized ? TITLE_H : (TITLE_H + content_h);
  return CheckCollisionPointRec(p, (Rectangle){w->pos.x, w->pos.y, width, h});
}
