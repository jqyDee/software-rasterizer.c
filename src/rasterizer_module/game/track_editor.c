#include "track_editor.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "raylib.h"
#include "track.h"
#include "../world.h"

#define POINT_HIT_RADIUS_PX 12.0f
#define GRID_STEP 10.0f
#define WIDTH_SCROLL_STEP 0.5f

/* view state — static is fine: resets on plugin reload, track data lives
 * in the world and survives */
static float cam_x = 0.0f;
static float cam_z = 0.0f;
static float zoom = 7.0f; /* pixels per world unit */
static int dragging = -1;

static Vector2 world_to_screen(float wx, float wz) {
  return (Vector2){(wx - cam_x) * zoom + GetScreenWidth() * 0.5f,
                   (wz - cam_z) * zoom + GetScreenHeight() * 0.5f};
}

static vec2f screen_to_world(Vector2 s) {
  return (vec2f){(s.x - GetScreenWidth() * 0.5f) / zoom + cam_x,
                 (s.y - GetScreenHeight() * 0.5f) / zoom + cam_z};
}

static int hovered_point(const track *t, Vector2 mouse) {
  int best = -1;
  float best_d = POINT_HIT_RADIUS_PX;
  for (size_t i = 0; i < t->point_count; i++) {
    Vector2 s = world_to_screen(t->points[i].pos.x, t->points[i].pos.y);
    float dx = s.x - mouse.x, dy = s.y - mouse.y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < best_d) {
      best_d = d;
      best = (int)i;
    }
  }
  return best;
}

/* distance from p to segment a-b in world space */
static float point_segment_dist(vec2f p, vec2f a, vec2f b) {
  float abx = b.x - a.x, aby = b.y - a.y;
  float len_sq = abx * abx + aby * aby;
  float u = 0.0f;
  if (len_sq > 0.00001f) {
    u = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len_sq;
    if (u < 0.0f)
      u = 0.0f;
    if (u > 1.0f)
      u = 1.0f;
  }
  float cx = a.x + abx * u, cy = a.y + aby * u;
  float dx = p.x - cx, dy = p.y - cy;
  return sqrtf(dx * dx + dy * dy);
}

static void insert_point(track *t, vec2f wpos) {
  if (t->point_count >= MAX_TRACK_POINTS)
    return;
  /* insert after the segment closest to the click */
  size_t after = 0;
  float best = 1e30f;
  for (size_t i = 0; i < t->point_count; i++) {
    size_t j = (i + 1) % t->point_count;
    float d = point_segment_dist(wpos, t->points[i].pos, t->points[j].pos);
    if (d < best) {
      best = d;
      after = i;
    }
  }
  /* neighbor widths average for the new point */
  float w = (t->points[after].width +
             t->points[(after + 1) % t->point_count].width) *
            0.5f;
  memmove(&t->points[after + 2], &t->points[after + 1],
          (t->point_count - after - 1) * sizeof(track_point));
  t->points[after + 1].pos = wpos;
  t->points[after + 1].width = w;
  t->point_count++;
}

static void remove_point(track *t, int idx) {
  if (t->point_count <= 3) /* keep a valid loop */
    return;
  memmove(&t->points[idx], &t->points[idx + 1],
          (t->point_count - (size_t)idx - 1) * sizeof(track_point));
  t->point_count--;
}

static void draw_grid(void) {
  int w = GetScreenWidth(), h = GetScreenHeight();
  vec2f tl = screen_to_world((Vector2){0, 0});
  vec2f br = screen_to_world((Vector2){(float)w, (float)h});
  Color grid = (Color){45, 50, 56, 255};
  Color axis = (Color){70, 78, 86, 255};
  for (float x = floorf(tl.x / GRID_STEP) * GRID_STEP; x <= br.x;
       x += GRID_STEP) {
    Vector2 a = world_to_screen(x, tl.y), b = world_to_screen(x, br.y);
    DrawLineEx(a, b, 1.0f, x == 0.0f ? axis : grid);
  }
  for (float z = floorf(tl.y / GRID_STEP) * GRID_STEP; z <= br.y;
       z += GRID_STEP) {
    Vector2 a = world_to_screen(tl.x, z), b = world_to_screen(br.x, z);
    DrawLineEx(a, b, 1.0f, z == 0.0f ? axis : grid);
  }
}

