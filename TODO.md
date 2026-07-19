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

Single-player car physics + camera, then split-screen multiplayer.

**Design decisions:**
- Hybrid physics: arcade ground movement (steering + friction curves), realistic air physics (gravity + rotation)
- Drifting: state machine (NONE → DRIFTING → BOOST_READY). Hold B to drift, release to apply boost
- Jump: press A when grounded, adds upward velocity. Mid-air: stick rotates kart
- Camera: third-person follow, looks ahead based on velocity
- Input: steering (analog X or A/D), accel/brake (button hold), jump (A), drift (B)
- Keep debug camera (F1 free cam) for map editing
- Track collision v1: raycast ground, simple bounds check for off-track

**Implementation roadmap (order matters):**
1. **Kart entity** — struct with position, velocity, forward_dir, rotation, ground/drift/air state
2. **Input system** — gamepad + keyboard mapping (steering, accel, brake, jump, drift)
3. **Ground movement** — steering rotates kart, accel/brake modify velocity, apply friction
4. **Drifting** — state machine, steering while drifting modifies slip angle, accumulate boost
5. **Jump & air** — button press → upward velocity, gravity pulls down, stick rotates in air
6. **Camera** — follow behind kart, look ahead scaled by speed
7. **Track collision** — raycast down for grounding, bounding box for off-track zones
8. **Kart rendering** — draw kart model in world (use existing mesh system)
9. **Split-screen** — render two cameras side-by-side or top/bottom (phase 2)

**Tuning parameters (placeholder values):**
- `ACCEL_RATE`: how fast velocity increases per second
- `FRICTION_COEFFICIENT`: ground drag
- `DRIFT_FRICTION_MULTIPLIER`: how slippery while drifting
- `BOOST_ACCUMULATION_RATE`: meter fill speed
- `BOOST_SPEED_MULTIPLIER`: speed boost magnitude
- `MAX_DRIFT_ANGLE`: sideways slip limit
- `GRAVITY`: air fall speed
- `JUMP_INITIAL_VELOCITY`: upward impulse

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
