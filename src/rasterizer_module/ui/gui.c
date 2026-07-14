#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "gui.h"

#include "../math/transformation.h"
#include "../engine/scene.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/vec.h"
#include "../engine/instance.h"
#include "../engine/texture.h"
#include "../renderer/renderer.h"

#define TITLE_H 24.0f
#define ROW_H 26.0f
#define PAD 10.0f
#define GAP 5.0f
#define LABEL_W 88.0f

typedef struct {
  Vector2 pos;
  bool open;
  bool minimized;
  bool drag_started;
  bool drag_active;
  Vector2 drag_origin;
  Vector2 drag_offset;
} WinState;

static bool do_window(WinState *w, const char *title, float width,
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

  Rectangle drag_area = {w->pos.x, w->pos.y, width - TITLE_H, TITLE_H};
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
    GuiStatusBar((Rectangle){w->pos.x, w->pos.y, width - TITLE_H, TITLE_H},
                 title);
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
  return true;
}

static bool point_over_win(WinState *w, float width, float content_h,
                           Vector2 p) {
  if (!w->open)
    return false;
  float h = w->minimized ? TITLE_H : (TITLE_H + content_h);
  return CheckCollisionPointRec(p, (Rectangle){w->pos.x, w->pos.y, width, h});
}

static void draw_gizmo(world *world) {
  if (world->selected_instance < 0)
    return;

  static int drag_axis = -1;
  static int last_sel = -1;
  static Vector2 drag_prev;

  if (world->selected_instance != last_sel) {
    drag_axis = -1;
    last_sel = world->selected_instance;
  }

  mesh_instance *inst = &world->instances[world->selected_instance];
  cam *c = world->cam;
  float near = world->settings.near_plane;
  float dscale = (float)world->renderer->display_width /
                 (float)world->renderer->screen_width;

  vec3f cam_coordinates =
      rotate_x(rotate_y(vec3f_sub(inst->pos, c->pos), -c->yaw), -c->pitch);

  if (cam_coordinates.z <= near)
    return;
  vec3f sc;
  project(world->cam, world->renderer, cam_coordinates, &sc);
  float cx = sc.x * dscale;
  float cy = sc.y * dscale;

  const float WORLD_OFF = 0.5f;
  const float ARROW_PX = 70.0f;
  const float HIT_DIST = 10.0f;

  vec3f axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  Color cols[3] = {RED, GREEN, BLUE};
  float *posf[3] = {&inst->pos.x, &inst->pos.y, &inst->pos.z};

  float tip_dx[3] = {0}, tip_dy[3] = {0};
  float tip_ex[3] = {cx}, tip_ey[3] = {cy};
  float wpx[3] = {0};
  bool ok[3] = {false};

  Vector2 mouse = GetMousePosition();
  int hovered = -1;

  for (int a = 0; a < 3; a++) {
    vec3f wtip = vec3f_add(inst->pos, vec3f_scale(axes[a], WORLD_OFF));
    vec3f ctip =
        rotate_x(rotate_y(vec3f_sub(wtip, c->pos), -c->yaw), -c->pitch);
    if (ctip.z <= near)
      continue;
    vec3f st;
    project(world->cam, world->renderer, ctip, &st);

    float dx = st.x * dscale - cx;
    float dy = st.y * dscale - cy;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f)
      continue;

    tip_dx[a] = dx / len;
    tip_dy[a] = dy / len;
    tip_ex[a] = cx + tip_dx[a] * ARROW_PX;
    tip_ey[a] = cy + tip_dy[a] * ARROW_PX;
    wpx[a] = WORLD_OFF / len;
    ok[a] = true;

    float mx = mouse.x - cx, my = mouse.y - cy;
    float t = fmaxf(0.0f, fminf(ARROW_PX, mx * tip_dx[a] + my * tip_dy[a]));
    float px = t * tip_dx[a] - mx, py = t * tip_dy[a] - my;
    if (sqrtf(px * px + py * py) < HIT_DIST)
      hovered = a;
  }

  if (!IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      hovered >= 0) {
    scene_push_undo(world);
    drag_axis = hovered;
    drag_prev = mouse;
  }

  if (drag_axis >= 0) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && ok[drag_axis]) {
      float dmx = mouse.x - drag_prev.x;
      float dmy = mouse.y - drag_prev.y;
      *posf[drag_axis] +=
          (dmx * tip_dx[drag_axis] + dmy * tip_dy[drag_axis]) * wpx[drag_axis];
      drag_prev = mouse;
      world->settings.mouse_over_gui = true;
    } else {
      drag_axis = -1;
    }
  }
  if (hovered >= 0 || drag_axis >= 0)
    world->settings.mouse_over_gui = true;

  for (int a = 0; a < 3; a++) {
    if (!ok[a])
      continue;
    bool active = (drag_axis == a) || (hovered == a && drag_axis < 0);
    DrawLineEx((Vector2){cx, cy}, (Vector2){tip_ex[a], tip_ey[a]},
               active ? 3.0f : 1.5f, cols[a]);
    DrawCircle((int)tip_ex[a], (int)tip_ey[a], active ? 6.0f : 4.0f, cols[a]);
  }
  DrawCircle((int)cx, (int)cy, 4, WHITE);
}

