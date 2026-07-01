# Backface Culling & Near-Plane Clipping Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix triangle gaps caused by incorrect backface culling and fix edge-of-screen artifacts by adding proper near-plane clipping.

**Architecture:** Two independent fixes applied in pipeline order. Fix 1 corrects the view-direction used in backface culling from a hardcoded `{0,0,1}` to a per-triangle centroid-based vector. Fix 2 adds Sutherland-Hodgman near-plane clipping in camera space before projection, replacing the current "reject whole triangle if any vertex off-screen" logic.

**Tech Stack:** C, raylib (display only), custom software rasterizer. Build: `make`. Run: `make run`.

## Global Constraints

- No new dependencies
- No new files except `test_clip.c` (temporary, can be deleted after task 2)
- Do not modify `point_in_triangle`, `draw_triangle_pixels`, `render_world`, camera/parser/engine code
- Do not run `git add` or `git commit` at any point

---

### Task 1: Fix backface culling — per-triangle view direction

**Files:**
- Modify: `src/rasterizer_module/draw.c:71-85` (`is_backfacing`)

**Interfaces:**
- Consumes: nothing from other tasks
- Produces: `is_backfacing(const vec3f triangleVerts[3]) -> bool` (signature unchanged)

- [ ] **Step 1: Understand current bug**

  Open `src/rasterizer_module/draw.c`. `is_backfacing` at line 71 uses `vec3f view_dir = {0.0f, 0.0f, 1.0f}` — a fixed direction. For perspective projection the correct vector is from the triangle's centroid toward the camera, which is at origin in camera space. Triangles at the screen periphery have a real view direction like `{-0.7, 0, 0.7}` that diverges from `{0,0,1}`, causing incorrect culling.

- [ ] **Step 2: Replace `is_backfacing` body**

  Replace the function body (lines 71–85) with:

  ```c
  bool is_backfacing(const vec3f triangleVerts[3]) {
    vec3f edge1 = vec3f_sub(triangleVerts[1], triangleVerts[0]);
    vec3f edge2 = vec3f_sub(triangleVerts[2], triangleVerts[0]);

    vec3f normal = vec_cross(edge1, edge2);
    normal = vec_normalize(normal);

    vec3f centroid = {
      (triangleVerts[0].x + triangleVerts[1].x + triangleVerts[2].x) / 3.0f,
      (triangleVerts[0].y + triangleVerts[1].y + triangleVerts[2].y) / 3.0f,
      (triangleVerts[0].z + triangleVerts[1].z + triangleVerts[2].z) / 3.0f,
    };
    vec3f view_dir = vec_normalize(vec3f_scale(centroid, -1.0f));

    float dot_nv = vec_dot(normal, view_dir);

    return (dot_nv >= 0.0f);
  }
  ```

- [ ] **Step 3: Build**

  ```bash
  make
  ```

  Expected: compiles with no errors. Warnings about unused params are OK.

- [ ] **Step 4: Visual test**

  ```bash
  make run
  ```

  Rotate the model with mouse/keys until it's near the screen edge. Triangles that previously flickered in/out should now render stably. Gaps along silhouette when model is off-center should be reduced or gone.

---

### Task 2: Add `clip_triangle_near_plane` and `project_cam`

**Files:**
- Modify: `src/rasterizer_module/coordinates.c` — add two new functions
- Modify: `src/rasterizer_module/coordinates.h` — declare both
- Create: `test_clip.c` (root of project, temporary test binary)

**Interfaces:**
- Consumes: `vec3f` from `types.h`
- Produces:
  - `int clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4])` — clips triangle against `z = near`, returns vertex count of output polygon (0–4), writes polygon into `out`
  - `bool project_cam(const world *world, vec3f cam_pos, vec3f *out)` — projects a camera-space point to screen coords; always returns true (caller guarantees `cam_pos.z > near`)

- [ ] **Step 1: Add `clip_triangle_near_plane` to `coordinates.c`**

  Append after `rotate_vector` (before `edgeFunction`):

  ```c
  int clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4]) {
    int out_count = 0;

    for (int i = 0; i < 3; i++) {
      vec3f curr = verts[i];
      vec3f prev = verts[(i + 2) % 3];

      bool curr_inside = curr.z >= near;
      bool prev_inside = prev.z >= near;

      if (curr_inside) {
        if (!prev_inside) {
          float t = (near - prev.z) / (curr.z - prev.z);
          out[out_count++] = (vec3f){
            prev.x + t * (curr.x - prev.x),
            prev.y + t * (curr.y - prev.y),
            near,
          };
        }
        out[out_count++] = curr;
      } else if (prev_inside) {
        float t = (near - prev.z) / (curr.z - prev.z);
        out[out_count++] = (vec3f){
          prev.x + t * (curr.x - prev.x),
          prev.y + t * (curr.y - prev.y),
          near,
        };
      }
    }

    return out_count;
  }
  ```

