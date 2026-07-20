#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "gui_kart.h"
#include "gui_window.h"

#include "../game/track.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../math/vec.h"
#include "raylib.h"

#define MAX_GAMEPAD_COUNT 4

/* camera-space transform + project a world point to screen pixels through a
 * specific camera/viewport (split-screen: each kart's overlay must project
 * through its OWN camera into its OWN viewport, not always world->cam /
 * the full frame). Returns false (and leaves *out untouched) when the point
 * is behind the near plane, since projecting it would be meaningless. */
static bool project_to_screen_vp(const cam *c, const viewport *vp,
                                 float near, vec3f world_pos, float dscale,
                                 Vector2 *out) {
  vec3f cam_space =
      rotate_x(rotate_y(vec3f_sub(world_pos, c->pos), -c->yaw), -c->pitch);
  if (cam_space.z <= near)
    return false;
  vec3f screen;
  project(c, vp, cam_space, &screen);
  *out = (Vector2){screen.x * dscale, screen.y * dscale};
  return true;
}

/* which camera/viewport should kart i's own debug overlay project through:
 * debug free-fly mode is always a single full-frame pass through
 * world->cam (matches render()'s own fallback); game mode uses that kart's
 * own game_cam and split-screen viewport rect */
static void kart_view_for(const world *world, size_t i, const cam **out_cam,
                          viewport *out_vp) {
  if (world->settings.show_debug_gui) {
    *out_cam = world->cam;
    *out_vp = viewport_full_frame(world->renderer);
  } else {
    *out_cam = &world->game_cams[i];
    *out_vp = compute_kart_viewport((int)i, (int)world->kart_count,
                                    world->renderer->screen_width,
                                    world->renderer->screen_height);
  }
}

static const char *drift_state_label(drift_state s) {
  switch (s) {
  case DRIFT_NONE:
    return "NONE";
  case DRIFT_DRIFTING:
    return "DRIFTING";
  case DRIFT_BOOST:
    return "BOOST";
  }
  return "?";
}

