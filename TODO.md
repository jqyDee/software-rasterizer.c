# TODO

## Raylib macOS viewport fix distribution

`libs/raylib/src/rcore.c` contains a local patch (`raylib-macos-viewport.patch`) that fixes a
macOS-specific bug: without `FLAG_WINDOW_HIGHDPI`, raylib multiplies the GL viewport by the
Retina DPI scale but the GL framebuffer stays at logical resolution, causing only the
bottom-left quadrant of the scene to render until the window is manually resized.

Currently the patch auto-applies on first `make` (when the raylib dylib is missing).
Pick one of the options below to make the fix permanent and shareable.

---

### Option A — Fork raylib (recommended)

1. Fork `https://github.com/raysan5/raylib` to your GitHub account.
2. Inside `libs/raylib`, commit the patched `rcore.c` and push to your fork.
3. Update the submodule URL:
   ```
   git submodule set-url libs/raylib https://github.com/YOUR_USERNAME/raylib.git
   git add .gitmodules libs/raylib
   git commit -m "point raylib submodule to fork with macOS viewport fix"
   ```
4. Others get the fix automatically via `git clone --recurse-submodules`.

---

### Option B — Patch file (current state)

`raylib-macos-viewport.patch` is committed to the repo root. The Makefile applies it
automatically before building raylib. No fork needed, but requires the patch to be
re-applied after updating the raylib submodule.

To re-apply manually:
```
cd libs/raylib && git apply ../../raylib-macos-viewport.patch
```

---

### Option C — Submit upstream PR to raylib (best long-term)


The bug is genuine and affects all macOS users of raylib without `FLAG_WINDOW_HIGHDPI`.
Open a PR at `https://github.com/raysan5/raylib` with the change in `src/rcore.c`.
If merged: revert local patch, update submodule to a release that includes the fix,
delete `raylib-macos-viewport.patch`.

---

## Mario Kart-like racing game (in progress)

**Phase 1 roadmap — DONE.** Kart entity, gamepad+keyboard input, ground
movement, drift→boost state machine, jump/air physics, third-person follow
camera (`kart_update_cameras`, `game/kart.c`), track collision + checkpoints/
laps, kart rendering, split-screen (up to 4P, `renderer/renderer.c`). Debug
free-fly cam (F1) still available alongside the game cam. HUD
(`draw_kart_hud`, `ui/gui_kart.c`) shows speed/drift/boost/ground-air state
and lap/timer per player. See `game/kart_tuning.h` for the tuning parameters
(accel/friction/drift/boost/gravity/jump), editable live in the Kart debug
window.

**Phase 2 roadmap — not started, ranked by cost/impact:**

1. **Race placement + finish condition** (cheapest, do first) — `kart->lap`/
   `next_checkpoint`/`lap_time`/`best_lap_time` already exist per-kart
   (`game/kart.h`); needs cross-kart comparison each frame to rank karts
   live (1st/2nd/3rd/4th, shown in HUD) and a "race finished" state once a
   kart completes the target lap count (currently laps just increment
   forever with no win condition). Turns this from a driving sandbox into
   an actual race.
2. **Items** — the core Mario Kart mechanic, currently 0% built. Start with
   one hazard (banana peel: static world object, triggers a spin-out on
   kart collision) and one pickup (speed boost pad, reuses the existing
   `boost_visual`/speed-multiplier plumbing already in `kart_update`).
   Needs: pickup spawn points (track data?), a collision check against kart
   position each frame, and a pickup/item-state field on `kart`.
3. **AI/bots** — biggest lift. Needed for solo play without split-screen.
   Path-following off the existing track spline (`track_sample_pos`,
   `game/track.c`, already used for start-grid placement) to drive a
   steering target, feeding into the same `controller_input` struct real
   karts already consume in `kart_update` — reuse the input path, don't
   build a separate AI-specific movement model.

**Smaller polish, any time:** start countdown (3-2-1-GO) before a race
begins, minimap using existing `track_data` points.

---

## Split-screen rendering performance (follow-up to Phase 2)

Split-screen (per-kart viewports, `renderer.c`'s `render()` loop calling
`build_screen_tris`/bin/`draw_tiles_parallel` once per kart) works correctly,
but CPU cost still doesn't scale as well as it could with player count.
Debug stats overlay (Stats window) has instrumentation for all of this now:
`PERF_RENDER_GEOM`/`_GEOM_XFORM`, `PERF_RENDER_BINNING`, `PERF_RENDER_RASTER`
+ `PERF_RASTER_ITER`/`_EDGE_PASS`/`_DEPTH_PASS` (shown as `fill%`/`overdraw`
in the Stats window), and 4 debug skip-toggles in the Renderer window
(texture/normal-buf/albedo-buf/framebuffer) for isolating per-fragment
shading cost by elimination.

