#include <math.h>
#include <stdio.h>

#include "gui_kart.h"
#include "gui_window.h"

#include "../game/track.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../math/vec.h"

/* camera-space transform + project a world point to screen pixels; returns
 * false (and leaves *out untouched) when the point is behind the near
 * plane, since projecting it would be meaningless */
static bool project_to_screen(world *world, vec3f world_pos, float dscale,
                              Vector2 *out) {
  float near = world->settings.near_plane;
  vec3f cam_space = rotate_x(
      rotate_y(vec3f_sub(world_pos, world->cam->pos), -world->cam->yaw),
      -world->cam->pitch);
  if (cam_space.z <= near)
    return false;
  vec3f screen;
  project(world->cam, world->renderer, cam_space, &screen);
  *out = (Vector2){screen.x * dscale, screen.y * dscale};
  return true;
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
  /* rows: (Drive: 1 header + 5) + (Offroad: 1 + 2) + (Boost: 1 + 4) +
   * (Drift: 1 + 4) + (Air: 1 + 2) + (Live: 1 header + 6 labels) +
   * (1 checkbox row) = 30 */
  const float KART_CONTENT = PAD + 30.0f * ROW_H + PAD;

  if (do_window(&gs->kart_win, "Kart", KART_W, KART_CONTENT)) {
    float x = gs->kart_win.pos.x + PAD;
    float y = gs->kart_win.pos.y + TITLE_H + PAD;
    float aw = KART_W - PAD * 2.0f;

    const float LW = 75.0f;
    const float BW = 22.0f;
    const float BG = 3.0f;
    const float SW = aw - LW - GAP - (3.0f * BW + 2.0f * BG) - GAP;

    kart_tuning *t = &world->kart_tuning;

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Drive");
    y += ROW_H;
    SROW("Accel: %.1f", t->accel_rate, 1.0f, 60.0f, 0.5f, 10.0f, "1", "60");
    SROW("Speed: %.1f", t->max_speed, 5.0f, 60.0f, 0.5f, 25.0f, "5", "60");
    SROW("Rev: %.1f", t->max_reverse_speed, 1.0f, 30.0f, 0.5f, 10.0f, "1",
         "30");
    SROW("Turn: %.2f", t->turn_rate, 0.1f, 5.0f, 0.05f, 1.0f, "0.1", "5");
    SROW("Fric: %.2f", t->friction_coefficient, 0.0f, 3.0f, 0.05f, 0.6f, "0",
         "3");

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Offroad");
    y += ROW_H;
    SROW("Fric: %.2f", t->offroad_friction, 0.0f, 10.0f, 0.1f, 2.5f, "0",
         "10");
    SROW("Speed: %.1f", t->offroad_max_speed, 1.0f, 40.0f, 0.5f, 10.0f, "1",
         "40");

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Boost");
    y += ROW_H;
    SROW("Accel: %.1f", t->boost_accel, 0.0f, 60.0f, 0.5f, 20.0f, "0", "60");
    SROW("Mult: %.2f", t->boost_speed_mult, 1.0f, 3.0f, 0.05f, 1.5f, "1", "3");
    SROW("Dur: %.2f", t->boost_duration_max, 0.1f, 5.0f, 0.05f, 2.0f, "0.1",
         "5");
    SROW("MinChg: %.2f", t->min_boost_charge, 0.0f, 1.0f, 0.05f, 0.2f, "0",
         "1");

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Drift");
    y += ROW_H;
    SROW("MaxAng: %.1f", t->max_drift_angle, 1.0f, 60.0f, 0.5f, 25.0f, "1",
         "60");
    SROW("SlipMin: %.1f", t->drift_slip_min, 1.0f, 60.0f, 0.5f, 10.0f, "1",
         "60");
    SROW("Resp: %.1f", t->drift_angle_response, 5.0f, 200.0f, 5.0f, 70.0f,
         "5", "200");
    SROW("MinSpd: %.1f", t->min_drift_speed, 0.0f, 20.0f, 0.5f, 5.0f, "0",
         "20");

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Air");
    y += ROW_H;
    SROW("Gravity: %.1f", t->gravity, 0.0f, 60.0f, 0.5f, 20.0f, "0", "60");
    SROW("Jump: %.1f", t->jump_velocity, 0.5f, 30.0f, 0.5f, 8.0f, "0.5", "30");

    GuiLine((Rectangle){x, y, aw, ROW_H}, "Live (kart 0)");
    y += ROW_H;
    if (world->kart_count > 0) {
      kart *k0 = &world->karts[0];
      char buf[64];
      float speed =
          sqrtf(k0->vel.x * k0->vel.x + k0->vel.z * k0->vel.z);

      snprintf(buf, sizeof(buf), "state: %s", drift_state_label(k0->drift_state));
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "drift angle: %.2f", k0->drift_angle);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "boost charge: %.2f", k0->drift_boost_accumulated);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "boost timer: %.2f", k0->boost_timer);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "grounded: %s", k0->is_grounded ? "yes" : "no");
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
      snprintf(buf, sizeof(buf), "speed: %.2f", speed);
      GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
      y += ROW_H;
    } else {
      GuiLabel((Rectangle){x, y, aw, ROW_H}, "no karts");
      y += ROW_H * 6.0f;
    }

    GuiLabel((Rectangle){x, y, LW * 2.0f, ROW_H}, "Show Debug Overlay");
    GuiCheckBox((Rectangle){x + aw - 16.0f, y + (ROW_H - 14.0f) * 0.5f, 14, 14},
                NULL, &world->settings.show_kart_debug);
  }
}

void draw_kart_debug_overlay(world *world) {
  float dscale = (float)world->renderer->display_width /
                 (float)world->renderer->screen_width;

  for (size_t i = 0; i < world->kart_count; i++) {
    kart *k = &world->karts[i];

    Vector2 anchor;
    if (!project_to_screen(world, k->pos, dscale, &anchor))
      continue;

    /* velocity vector: fixed screen length, green (slow) -> red (fast) */
    {
      float speed = sqrtf(k->vel.x * k->vel.x + k->vel.z * k->vel.z);
      float ratio = kart_speed_ratio(k, &world->kart_tuning);
      vec3f vel_dir_world =
          speed > 0.001f
              ? (vec3f){k->vel.x / speed, 0.0f, k->vel.z / speed}
              : (vec3f){0, 0, 1};
      vec3f tip_world = vec3f_add(k->pos, vec3f_scale(vel_dir_world, 2.0f));
      Vector2 tip_screen;
      if (project_to_screen(world, tip_world, dscale, &tip_screen)) {
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
      if (project_to_screen(world, nose_tip, dscale, &s))
        DrawLineEx(anchor, s, 1.5f, SKYBLUE);
      if (project_to_screen(world, travel_tip, dscale, &s))
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
  int next_cp = world->kart_count > 0 ? world->karts[0].next_checkpoint : -1;
  const float GATE_Y = 0.3f; /* lifted above the road surface */

  for (int i = 0; i < RACE_CHECKPOINTS; i++) {
    size_t seg;
    float u;
    checkpoint_seg_u(t, i, &seg, &u);

    vec2f left, right;
    track_sample_edges(t, seg, u, &left, &right);
    vec3f wl = {left.x, GATE_Y, left.y};
    vec3f wr = {right.x, GATE_Y, right.y};

    Vector2 sl, sr;
    if (!project_to_screen(world, wl, dscale, &sl) ||
        !project_to_screen(world, wr, dscale, &sr))
      continue;

    Color col = (i == next_cp) ? GREEN : YELLOW;
    DrawLineEx(sl, sr, 2.0f, col);
  }
}