- [ ] **Step 2: Add `project_cam` to `coordinates.c`**

  Append directly after `clip_triangle_near_plane`:

  ```c
  bool project_cam(const world *world, vec3f cam_pos, vec3f *out) {
    float x_proj = (cam_pos.x / cam_pos.z) * world->cam->focal_length;
    float y_proj = (cam_pos.y / cam_pos.z) * world->cam->focal_length *
                   world->renderer->aspect_ratio;

    out->x = (x_proj + 1.0f) * 0.5f * world->renderer->screen_width;
    out->y = (1.0f - y_proj) * 0.5f * world->renderer->screen_height;
    out->z = cam_pos.z;

    return true;
  }
  ```

  Note: no NDC bounds check — near-plane clipping guarantees `cam_pos.z > 0`. Off-screen x/y are fine; `compute_triangle_bbox` clamps to screen.

- [ ] **Step 3: Declare both in `coordinates.h`**

  Add after the existing declarations:

  ```c
  int clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4]);
  bool project_cam(const world *world, vec3f cam_pos, vec3f *out);
  ```

- [ ] **Step 4: Write test for `clip_triangle_near_plane`**

  Create `test_clip.c` in the project root:

  ```c
  #include <assert.h>
  #include <math.h>
  #include <stdbool.h>
  #include <stdio.h>

  /* pull in just enough types — vec3f only, no raylib needed */
  typedef struct { float x, y, z; } vec3f;

  int clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4]);

  /* paste the implementation here for standalone compilation */
  int clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4]) {
    int out_count = 0;
    for (int i = 0; i < 3; i++) {
      vec3f curr = verts[i];
      vec3f prev = verts[(i + 2) % 3];
      bool curr_inside = curr.z >= near;
      bool prev_inside = prev.z >= near;
      if (curr_inside) {
        if (!prev_inside) {
          float t = (near - prev.z) / (curr.z - prev.z);
          out[out_count++] = (vec3f){
            prev.x + t * (curr.x - prev.x),
            prev.y + t * (curr.y - prev.y),
            near,
          };
        }
        out[out_count++] = curr;
      } else if (prev_inside) {
        float t = (near - prev.z) / (curr.z - prev.z);
        out[out_count++] = (vec3f){
          prev.x + t * (curr.x - prev.x),
          prev.y + t * (curr.y - prev.y),
          near,
        };
      }
    }
    return out_count;
  }

  static void assert_near(float a, float b, float eps) {
    assert(fabsf(a - b) < eps);
  }

  int main(void) {
    vec3f out[4];
    int n;

    /* all inside: output is identical 3-vertex triangle */
    vec3f all_in[3] = {{0,0,2},{1,0,2},{0,1,2}};
    n = clip_triangle_near_plane(all_in, 0.1f, out);
    assert(n == 3);

    /* all outside: clipped to nothing */
    vec3f all_out[3] = {{0,0,-1},{1,0,-1},{0,1,-1}};
    n = clip_triangle_near_plane(all_out, 0.1f, out);
    assert(n == 0);

    /* one vertex outside (vertex 0 at z=-0.5): produces quad (4 verts) */
    vec3f one_out[3] = {{0,0,-0.5f},{2,0,2},{0,2,2}};
    n = clip_triangle_near_plane(one_out, 0.1f, out);
    assert(n == 4);
    /* both intersection points must sit on the near plane */
    assert_near(out[0].z, 0.1f, 1e-5f);
    assert_near(out[3].z, 0.1f, 1e-5f);

    /* two vertices outside (0 and 1): produces 1 clipped triangle */
    vec3f two_out[3] = {{0,0,-1},{1,0,-1},{0,1,2}};
    n = clip_triangle_near_plane(two_out, 0.1f, out);
    assert(n == 3);
    assert_near(out[0].z, 0.1f, 1e-5f);
    assert_near(out[1].z, 0.1f, 1e-5f);

    printf("All clip tests passed\n");
    return 0;
  }
  ```

