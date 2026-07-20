#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_object.h"
#include "gui_window.h"

#include "../engine/texture.h"
#include "../engine/undo.h"
#include "../math/projection.h"
#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../math/vec.h"
#include "../renderer/geometry.h"
#include "../renderer/transform.h"

void draw_gizmo(world *world) {
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
  viewport vp = viewport_full_frame(world->renderer);
  project(world->cam, &vp, cam_coordinates, &sc);
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
    project(world->cam, &vp, ctip, &st);

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

void draw_collision_boxes(const world *world) {
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

    viewport vp = viewport_full_frame(world->renderer);
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
      project(world->cam, &vp, ca, &sa);
      project(world->cam, &vp, cb, &sb);
      DrawLine((int)(sa.x * scx), (int)(sa.y * scy), (int)(sb.x * scx),
               (int)(sb.y * scy), col);
    }
  }
}

void draw_normals(const world *world) {
  if (!world->settings.show_normals)
    return;

  const float near = world->settings.near_plane;
  float scx = (float)world->renderer->display_width /
              (float)world->renderer->screen_width;
  float scy = (float)world->renderer->display_height /
              (float)world->renderer->screen_height;

  /* debug free-fly mode is a single full-frame pass through world->cam
   * (matches render()'s own fallback); game mode draws the scene's normals
   * once per split-screen viewport, through that viewport's own camera —
   * otherwise viewports 1+ would show normals projected with the wrong
   * camera and viewport 0's own scaling would be wrong too */
  bool single_view = world->settings.show_debug_gui;
  size_t view_count =
      single_view ? 1 : (world->kart_count > 0 ? world->kart_count : 1);

  for (size_t vi = 0; vi < view_count; vi++) {
    const cam *c = single_view ? world->cam : &world->game_cams[vi];
    viewport vp = single_view
                      ? viewport_full_frame(world->renderer)
                      : compute_kart_viewport((int)vi, (int)world->kart_count,
                                              world->renderer->screen_width,
                                              world->renderer->screen_height);

    for (size_t ii = 0; ii < world->instance_count; ii++) {
      const mesh_instance *inst = &world->instances[ii];
      const mesh *m = &world->mesh_data[inst->mesh_idx];
      for (size_t i = 0; i + 2 < m->vertex_count; i += 3) {
        vec3f v_cam[3];
        transform_triangle_to_camera(world->mesh_data, i, inst, c, v_cam);
        vec3f normal = compute_face_normal(v_cam);
        if (is_backfacing(v_cam, normal))
          continue;

        vec3f centroid = {
            (v_cam[0].x + v_cam[1].x + v_cam[2].x) / 3.0f,
            (v_cam[0].y + v_cam[1].y + v_cam[2].y) / 3.0f,
            (v_cam[0].z + v_cam[1].z + v_cam[2].z) / 3.0f,
        };
        if (centroid.z <= near)
          continue;

        vec3f tip = vec_add(centroid, vec_scale(normal, 0.25f));
        if (tip.z <= near)
          continue;

        vec3f sc, st;
        project(c, &vp, centroid, &sc);
        project(c, &vp, tip, &st);
        DrawLine((int)(sc.x * scx), (int)(sc.y * scy), (int)(st.x * scx),
                 (int)(st.y * scy), BLUE);
      }
    }
  }
}

void draw_object_window(world *world, gui_state_t *gs) {
  const float OBJ_W = 300.0f;
  const float OBJ_CONTENT = PAD + 16.0f * ROW_H + 128.0f + GAP + PAD;

  if (world->selected_instance >= 0 &&
      world->selected_instance < (int)world->instance_count) {
    gs->obj_win.open = true;
    mesh_instance *inst = &world->instances[world->selected_instance];

    const char *mname = world->mesh_data[inst->mesh_idx].name;
    char obj_title[64];
    snprintf(obj_title, sizeof(obj_title), "Object: %s", mname);

    if (do_window(&gs->obj_win, obj_title, OBJ_W, OBJ_CONTENT)) {
      float x = gs->obj_win.pos.x + PAD;
      float y = gs->obj_win.pos.y + TITLE_H + PAD;
      float aw = OBJ_W - PAD * 2.0f;

      /* snapshot once per mouse-press-in-inspector, per selected instance */
      static int snap_guard = -2;
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        snap_guard = -2;
      if (snap_guard != world->selected_instance &&
          IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
          CheckCollisionPointRec(GetMousePosition(),
                                 (Rectangle){gs->obj_win.pos.x, gs->obj_win.pos.y,
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

      /* draw mesh dropdown last so open list overlays texture section */
      if (GuiDropdownBox(mesh_dd_rect, mesh_items, &mesh_dd_active,
                         mesh_dd_open))
        mesh_dd_open = !mesh_dd_open;
      if (mesh_dd_active != inst->mesh_idx)
        inst->mesh_idx = mesh_dd_active;
      if (mesh_dd_open)
        world->settings.mouse_over_gui = true;
    }
    if (!gs->obj_win.open)
      world->selected_instance = -1;
  } else {
    gs->obj_win.open = false;
  }
}