static void draw_track(const track *t, int hovered) {
  /* road edges + centerline, sampled along the spline */
  Color edge_col = (Color){120, 128, 140, 255};
  Color center_col = (Color){200, 205, 215, 255};
  for (size_t seg = 0; seg < t->point_count; seg++) {
    for (int s = 0; s < TRACK_SAMPLES_PER_SEG; s++) {
      float u0 = (float)s / TRACK_SAMPLES_PER_SEG;
      float u1 = (float)(s + 1) / TRACK_SAMPLES_PER_SEG;
      size_t seg1 = seg;
      if (u1 >= 1.0f) {
        u1 = 0.0f;
        seg1 = (seg + 1) % t->point_count;
      }
      vec2f c0 = track_sample_pos(t, seg, u0);
      vec2f c1 = track_sample_pos(t, seg1, u1);
      vec2f l0, r0, l1, r1;
      track_sample_edges(t, seg, u0, &l0, &r0);
      track_sample_edges(t, seg1, u1, &l1, &r1);
      DrawLineEx(world_to_screen(c0.x, c0.y), world_to_screen(c1.x, c1.y),
                 1.0f, center_col);
      DrawLineEx(world_to_screen(l0.x, l0.y), world_to_screen(l1.x, l1.y),
                 2.0f, edge_col);
      DrawLineEx(world_to_screen(r0.x, r0.y), world_to_screen(r1.x, r1.y),
                 2.0f, edge_col);
    }
  }

  /* control points; point 0 = start/finish (green) with direction arrow */
  for (size_t i = 0; i < t->point_count; i++) {
    Vector2 s = world_to_screen(t->points[i].pos.x, t->points[i].pos.y);
    Color col = i == 0 ? GREEN : (Color){235, 165, 50, 255};
    if ((int)i == hovered)
      col = YELLOW;
    DrawCircleV(s, i == 0 ? 8.0f : 6.0f, col);
  }
  if (t->point_count >= 2) {
    vec2f p0 = track_sample_pos(t, 0, 0.0f);
    vec2f p1 = track_sample_pos(t, 0, 0.3f);
    DrawLineEx(world_to_screen(p0.x, p0.y), world_to_screen(p1.x, p1.y), 3.0f,
               GREEN);
  }
}

void track_editor_update_and_draw(world *world) {
  track *t = &world->track_data;
  Vector2 mouse = GetMousePosition();
  vec2f mouse_world = screen_to_world(mouse);
  int hovered = hovered_point(t, mouse);

  /* ---- input ---- */
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hovered >= 0)
    dragging = hovered;
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    dragging = -1;
  if (dragging >= 0 && dragging < (int)t->point_count)
    t->points[dragging].pos = mouse_world;

  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    insert_point(t, mouse_world);

  if (hovered >= 0 && dragging < 0 &&
      (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_DELETE) ||
       IsKeyPressed(KEY_BACKSPACE)))
    remove_point(t, hovered);

  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    if (hovered >= 0 && dragging < 0) {
      t->points[hovered].width += wheel * WIDTH_SCROLL_STEP;
      if (t->points[hovered].width < 1.5f)
        t->points[hovered].width = 1.5f;
      if (t->points[hovered].width > 30.0f)
        t->points[hovered].width = 30.0f;
    } else {
      zoom *= 1.0f + wheel * 0.1f;
      if (zoom < 1.0f)
        zoom = 1.0f;
      if (zoom > 60.0f)
        zoom = 60.0f;
    }
  }

  if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
    Vector2 d = GetMouseDelta();
    cam_x -= d.x / zoom;
    cam_z -= d.y / zoom;
  }

  if ((IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) ||
       IsKeyDown(KEY_LEFT_CONTROL)) &&
      IsKeyPressed(KEY_S)) {
    track_save(t, TRACK_FILE);
    track_build_mesh(world);
  }

  /* ---- draw ---- */
  ClearBackground((Color){28, 31, 36, 255});
  draw_grid();
  draw_track(t, hovered);

  DrawText("TRACK EDITOR", 10, 10, 24, RAYWHITE);
  DrawText(TextFormat("%zu points  |  mouse %.1f / %.1f", t->point_count,
                      mouse_world.x, mouse_world.y),
           10, 40, 18, LIGHTGRAY);
  DrawText("LMB drag point | RMB add | X del | wheel: width on point, zoom "
           "elsewhere | MMB pan | Cmd+S save | F4 exit",
           10, GetScreenHeight() - 28, 16, GRAY);
  if (hovered >= 0)
    DrawText(TextFormat("width %.1f", t->points[hovered].width),
             (int)mouse.x + 14, (int)mouse.y - 8, 16, YELLOW);
}