**DONE — frustum/visibility culling per viewport.** `instance_potentially_visible`
(`renderer/geometry.c`) runs a conservative bounding-sphere-vs-frustum-cone
test per instance per viewport, before `build_screen_tris`'s per-triangle
loop, so instances outside a given viewport's view skip the full
transform+clip+light+project cost entirely. Already in place — the section
below describing this as future work is stale.

**DONE — object→world transform de-duplication.** The object→world part of
the per-triangle transform is camera-independent, so split-screen's extra
viewport passes were redoing it once per kart for the same result. Now
cached per-instance per-frame (`world->world_verts_cache`,
`reset_world_vertex_cache` + the cache-fill in `build_screen_tris`,
`renderer/geometry.c`). Confirmed via `PERF_RENDER_GEOM_XFORM`: 4-player
xform cost dropped from ~1.0ms back to the 1-player baseline.

**DONE — scanline x-range tightening in the rasterizer.** `draw_triangle_pixels_tiled`
(`renderer/draw.c`) used to brute-test every pixel in a triangle's
tile-clipped bounding box; `narrow_edge_bound` now analytically bounds each
scanline's x-range first (padded ±1px, exact per-pixel test unchanged as the
correctness source of truth). Fill% (useful-test / total-test ratio) went
from ~35-40% to 85%+.

**DONE — merged tile binning + draw into one pass.** `render()`
(`renderer/renderer.c`) now accumulates every active viewport's triangles
into a shared `screen_triangles[]` via `accumulate_viewport_tris` (running
offset, `build_screen_tris` takes a `max_out` remaining-capacity param
instead of hardcoding `MAX_SCREEN_TRIS` so appends can't overflow), then
runs `compute_triangles_per_tile`/`bin_triangles_into_tiles`/
`draw_tiles_parallel` **once** over a tile grid spanning the whole
framebuffer instead of once per viewport. Net effect is scenario-dependent:
a wash (or very slightly worse, single dispatch overhead) when load is even
across viewports, a real win when it's uneven (one kart facing dense
geometry, another open space) since OpenMP now load-balances across *all*
tiles from *all* viewports in one parallel region instead of leaving threads
idle in a light viewport's own separate pass. Kept — the uneven case is the
one that matters in practice.

**Remaining, not yet done:**

### Per-fragment shading cost (the actual remaining bottleneck)

Isolated via the 4 debug skip-toggles: at 4-player split-screen, texture
sampling / normal-buffer encode (`sqrt` in `vec_normalize`) / shadow blend +
framebuffer write each cost roughly ~1-2ms, totalling ~75% of raster time.
This is legitimate work (not redundant computation like the two fixes
above) that scales with total shaded pixels across viewports. Two different
directions, not yet decided between:
- **Interleave the G-buffer** (depth/normal/albedo/id currently 5 separate
  heap buffers touched per fragment — real cache/bandwidth contention
  between them) into one packed struct-of-pixel array. Bigger refactor,
  touches the GPU upload path and `phong.fs` too.
- **Dynamic resolution scaling** by player count (lower `render_width`
  automatically at 3-4P, like real split-screen racing games do) — doesn't
  reduce per-pixel cost, reduces total pixel count instead. Simpler.

---

## Engine/Game split (future refactor)

Make rasterizer engine standalone by separating graphics code from game logic.

**Goal:** enable engine reuse across projects, cleaner architectural boundary.

**Proposed structure:**
```
libs/engine/
├── include/
│   ├── engine.h         ← public API (render, project, etc)
│   └── types.h          ← Color, Renderer, Triangle, Matrix
└── src/
    ├── rasterizer.c     ← barycentric rasterization, depth test
    ├── coordinates.c    ← projection, transforms, NDC→screen
    ├── framebuffer.c    ← framebuffer/depthbuffer ops
    └── math.c           ← matrix, vector math

src/game/
├── rasterizer.c         ← plugin entry, game render loop
├── scene.h/c            ← World, Camera, Mesh state
├── update.c             ← game logic, camera control
├── draw.c               ← which meshes to render
└── types.h              ← game-only types
```

**Defer until:** second project or heavy engine reuse justifies the refactor cost.