static void draw_collision_boxes(const world *world) {
  static const int edges[12][2] = {
      {0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
      {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
  };
  const float near = world->settings.near_plane;
  float scx = (float)world->renderer->display_width /
              (float)world->renderer->screen_width;
  float scy = (float)world->renderer->display_height /
              (float)world->renderer->screen_height;

  for (int ii = 0; ii < (int)world->instance_count; ii++) {
    const mesh_instance *inst = &world->instances[ii];
    const mesh *m = &world->mesh_data[inst->mesh_idx];
    vec3f mn = m->aabb_min, mx = m->aabb_max;

    vec3f obj[8] = {
        {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mn.y, mx.z},
        {mx.x, mn.y, mx.z}, {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z},
        {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z},
    };

    /* transform directly to camera space */
    vec3f cc[8];
    for (int ci = 0; ci < 8; ci++) {
      vec3f v = {obj[ci].x * inst->scale.x, obj[ci].y * inst->scale.y,
                 obj[ci].z * inst->scale.z};
      v = rotate_z_snapped(v, inst->rotation.z * DEG_TO_RAD); // roll
      v = rotate_x_snapped(v, inst->rotation.x * DEG_TO_RAD); // pitch
      v = rotate_y_snapped(v, inst->rotation.y * DEG_TO_RAD); // yaw
      vec3f rel = vec_sub(vec_add(v, inst->pos), world->cam->pos);
      cc[ci] = rotate_x(rotate_y(rel, -world->cam->yaw), -world->cam->pitch);
    }

    Color col = (ii == world->selected_instance) ? RED : BLUE;
    for (int e = 0; e < 12; e++) {
      vec3f ca = cc[edges[e][0]], cb = cc[edges[e][1]];
      if (ca.z < near && cb.z < near)
        continue; /* fully behind camera */

      /* clip to near plane */
      if (ca.z < near) {
        float t = (near - ca.z) / (cb.z - ca.z);
        ca = (vec3f){ca.x + t * (cb.x - ca.x), ca.y + t * (cb.y - ca.y), near};
      } else if (cb.z < near) {
        float t = (near - cb.z) / (ca.z - cb.z);
        cb = (vec3f){cb.x + t * (ca.x - cb.x), cb.y + t * (ca.y - cb.y), near};
      }

      vec3f sa, sb;
      project(world->cam, world->renderer, ca, &sa);
      project(world->cam, world->renderer, cb, &sb);
      DrawLine((int)(sa.x * scx), (int)(sa.y * scy), (int)(sb.x * scx),
               (int)(sb.y * scy), col);
    }
  }
}

void draw_debug_gui(world *world, float delta_time) {
  if (world->settings.show_fps && !world->settings.show_debug_gui) {
    char buf[32];
    int fps = delta_time > 0.0f ? (int)(1.0f / delta_time) : 0;
    snprintf(buf, sizeof(buf), "%d FPS", fps);
    DrawText(buf, 9, 9, 16, BLACK);
    DrawText(buf, 8, 8, 16, WHITE);
  }

  if (!world->settings.show_debug_gui) {
    world->settings.mouse_over_gui = false;
    world->settings.text_input_active = false;
    world->settings.dragging_gui = false;
    return;
  }
  world->settings.text_input_active = false;

  static WinState main_win = {.pos = {10, 10}, .open = true};
  static WinState stats_win = {.pos = {10, 160}, .open = false};
  static WinState rend_win = {.pos = {240, 10}, .open = false};
  static WinState cam_win = {.pos = {240, 200}, .open = false};
  static WinState physics_win = {.pos = {470, 10}, .open = false};
  static WinState scene_win = {.pos = {10, 160}, .open = false};
  static WinState obj_win = {.pos = {290, 10}, .open = false};

  static int sel_mesh = 0;
  static bool dd_open = false;

  if (sel_mesh >= (int)world->mesh_data_count)
    sel_mesh = 0;

  /* mouse-over-gui (1-frame lookahead) */
  {
    const float MAIN_C =
        PAD + ROW_H + GAP + ROW_H + GAP + ROW_H + GAP + ROW_H + PAD;
    const float STATS_C = PAD + 8.0f * ROW_H + PAD;
    const float REND_C = PAD + 6.0f * ROW_H + PAD;
    const float CAM_C = PAD + 6.0f * ROW_H + PAD;
    const float PHYS_C = PAD + 4.0f * ROW_H + PAD;
    float scene_c = PAD + 2.0f * ROW_H + GAP +
                    (float)world->instance_count * ROW_H + GAP + 4.0f * ROW_H +
                    PAD;
    const float OBJ_C = PAD + 16.0f * ROW_H + 128.0f + GAP + PAD;
    Vector2 m = GetMousePosition();
    bool over = point_over_win(&main_win, 220.0f, MAIN_C, m) ||
                point_over_win(&stats_win, 270.0f, STATS_C, m) ||
                point_over_win(&rend_win, 260.0f, REND_C, m) ||
                point_over_win(&cam_win, 220.0f, CAM_C, m) ||
                point_over_win(&scene_win, 220.0f, scene_c, m) ||
                point_over_win(&obj_win, 300.0f, OBJ_C, m) ||
                point_over_win(&physics_win, 260.0f, PHYS_C, m) || dd_open;
    world->settings.mouse_over_gui = over;

    /* latch drag: if mouse pressed over GUI, stay "over GUI" until released */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && over)
      world->settings.dragging_gui = true;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
      world->settings.dragging_gui = false;
    if (world->settings.dragging_gui)
      world->settings.mouse_over_gui = true;
  }

  if (world->settings.show_collision_boxes)
    draw_collision_boxes(world);

  draw_gizmo(world);

  /* ── Main hub ──────────────────────────────────────────────── */
  const float MAIN_W = 220.0f;
  const float MAIN_CONTENT =
      PAD + ROW_H + GAP + ROW_H + GAP + ROW_H + GAP + ROW_H + PAD;

  if (do_window(&main_win, "Debug", MAIN_W, MAIN_CONTENT)) {
    float x = main_win.pos.x + PAD;
    float y = main_win.pos.y + TITLE_H + PAD;
    float aw = MAIN_W - PAD * 2.0f;

    char fps_buf[48];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %d   %.2f ms",
             delta_time > 0.0f ? (int)(1.0f / delta_time) : 0,
             delta_time * 1000.0f);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, fps_buf);
    y += ROW_H + GAP;

    float bw = (aw - GAP * 3.0f) / 4.0f;
    GuiToggle((Rectangle){x, y, bw, ROW_H}, "Stats", &stats_win.open);
    GuiToggle((Rectangle){x + (bw + GAP), y, bw, ROW_H}, "Renderer",
              &rend_win.open);
    GuiToggle((Rectangle){x + (bw + GAP) * 2.0f, y, bw, ROW_H}, "Camera",
              &cam_win.open);
    GuiToggle((Rectangle){x + (bw + GAP) * 3.0f, y, bw, ROW_H}, "Physics",
              &physics_win.open);
    y += ROW_H + GAP;

    GuiToggle((Rectangle){x, y, aw, ROW_H}, "Scene", &scene_win.open);
    y += ROW_H + GAP;

    float rw = (aw - GAP) * 0.5f;
    if (GuiButton((Rectangle){x, y, rw, ROW_H}, "Reload OBJs")) {
      load_objs_files(world);
      reload_instance_textures(world);
    }
    if (GuiButton((Rectangle){x + rw + GAP, y, rw, ROW_H}, "Reload Textures"))
      load_texture_library(world);
  }
  if (!main_win.open) {
    world->settings.show_debug_gui = false;
    world->cam = &world->game_cam;
    main_win.open = true;
    return;
  }

  /* ── Stats ─────────────────────────────────────────────────── */
  const float STATS_W = 340.0f;
  const float STATS_CONTENT = PAD + 11.0f * ROW_H + PAD;

  if (do_window(&stats_win, "Stats", STATS_W, STATS_CONTENT)) {
    float x = stats_win.pos.x + PAD;
    float y = stats_win.pos.y + TITLE_H + PAD;
    float aw = STATS_W - PAD * 2.0f;
    char buf[96];

    perf_metric_t *frametime = &world->perf.metrics[PERF_FRAMETIME];
    perf_metric_t *clear = &world->perf.metrics[PERF_CLEAR];
    perf_metric_t *render = &world->perf.metrics[PERF_RENDER];
    perf_metric_t *upload = &world->perf.metrics[PERF_UPLOAD];
    perf_metric_t *uniform = &world->perf.metrics[PERF_UNIFORM];
    perf_metric_t *shading = &world->perf.metrics[PERF_SHADING];
    perf_metric_t *pupdate = &world->perf.metrics[PERF_UPDATE];
    perf_metric_t *pgui = &world->perf.metrics[PERF_GUI];
    perf_metric_t *pinput = &world->perf.metrics[PERF_INPUT];

    int fps_1pct_low = frametime->low_1pct_ms > 0.0f
                           ? (int)(1000.0f / frametime->low_1pct_ms)
                           : 0;
    snprintf(buf, sizeof(buf),
             "FPS: %d (1%% low: %d)  frametime: %.2f (1%% low: %.2f) ms",
             delta_time > 0.0f ? (int)(1.0f / delta_time) : 0, fps_1pct_low,
             frametime->avg_ms, frametime->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    snprintf(buf, sizeof(buf), "buf: %dx%d   disp: %dx%d",
             world->renderer->screen_width, world->renderer->screen_height,
             world->renderer->display_width, world->renderer->display_height);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    snprintf(buf, sizeof(buf), "cam: %.2f  %.2f  %.2f", world->cam->pos.x,
             world->cam->pos.y, world->cam->pos.z);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    GuiLine((Rectangle){x, y, aw, 1}, NULL);
    y += 6;

    float total = clear->avg_ms + render->avg_ms + upload->avg_ms +
                  uniform->avg_ms + shading->avg_ms;
    snprintf(buf, sizeof(buf), "clear:   %.2f ms  (1%% low: %.2f)",
             clear->avg_ms, clear->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "render:  %.2f ms  (1%% low: %.2f)",
             render->avg_ms, render->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "upload:  %.2f ms  (1%% low: %.2f)",
             upload->avg_ms, upload->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "uniform: %.2f ms  (1%% low: %.2f)",
             uniform->avg_ms, uniform->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "shading: %.2f (1%% low: %.2f)  [total: %.2f]",
             shading->avg_ms, shading->low_1pct_ms, total);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    GuiLine((Rectangle){x, y, aw, 1}, NULL);
    y += 6;

    snprintf(buf, sizeof(buf), "input:   %.2f ms  (1%% low: %.2f)",
             pinput->avg_ms, pinput->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "update:  %.2f ms  (1%% low: %.2f)",
             pupdate->avg_ms, pupdate->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "gui:     %.2f ms  (1%% low: %.2f)",
             pgui->avg_ms, pgui->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
  }

  /* ── Renderer ──────────────────────────────────────────────── */
  const float REND_W = 260.0f;
  const float REND_CONTENT = PAD + 6.0f * ROW_H + PAD;

  if (do_window(&rend_win, "Renderer", REND_W, REND_CONTENT)) {
    float x = rend_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = REND_W - PAD * 2.0f - LABEL_W - GAP;
    float y = rend_win.pos.y + TITLE_H + PAD;

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

  /* ── Camera ────────────────────────────────────────────────── */
  const float CAM_W = 220.0f;
  const float CAM_CONTENT = PAD + 6.0f * ROW_H + PAD;

  if (do_window(&cam_win, "Camera", CAM_W, CAM_CONTENT)) {
    float x = cam_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = CAM_W - PAD * 2.0f - LABEL_W - GAP;
    float y = cam_win.pos.y + TITLE_H + PAD;

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

  /* ── Physics ───────────────────────────────────────────────── */
  const float PHYS_W = 260.0f;
  const float PHYS_CONTENT = PAD + 4.0f * ROW_H + PAD;

  if (do_window(&physics_win, "Physics", PHYS_W, PHYS_CONTENT)) {
    float x = physics_win.pos.x + PAD;
    float cx = x + LABEL_W + GAP;
    float cw = PHYS_W - PAD * 2.0f - LABEL_W - GAP;
    float y = physics_win.pos.y + TITLE_H + PAD;

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

  /* ── Scene ─────────────────────────────────────────────────── */
  const float SCENE_W = 220.0f;
  /* 2 fixed rows (Add/Remove) + instance list + gap + File section
   * (sep+name+btns+status) */
  const float SCENE_CONTENT = PAD + 2.0f * ROW_H + GAP +
                              (float)world->instance_count * ROW_H + GAP +
                              4.0f * ROW_H + PAD;

  if (do_window(&scene_win, "Scene", SCENE_W, SCENE_CONTENT)) {
    static char scene_name[64] = "scene";
    static bool name_edit = false;
    static char status_msg[80] = "";
    static float status_timer = 0.0f;
    static bool status_ok = true;
    status_timer -= delta_time;

    float x = scene_win.pos.x + PAD;
    float aw = SCENE_W - PAD * 2.0f;

    float y_ctrl = scene_win.pos.y + TITLE_H + PAD;
    float y_remove = y_ctrl + ROW_H;
    float y_list = y_remove + ROW_H + GAP;
    float y_filesep = y_list + (float)world->instance_count * ROW_H + GAP;
    float y_name = y_filesep + ROW_H;
    float y_filebtns = y_name + ROW_H;
    float y_status = y_filebtns + ROW_H;

    /* instance list (back layer — drawn first) */
    for (int i = 0; i < (int)world->instance_count; i++) {
      const char *mname = world->mesh_data[world->instances[i].mesh_idx].name;
      char label[64];
      snprintf(label, sizeof(label), "[%d] %s", i, mname);

      bool selected = (world->selected_instance == i);
      GuiToggle((Rectangle){x, y_list + (float)i * ROW_H, aw, ROW_H - 2}, label,
                &selected);
      if (selected && world->selected_instance != i)
        world->selected_instance = i;
      else if (!selected && world->selected_instance == i)
        world->selected_instance = -1;
    }

    if (world->selected_instance < 0)
      GuiDisable();
    if (GuiButton((Rectangle){x, y_remove, aw, ROW_H - 2}, "Remove Selected"))
      remove_instance(world, world->selected_instance);
    if (world->selected_instance < 0)
      GuiEnable();

    /* File section */
    GuiLine((Rectangle){x, y_filesep, aw, ROW_H}, "File");

    if (GuiTextBox((Rectangle){x, y_name, aw, ROW_H - 2}, scene_name,
                   sizeof(scene_name), name_edit))
      name_edit = !name_edit;
    if (name_edit)
      world->settings.text_input_active = true;

    {
      char path[512];
      float hw = (aw - GAP) * 0.5f;
      snprintf(path, sizeof(path), "assets/scenes/%s.json", scene_name);

      if (GuiButton((Rectangle){x, y_filebtns, hw, ROW_H - 2}, "Save")) {
        status_ok = scene_save(world, path);
        status_timer = 3.0f;
        if (status_ok)
          snprintf(status_msg, sizeof(status_msg), "Saved %s", path);
        else
          snprintf(status_msg, sizeof(status_msg), "Error: could not save");
      }
      if (GuiButton((Rectangle){x + hw + GAP, y_filebtns, hw, ROW_H - 2},
                    "Load")) {
        status_ok = scene_load(world, path);
        if (status_ok)
          reload_instance_textures(world);
        status_timer = 3.0f;
        if (status_ok)
          snprintf(status_msg, sizeof(status_msg), "Loaded %s (%zu inst)", path,
                   world->instance_count);
        else
          snprintf(status_msg, sizeof(status_msg), "Error: could not load");
      }
    }

    if (status_timer > 0.0f) {
      Color col =
          status_ok ? (Color){0, 180, 80, 255} : (Color){220, 50, 50, 255};
      DrawText(status_msg, (int)(x), (int)(y_status + 4), 10, col);
    }

    /* dropdown + add — drawn last so open list overlays everything */
    char mesh_names[512] = "";
    for (int i = 0; i < (int)world->mesh_data_count; i++) {
      if (i > 0)
        strncat(mesh_names, ";", sizeof(mesh_names) - strlen(mesh_names) - 1);
      const char *n = world->mesh_data[i].name;
      strncat(mesh_names, n, sizeof(mesh_names) - strlen(mesh_names) - 1);
    }
    float dd_w = aw * 0.72f;
    float add_w = aw - dd_w - GAP;
    if (GuiDropdownBox((Rectangle){x, y_ctrl, dd_w, ROW_H - 2}, mesh_names,
                       &sel_mesh, dd_open))
      dd_open = !dd_open;
    if (!dd_open) {
      if (GuiButton((Rectangle){x + dd_w + GAP, y_ctrl, add_w, ROW_H - 2},
                    "+ Add"))
        add_instance(world, sel_mesh);
    }
  } else {
    dd_open = false;
  }

  /* ── Object editor ─────────────────────────────────────────── */
  /* 12 rows: 3 section headers + 9 value rows                   */
  const float OBJ_W = 300.0f;
  const float OBJ_CONTENT = PAD + 16.0f * ROW_H + 128.0f + GAP + PAD;

  if (world->selected_instance >= 0 &&
      world->selected_instance < (int)world->instance_count) {
    obj_win.open = true;
    mesh_instance *inst = &world->instances[world->selected_instance];

    const char *mname = world->mesh_data[inst->mesh_idx].name;
    char obj_title[64];
    snprintf(obj_title, sizeof(obj_title), "Object: %s", mname);

    if (do_window(&obj_win, obj_title, OBJ_W, OBJ_CONTENT)) {
      float x = obj_win.pos.x + PAD;
      float y = obj_win.pos.y + TITLE_H + PAD;
      float aw = OBJ_W - PAD * 2.0f;

      /* snapshot once per mouse-press-in-inspector, per selected instance */
      static int snap_guard = -2;
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        snap_guard = -2;
      if (snap_guard != world->selected_instance &&
          IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
          CheckCollisionPointRec(GetMousePosition(),
                                 (Rectangle){obj_win.pos.x, obj_win.pos.y,
                                             OBJ_W, TITLE_H + OBJ_CONTENT})) {
        scene_push_undo(world);
        snap_guard = world->selected_instance;
      }

      /* per-row constants */
      const float LW = 75.0f; /* label width */
      const float BW = 22.0f; /* button width */
      const float BG = 3.0f;  /* gap between buttons */
      /* slider fills remaining space: aw - label - gap - 3 buttons - 2 gaps -
       * gap */
      const float SW = aw - LW - GAP - (3.0f * BW + 2.0f * BG) - GAP;

/* SROW: label+slider+[-][+][R] for a single float field.
   Clamps on +/- button press. R resets to default value. */
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

      /* ── Position ── */
      GuiLine((Rectangle){x, y, aw, ROW_H}, "Position");
      y += ROW_H;
      SROW("X: %.2f", inst->pos.x, -50.0f, 50.0f, 0.01f, 0.0f, "-50", "50");
      SROW("Y: %.2f", inst->pos.y, -50.0f, 50.0f, 0.01f, 0.0f, "-50", "50");
      SROW("Z: %.2f", inst->pos.z, -50.0f, 50.0f, 0.01f, 0.0f, "-50", "50");

      /* ── Scale (with uniform-link toggle) ── */
      {
        static bool link_scale = false;
        float link_area = 62.0f;
        GuiLine((Rectangle){x, y, aw - link_area, ROW_H}, "Scale");
        GuiLabel((Rectangle){x + aw - link_area + 4, y, 30.0f, ROW_H}, "Link");
        GuiCheckBox(
            (Rectangle){x + aw - 16.0f, y + (ROW_H - 14.0f) * 0.5f, 14, 14},
            NULL, &link_scale);
        y += ROW_H;

        float sx0 = inst->scale.x, sy0 = inst->scale.y, sz0 = inst->scale.z;
        SROW("Sx: %.2f", inst->scale.x, 0.01f, 20.0f, 0.01f, 1.0f, "0.01",
             "20");
        SROW("Sy: %.2f", inst->scale.y, 0.01f, 20.0f, 0.01f, 1.0f, "0.01",
             "20");
        SROW("Sz: %.2f", inst->scale.z, 0.01f, 20.0f, 0.01f, 1.0f, "0.01",
             "20");

        if (link_scale) {
          if (inst->scale.x != sx0)
            inst->scale.y = inst->scale.z = inst->scale.x;
          else if (inst->scale.y != sy0)
            inst->scale.x = inst->scale.z = inst->scale.y;
          else if (inst->scale.z != sz0)
            inst->scale.x = inst->scale.y = inst->scale.z;
        }
      }

      /* ── Rotation ── */
      GuiLine((Rectangle){x, y, aw, ROW_H}, "Rotation");
      y += ROW_H;
      SROW("Pitch: %.1f", inst->rotation.x, -180.0f, 180.0f, 0.1f, 0.0f, "-180",
           "180");
      SROW("Yaw:   %.1f", inst->rotation.y, -180.0f, 180.0f, 0.1f, 0.0f, "-180",
           "180");
      SROW("Roll:  %.1f", inst->rotation.z, -180.0f, 180.0f, 0.1f, 0.0f, "-180",
           "180");

      /* ── Mesh ── (header + placeholder; dropdown drawn last to overlay) */
      static int mesh_dd_active = 0;
      static bool mesh_dd_open = false;
      static int last_mesh_sel = -1;
      static char mesh_items[1024] = "";
      static int mesh_snap = -1;

      if (mesh_snap != (int)world->mesh_data_count) {
        mesh_snap = (int)world->mesh_data_count;
        char *p = mesh_items, *end = mesh_items + sizeof(mesh_items) - 1;
        *p = '\0';
        for (int mi = 0; mi < (int)world->mesh_data_count; mi++) {
          if (mi > 0 && p < end)
            *p++ = ';';
          int n =
              snprintf(p, (size_t)(end - p), "%s", world->mesh_data[mi].name);
          if (n > 0)
            p += n;
        }
      }
      if (world->selected_instance != last_mesh_sel) {
        last_mesh_sel = world->selected_instance;
        mesh_dd_active = inst->mesh_idx;
        mesh_dd_open = false;
      }

      GuiLine((Rectangle){x, y, aw, ROW_H}, "Mesh");
      y += ROW_H;
      Rectangle mesh_dd_rect = {x, y, aw, ROW_H - 4};
      y += ROW_H;

      /* ── Texture ── */
      {
        static int last_sel = -1;
        static Color *last_pixels = NULL;
        static int tex_active = 0;
        static bool tex_dd_open = false;
        static Texture2D preview_tex = {0};

        /* build dropdown items: "none;name1;name2;..." */
        static char tex_items[4096] = "none";
        static int tex_lib_snap = -1;
        if (tex_lib_snap != world->tex_lib_count) {
          tex_lib_snap = world->tex_lib_count;
          char *p = tex_items, *end = tex_items + sizeof(tex_items) - 1;
          p += snprintf(p, (size_t)(end - p), "none");
          for (int ti = 0; ti < world->tex_lib_count && p < end; ti++)
            p += snprintf(p, (size_t)(end - p), ";%s", world->tex_lib[ti].name);
        }

        /* sync on selection or texture change */
        if (world->selected_instance != last_sel ||
            inst->tex_pixels != last_pixels) {
          last_sel = world->selected_instance;
          last_pixels = inst->tex_pixels;
          tex_active = 0;
          tex_dd_open = false;
          for (int ti = 0; ti < world->tex_lib_count; ti++) {
            if (strcmp(world->tex_lib[ti].path, inst->tex_path) == 0) {
              tex_active = ti + 1;
              break;
            }
          }
          /* rebuild preview GPU texture */
          if (preview_tex.id > 0)
            UnloadTexture(preview_tex);
          preview_tex = (Texture2D){0};
          if (inst->tex_pixels && inst->tex_w > 0 && inst->tex_h > 0) {
            Image img = {.data = inst->tex_pixels,
                         .width = inst->tex_w,
                         .height = inst->tex_h,
                         .mipmaps = 1,
                         .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
            preview_tex = LoadTextureFromImage(img);
          }
        }

        GuiLine((Rectangle){x, y, aw, ROW_H}, "Texture");
        y += ROW_H;

        /* preview box */
        const float PH = 128.0f;
        DrawRectangleLinesEx((Rectangle){x, y, aw, PH}, 1,
                             GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
        if (preview_tex.id > 0) {
          float aspect = (float)preview_tex.width / (float)preview_tex.height;
          float pw = aw, ph = PH;
          if (pw / ph > aspect)
            pw = ph * aspect;
          else
            ph = pw / aspect;
          float ox = x + (aw - pw) * 0.5f;
          float oy = y + (PH - ph) * 0.5f;
          DrawTexturePro(preview_tex,
                         (Rectangle){0, 0, (float)preview_tex.width,
                                     (float)preview_tex.height},
                         (Rectangle){ox, oy, pw, ph}, (Vector2){0, 0}, 0.0f,
                         WHITE);
          char dim[32];
          snprintf(dim, sizeof(dim), "%dx%d", inst->tex_w, inst->tex_h);
          DrawText(dim, (int)(x + 4), (int)(y + PH - 14), 10,
                   GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
        } else {
          DrawText("no texture", (int)(x + 4), (int)(y + PH * 0.5f - 6), 10,
                   GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
        }
        y += PH + GAP;

        int prev_active = tex_active;
        if (GuiDropdownBox((Rectangle){x, y, aw, ROW_H - 4}, tex_items,
                           &tex_active, tex_dd_open))
          tex_dd_open = !tex_dd_open;
        if (tex_active != prev_active) {
          if (tex_active == 0) {
            free(inst->tex_pixels);
            inst->tex_pixels = NULL;
            inst->tex_w = inst->tex_h = 0;
            inst->tex_path[0] = '\0';
          } else {
            apply_lib_texture(world, world->selected_instance, tex_active - 1);
          }
          last_sel = -2; /* force preview rebuild next frame */
        }
        if (tex_dd_open)
          world->settings.mouse_over_gui = true;
        y += ROW_H;
      }

#undef SROW

      /* draw mesh dropdown last so open list overlays texture section */
      if (GuiDropdownBox(mesh_dd_rect, mesh_items, &mesh_dd_active,
                         mesh_dd_open))
        mesh_dd_open = !mesh_dd_open;
      if (mesh_dd_active != inst->mesh_idx)
        inst->mesh_idx = mesh_dd_active;
      if (mesh_dd_open)
        world->settings.mouse_over_gui = true;
    }
    if (!obj_win.open)
      world->selected_instance = -1;
  } else {
    obj_win.open = false;
  }
}
