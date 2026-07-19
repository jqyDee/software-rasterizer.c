#pragma once

#include <stdbool.h>

#include "raylib.h"

typedef struct {
  Vector2 pos;
  bool open;
  bool minimized;
  bool drag_started;
  bool drag_active;
  Vector2 drag_origin;
  Vector2 drag_offset;
} WinState;

typedef struct {
  WinState main_win;
  WinState stats_win;
  WinState rend_win;
  WinState cam_win;
  WinState physics_win;
  WinState kart_win;
  WinState scene_win;
  WinState obj_win;
  int sel_mesh;
  bool dd_open;
} gui_state_t;