void draw_kart_window(world *world, gui_state_t *gs) {
  const float KART_W = 340.0f;
  /* rows: (Input: 1 header + N players) + (Drive: 1 header + 5) + (Offroad: 1 +
   * 2) + (Boost: 1 + 4) + (Drift: 1 + 4) + (Air: 1 + 2) + (Live: 1 header + 6
   * labels) + (1 checkbox row) = 31 static rows + kart_count */
  const float KART_CONTENT = PAD + (31.0f + world->kart_count) * ROW_H + PAD;

  static bool kart_dd_open[32] = {false}; // supports up to 32 players safely

  if (do_window(&gs->kart_win, "Kart", KART_W, KART_CONTENT)) {
    float x = gs->kart_win.pos.x + PAD;
    float y = gs->kart_win.pos.y + TITLE_H + PAD;
    float aw = KART_W - PAD * 2.0f;

    const float LW = 75.0f;
    const float BW = 22.0f;
    const float BG = 3.0f;
    const float SW = aw - LW - GAP - (3.0f * BW + 2.0f * BG) - GAP;

    kart_tuning *t = &world->kart_tuning;

    // While a dropdown is expanded, its option list visually overlaps rows
    // below it (other dropdowns AND the sliders further down) — raygui has
    // no z-order-aware input routing, so every control just independently
    // tests "is the mouse over my own rect" regardless of what's drawn on
    // top of it this frame. GuiLock() disables input for every control
    // except the one currently in editMode (GuiDropdownBox checks
    // `editMode || !guiLocked`), so locking here makes the open dropdown's
    // list the only thing clickable while it's expanded — everything it
    // covers (checkbox, sliders, other closed dropdowns) is inert instead
    // of also reacting to the same click.
    bool any_dd_open = false;
    for (size_t i = 0; i < world->kart_count; i++)
      if (kart_dd_open[i])
        any_dd_open = true;
    if (any_dd_open)
      GuiLock();

    // Save Y space for the Input Assignment section to draw later
    float start_y_input = y;
    y += ROW_H * (1 + world->kart_count);

    // --- Drive ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Drive");
    y += ROW_H;
    SROW("Accel: %.1f", t->accel_rate, 1.0f, 60.0f, 0.5f, 10.0f, "1", "60");
    SROW("Speed: %.1f", t->max_speed, 5.0f, 60.0f, 0.5f, 25.0f, "5", "60");
    SROW("Rev: %.1f", t->max_reverse_speed, 1.0f, 30.0f, 0.5f, 10.0f, "1",
         "30");
    SROW("Turn: %.2f", t->turn_rate, 0.1f, 5.0f, 0.05f, 1.0f, "0.1", "5");
    SROW("Fric: %.2f", t->friction_coefficient, 0.0f, 3.0f, 0.05f, 0.6f, "0",
         "3");

    // --- Offroad ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Offroad");
    y += ROW_H;
    SROW("Fric: %.2f", t->offroad_friction, 0.0f, 10.0f, 0.1f, 2.5f, "0", "10");
    SROW("Speed: %.1f", t->offroad_max_speed, 1.0f, 40.0f, 0.5f, 10.0f, "1",
         "40");

    // --- Boost ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Boost");
    y += ROW_H;
    SROW("Accel: %.1f", t->boost_accel, 0.0f, 60.0f, 0.5f, 20.0f, "0", "60");
    SROW("Mult: %.2f", t->boost_speed_mult, 1.0f, 3.0f, 0.05f, 1.5f, "1", "3");
    SROW("Dur: %.2f", t->boost_duration_max, 0.1f, 5.0f, 0.05f, 2.0f, "0.1",
         "5");
    SROW("MinChg: %.2f", t->min_boost_charge, 0.0f, 1.0f, 0.05f, 0.2f, "0",
         "1");

    // --- Drift ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Drift");
    y += ROW_H;
    SROW("MaxAng: %.1f", t->max_drift_angle, 1.0f, 60.0f, 0.5f, 25.0f, "1",
         "60");
    SROW("SlipMin: %.1f", t->drift_slip_min, 1.0f, 60.0f, 0.5f, 10.0f, "1",
         "60");
    SROW("Resp: %.1f", t->drift_angle_response, 5.0f, 200.0f, 5.0f, 70.0f, "5",
         "200");
    SROW("MinSpd: %.1f", t->min_drift_speed, 0.0f, 20.0f, 0.5f, 5.0f, "0",
         "20");

    // --- Air ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Air");
    y += ROW_H;
    SROW("Gravity: %.1f", t->gravity, 0.0f, 60.0f, 0.5f, 20.0f, "0", "60");
    SROW("Jump: %.1f", t->jump_velocity, 0.5f, 30.0f, 0.5f, 8.0f, "0.5", "30");

    // --- Live ---
    GuiLine((Rectangle){x, y, aw, ROW_H}, "Live (kart 0)");
    y += ROW_H;
    if (world->kart_count > 0) {
      kart *k0 = &world->karts[0];
      char buf[64];
      float speed = sqrtf(k0->vel.x * k0->vel.x + k0->vel.z * k0->vel.z);

      snprintf(buf, sizeof(buf), "state: %s",
               drift_state_label(k0->drift_state));
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "drift angle: %.2f", k0->drift_angle);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "boost charge: %.2f",
               k0->drift_boost_accumulated);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "boost timer: %.2f", k0->boost_timer);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "grounded: %s",
               k0->is_grounded ? "yes" : "no");
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "speed: %.2f", speed);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
    } else {
      GuiLabel((Rectangle){x, y, aw, ROW_H}, "no karts");
      y += ROW_H * 6.0f;
    }

    // --- Debug Checkbox ---
    GuiLabel((Rectangle){x, y, LW * 2.0f, ROW_H}, "Show Debug Overlay");
    GuiCheckBox((Rectangle){x + aw - 16.0f, y + (ROW_H - 14.0f) * 0.5f, 14, 14},
                NULL, &world->settings.show_kart_debug);

    // =========================================================================
    // --- INPUT ASSIGNMENT SECTION (Drawn Last) ---
    // Drawing this last ensures that the dropdowns visually float ON TOP of
    // the Drive/Offroad sliders beneath them without Z-ordering issues.

    char pad_names[256] = "None;Keyboard";
    for (int i = 0; i < MAX_GAMEPAD_COUNT; i++) {
      char temp[32];
      if (IsGamepadAvailable(i)) {
        snprintf(temp, sizeof(temp), ";Pad %d (Conn)", i);
      } else {
        snprintf(temp, sizeof(temp), ";Pad %d", i);
      }
      strncat(pad_names, temp, sizeof(pad_names) - strlen(pad_names) - 1);
    }

    GuiLine((Rectangle){x, start_y_input, aw, ROW_H}, "Input Assignment");

    // Loop BACKWARDS so Player 0 draws last and its dropdown overlaps Player 1
    for (int i = (int)world->kart_count - 1; i >= 0; i--) {
      kart *k = &world->karts[i];
      float row_y = start_y_input + ROW_H * (i + 1);

      char label[32];
      snprintf(label, sizeof(label), "Player %d:", i);
      GuiLabel((Rectangle){x, row_y, 60, ROW_H}, label);

      Rectangle dd_rect = {x + 65, row_y, 120, ROW_H - 2};

      // Map current id to UI list index: -2 = None, -1 = Keyboard, 0-3 =
      // Gamepads
      int dd_index = k->gamepad_id + 2;
      if (dd_index < 0 || dd_index > 5)
        dd_index = 1; // Default to Keyboard

      // Draw and process the dropdown
      if (GuiDropdownBox(dd_rect, pad_names, &dd_index, kart_dd_open[i])) {
        kart_dd_open[i] = !kart_dd_open[i];

        // Close all other dropdowns automatically when this one opens
        if (kart_dd_open[i]) {
          for (int j = 0; j < 32; j++)
            if (j != i)
              kart_dd_open[j] = false;
        }
      }

      // Re-map back to internal gamepad_id
      k->gamepad_id = dd_index - 2;

      // Visual Connection Status text
      Color status_col = DARKGRAY;
      const char *status_txt = "None";

      if (k->gamepad_id == -1) {
        status_txt = "Keys";
      } else if (k->gamepad_id >= 0) {
        if (IsGamepadAvailable(k->gamepad_id)) {
          status_col = DARKGREEN;
          status_txt = "Ready";
        } else {
          status_col = MAROON;
          status_txt = "Disc";
        }
      }

      DrawText(status_txt, (int)(x + 195), (int)(row_y + 4), 10, status_col);
    }

    if (any_dd_open)
      GuiUnlock();

  } else {
    // If window is minimized or closed, instantly close all dropdowns
    for (int i = 0; i < 32; i++)
      kart_dd_open[i] = false;
  }
}