- [ ] **Step 5: Compile and run test**

  ```bash
  cc test_clip.c -lm -o test_clip && ./test_clip
  ```

  Expected output:
  ```
  All clip tests passed
  ```

- [ ] **Step 6: Build the main project**

  ```bash
  make
  ```

  Expected: no errors. `project_cam` and `clip_triangle_near_plane` will be unused until Task 3 — that's fine.

---

### Task 3: Refactor `render_mesh` to use near-plane clipping

**Files:**
- Modify: `src/rasterizer_module/draw.c` — replace `project_triangle`-based loop in `render_mesh`, remove `project_triangle`

**Interfaces:**
- Consumes:
  - `clip_triangle_near_plane(const vec3f verts[3], float near, vec3f out[4]) -> int` (from Task 2)
  - `project_cam(const world *world, vec3f cam_pos, vec3f *out) -> bool` (from Task 2)
  - `transform_triangle_to_camera`, `is_backfacing`, `draw_triangle_pixels`, `hsv_to_rgb` (existing, unchanged)
- Produces: corrected `render_mesh` with no partial-triangle culling

- [ ] **Step 1: Delete `project_triangle` from `draw.c`**

  Remove the entire `project_triangle` function (lines 87–108):

  ```c
  static bool project_triangle(world *world, const mesh *mesh, size_t i,
                               vec3f out[3]) {
    ...
  }
  ```

  It will be entirely replaced by the new `render_mesh` logic.

- [ ] **Step 2: Replace `render_mesh` body**

  Replace the body of `render_mesh` (lines 157–187) with:

  ```c
  void render_mesh(world *world, const mesh mesh) {
    assert((mesh.vertex_count % 3) == 0);

    for (size_t i = 0; i + 2 < mesh.vertex_count; i += 3) {
      vec3f v_cam[3];
      if (!transform_triangle_to_camera(&mesh, i, world->cam, v_cam))
        continue;

      if (is_backfacing(v_cam))
        continue;

      vec3f clipped[4];
      int clipped_count = clip_triangle_near_plane(v_cam, 0.1f, clipped);
      if (clipped_count < 3)
        continue;

      int triangle_index = i / 3;
      float hue = fmodf((float)triangle_index * 10.0f, 360.0f);
      Color color = hsv_to_rgb(hue, 1.0f, 1.0f);

      for (int t = 0; t + 2 < clipped_count; t++) {
        vec3f fan[3] = {clipped[0], clipped[t + 1], clipped[t + 2]};
        vec3f projected[3];

        for (int j = 0; j < 3; j++)
          project_cam(world, fan[j], &projected[j]);

        float dx01 = projected[0].x - projected[1].x;
        float dy01 = projected[0].y - projected[1].y;
        float dx12 = projected[1].x - projected[2].x;
        float dy12 = projected[1].y - projected[2].y;
        float dx20 = projected[2].x - projected[0].x;
        float dy20 = projected[2].y - projected[0].y;
        if (dx01*dx01 + dy01*dy01 < 1e-2f) continue;
        if (dx12*dx12 + dy12*dy12 < 1e-2f) continue;
        if (dx20*dx20 + dy20*dy20 < 1e-2f) continue;

        draw_triangle_pixels(world, projected[0], projected[1], projected[2], color);
      }
    }
  }
  ```

- [ ] **Step 3: Add `#include "coordinates.h"` if not already present**

  Check top of `draw.c` — `coordinates.h` is already included at line 9. No change needed.

- [ ] **Step 4: Build**

  ```bash
  make
  ```

  Expected: no errors, no warnings about `project_triangle` (it's removed). If there's a warning about `project` being unused in `coordinates.c`, that's acceptable — `project()` stays declared for potential future use.

- [ ] **Step 5: Visual test — near-plane clipping**

  ```bash
  make run
  ```

  Move camera very close to the model (walk into it). Previously: triangles would pop out completely as vertices crossed behind the camera. Now: triangles clip cleanly at the near plane with no popping.

- [ ] **Step 6: Visual test — edge of screen**

  Rotate pitch/yaw so the model sits at the edge of the screen. Previously: large chunks of mesh would vanish as projected vertices crossed NDC boundary. Now: triangles render up to the screen edge and are clipped by the bbox clamping in `compute_triangle_bbox`.

- [ ] **Step 7: Cleanup**

  Delete the temporary test file:

  ```bash
  rm test_clip.c test_clip
  ```
