# Rasterizer Bug Fixes: Backface Culling & Near-Plane Clipping

**Date:** 2026-06-23  
**Bugs:** Triangle gaps from incorrect backface culling; edge-of-screen weirdness from missing frustum clipping

---

## Bug 1: Incorrect Backface Culling

### Root Cause

`is_backfacing` in `draw.c` uses a fixed view direction `{0, 0, 1}` for all triangles. For perspective projection, the correct view direction is per-triangle: the vector from the triangle's centroid to the camera origin (which is at `{0,0,0}` in camera space). Triangles near the screen periphery have a real view direction that diverges significantly from `{0,0,1}`, causing front-facing triangles to be misclassified as back-facing. Rotating pitch/yaw moves triangles into and out of this bad zone, producing flickering gaps.

### Fix

In `draw.c:is_backfacing(const vec3f triangleVerts[3])`:

- Compute centroid: average of the 3 camera-space vertices
- Compute view direction: `normalize(-centroid)` (vector from centroid to camera at origin)
- Use that as `view_dir` instead of the hardcoded `{0, 0, 1}`

Signature unchanged. No other files affected.

---

## Bug 2: Edge-of-Screen Weirdness / Near-Plane Artifacts

### Root Cause

Two compounding issues:

1. `project()` clamps `z` to `near_plane` (0.001) instead of clipping. Vertices behind or at the near plane get projected with a near-zero z, producing extreme x/y values.
2. `project_triangle()` uses `bool in = project(v0) && project(v1) && project(v2)`. If any single vertex projects outside NDC `[-1, 1]`, the entire triangle is discarded. Triangles that partially cross the screen boundary disappear entirely instead of being drawn clipped.

Result: as pitch/yaw moves the model toward the screen edge, triangles pop out.

### Fix

**New function:** `clip_triangle_near_plane` in `coordinates.c/.h`

- Input: 3 camera-space vertices, near-plane z value
- Algorithm: Sutherland-Hodgman against the plane `z = near_plane`
- Output: clipped polygon as array of up to 4 vertices + vertex count (0 = fully clipped)
- Caller fans the polygon into 1–2 triangles

**Modified pipeline in `render_mesh` (`draw.c`):**

1. Transform to camera space → `v_cam[3]`
2. Backface cull (using fix 1 above)
3. Clip against `z = near_plane` → polygon (0–4 verts)
4. If 0 verts: skip triangle
5. Fan polygon into triangles (triangle fan from vertex 0)
6. For each clipped triangle: project each vertex with `project()` individually
7. Draw with `draw_triangle_pixels`

**Modified `project()`:**

- Remove the NDC bounds check (`if (x_proj < -1 || x_proj > 1 ...)`). After near-plane clipping, vertices are guaranteed to be in front of camera. Projection can produce x/y > 1 (off-screen) which is fine — `compute_triangle_bbox` already clamps to screen bounds.
- Remove the near-plane z clamping hack (now handled by clipping)
- Return type can remain `bool`; always returns `true` after this change (or becomes `void`)

**Modified `project_triangle()`:**

- Remove entirely, or reduce to just calling `project()` per vertex without the `bool in` early-exit logic. Folded into the new per-clipped-triangle loop in `render_mesh`.

### Files Changed

| File | Change |
|------|--------|
| `src/rasterizer_module/coordinates.c` | Add `clip_triangle_near_plane`; simplify `project()` |
| `src/rasterizer_module/coordinates.h` | Declare `clip_triangle_near_plane` |
| `src/rasterizer_module/draw.c` | Refactor `render_mesh` pipeline; remove or gut `project_triangle` |

---

## What Does NOT Change

- `point_in_triangle` and top-left rule logic (not the source of these bugs)
- `draw_triangle_pixels` (unchanged)
- `is_backfacing` signature
- `render_world`, `render_mesh` callers
- Camera, mesh, or parser code