// player-facing race HUD (speed/drift/boost + lap timing): debug free-fly
// mode always shows kart 0 fullscreen (matches the single full-frame render
// pass); game mode draws each kart's HUD inside its own viewport, so every
// player sees only their own stats
void draw_kart_hud(world *world) {
  if (world->kart_count == 0)
    return;

  bool single_view = world->settings.show_debug_gui;
  size_t hud_count = single_view ? 1 : world->kart_count;

  float scx = (float)world->renderer->display_width /
              (float)world->renderer->screen_width;
  float scy = (float)world->renderer->display_height /
              (float)world->renderer->screen_height;

  for (size_t i = 0; i < hud_count; i++) {
    kart *k = &world->karts[i];

    viewport vp = single_view
                      ? viewport_full_frame(world->renderer)
                      : compute_kart_viewport((int)i, (int)world->kart_count,
                                              world->renderer->screen_width,
                                              world->renderer->screen_height);
    int ox = (int)(vp.x * scx) + 10;
    int oy = (int)(vp.y * scy);

    float spd = sqrtf(k->vel.x * k->vel.x + k->vel.z * k->vel.z);
    const char *drift_txt = k->drift_state == DRIFT_DRIFTING ? "DRIFT"
                            : k->drift_state == DRIFT_BOOST  ? "BOOST"
                                                              : "-";
    bool on_road = track_is_on_road(&world->track_data, k->pos.x, k->pos.z);
    DrawText(TextFormat("kart  spd %5.1f  |  %s  boost %.2f  |  %s%s", spd,
                        drift_txt, k->drift_boost_accumulated,
                        k->is_grounded ? "ground" : "AIR",
                        on_road ? "" : "  |  GRASS"),
             ox, oy + 34, 20, on_road ? GREEN : ORANGE);

    // race HUD: lap, running time, last/best lap
    if (k->wrong_way) {
      DrawText("WRONG WAY!", ox, oy + 58, 24, RED);
    } else if (k->lap == 0) {
      DrawText("cross the line to start", ox, oy + 58, 20, SKYBLUE);
    } else if (k->last_lap_time > 0.0f) {
      DrawText(TextFormat("LAP %d   %6.2fs   last %6.2f   best %6.2f", k->lap,
                          k->lap_time, k->last_lap_time, k->best_lap_time),
               ox, oy + 58, 20, SKYBLUE);
    } else {
      DrawText(TextFormat("LAP %d   %6.2fs", k->lap, k->lap_time), ox,
               oy + 58, 20, SKYBLUE);
    }
  }
}

