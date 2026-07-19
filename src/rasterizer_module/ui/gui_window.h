#pragma once

#include <stdio.h>

#include "raygui.h"
#include "raylib.h"

#include "gui_state.h"

#define TITLE_H 24.0f
#define ROW_H 26.0f
#define PAD 10.0f
#define GAP 5.0f
#define LABEL_W 88.0f

bool do_window(WinState *w, const char *title, float width, float content_h);
bool point_over_win(WinState *w, float width, float content_h, Vector2 p);

/* SROW: label+slider+[-][+][R] for a single float field. Expects local
 * variables x, y, LW, SW, BW, BG in scope (each window function declares
 * its own). Clamps on +/- button press. R resets to default value.
 * NOTE: sl/sr parameters are accepted but unused — preserved from the
 * original macro as-is, not fixed as part of this move. */
#define SROW(fmt, val, mn, mx, step, def, sl, sr)                              \
  do {                                                                         \
    char _l[32];                                                               \
    snprintf(_l, sizeof(_l), fmt, (double)(val));                              \
    GuiLabel((Rectangle){x, y, LW, ROW_H}, _l);                                \
    GuiSlider((Rectangle){x + LW + GAP, y, SW, ROW_H - 4}, NULL, NULL, &(val), \
              mn, mx);                                                         \
    float _bx = x + LW + GAP + SW + GAP;                                       \
    if (GuiButton((Rectangle){_bx, y, BW, ROW_H - 4}, "-")) {                  \
      (val) -= (step);                                                         \
      if ((val) < (mn))                                                        \
        (val) = (mn);                                                          \
    }                                                                          \
    if (GuiButton((Rectangle){_bx + BW + BG, y, BW, ROW_H - 4}, "+")) {        \
      (val) += (step);                                                         \
      if ((val) > (mx))                                                        \
        (val) = (mx);                                                          \
    }                                                                          \
    if (GuiButton((Rectangle){_bx + 2 * (BW + BG), y, BW, ROW_H - 4}, "R"))    \
      (val) = (def);                                                           \
    y += ROW_H;                                                                \
  } while (0)