void draw_kart_debug_overlay(world *world) {
  float dscale = (float)world->renderer->display_width /
                 (float)world->renderer->screen_width;
  float near = world->settings.near_plane;

  for (size_t i = 0; i < world->kart_count; i++) {
    kart *k = &world->karts[i];

    const cam *c;
    viewport vp;
    kart_view_for(world, i, &c, &vp);

    Vector2 anchor;
    if (!project_to_screen_vp(c, &vp, near, k->pos, dscale, &anchor))
      continue;

    /* velocity vector: fixed screen length, green (slow) -> red (fast) */
    {
      float speed = sqrtf(k->vel.x * k->vel.x + k->vel.z * k->vel.z);
      float ratio = kart_speed_ratio(k, &world->kart_tuning);
      vec3f vel_dir_world =
          speed > 0.001f ? (vec3f){k->vel.x / speed, 0.0f, k->vel.z / speed}
                         : (vec3f){0, 0, 1};
      vec3f tip_world = vec3f_add(k->pos, vec3f_scale(vel_dir_world, 2.0f));
      Vector2 tip_screen;
      if (project_to_screen_vp(c, &vp, near, tip_world, dscale, &tip_screen)) {
        Color col = (Color){(unsigned char)(255 * ratio),
                            (unsigned char)(255 * (1.0f - ratio)), 0, 255};
        DrawLineEx(anchor, tip_screen, 2.0f, col);
      }
    }

    /* drift cone: nose (forward_dir) vs travel dir (yaw - drift_angle) */
    {
      vec3f nose_dir = k->forward_dir;
      vec3f travel_dir =
          rotate_y((vec3f){0, 0, 1}, k->yaw - k->drift_angle * DEG_TO_RAD);

      vec3f nose_tip = vec3f_add(k->pos, vec3f_scale(nose_dir, 1.5f));
      vec3f travel_tip = vec3f_add(k->pos, vec3f_scale(travel_dir, 1.5f));

      Vector2 s;
      if (project_to_screen_vp(c, &vp, near, nose_tip, dscale, &s))
        DrawLineEx(anchor, s, 1.5f, SKYBLUE);
      if (project_to_screen_vp(c, &vp, near, travel_tip, dscale, &s))
        DrawLineEx(anchor, s, 1.5f, ORANGE);
    }

    /* boost meter: small screen-space bar above the kart's anchor point */
    {
      const float BAR_W = 40.0f, BAR_H = 5.0f;
      float bx = anchor.x - BAR_W * 0.5f, by = anchor.y - 30.0f;
      DrawRectangleLinesEx((Rectangle){bx, by, BAR_W, BAR_H}, 1, WHITE);
      float fill = k->drift_boost_accumulated;
      if (fill > 1.0f)
        fill = 1.0f;
      if (fill < 0.0f)
        fill = 0.0f;
      Color fill_col = k->drift_state == DRIFT_BOOST ? ORANGE : SKYBLUE;
      DrawRectangle((int)bx, (int)by, (int)(BAR_W * fill), (int)BAR_H,
                    fill_col);
    }
  }
}

/* checkpoint fraction i/RACE_CHECKPOINTS -> (seg, u) spline position, same
 * sample-index math kart_update() uses to test lap-checkpoint crossings */
static void checkpoint_seg_u(const track *t, int i, size_t *seg, float *u) {
  size_t samples = t->point_count * TRACK_SAMPLES_PER_SEG;
  float k = (float)i / (float)RACE_CHECKPOINTS * (float)samples;
  *seg = ((size_t)k) / TRACK_SAMPLES_PER_SEG;
  *u = fmodf(k, (float)TRACK_SAMPLES_PER_SEG) / (float)TRACK_SAMPLES_PER_SEG;
}

void draw_track_checkpoints(world *world) {
  const track *t = &world->track_data;
  if (t->point_count < 3)
    return;

  float dscale = (float)world->renderer->display_width /
                 (float)world->renderer->screen_width;
  float near = world->settings.near_plane;
  const float GATE_Y = 0.3f; /* lifted above the road surface */

  /* checkpoint world positions don't depend on the viewport — compute once */
  vec3f wl_all[RACE_CHECKPOINTS], wr_all[RACE_CHECKPOINTS];
  for (int i = 0; i < RACE_CHECKPOINTS; i++) {
    size_t seg;
    float u;
    checkpoint_seg_u(t, i, &seg, &u);
    vec2f left, right;
    track_sample_edges(t, seg, u, &left, &right);
    wl_all[i] = (vec3f){left.x, GATE_Y, left.y};
    wr_all[i] = (vec3f){right.x, GATE_Y, right.y};
  }

  /* each viewport draws every gate through its own kart's camera, and
   * highlights that kart's own next checkpoint (not always kart 0's) */
  size_t view_count = world->settings.show_debug_gui
                          ? 1
                          : (world->kart_count > 0 ? world->kart_count : 1);
  for (size_t vi = 0; vi < view_count; vi++) {
    const cam *c;
    viewport vp;
    kart_view_for(world, vi, &c, &vp);
    int next_cp =
        world->kart_count > vi ? world->karts[vi].next_checkpoint : -1;

    for (int i = 0; i < RACE_CHECKPOINTS; i++) {
      vec3f wl = wl_all[i], wr = wr_all[i];

      Vector2 sl, sr;
      if (!project_to_screen_vp(c, &vp, near, wl, dscale, &sl) ||
          !project_to_screen_vp(c, &vp, near, wr, dscale, &sr))
        continue;

      Color col = (i == next_cp) ? GREEN : YELLOW;
      DrawLineEx(sl, sr, 2.0f, col);
    }
  }
}
