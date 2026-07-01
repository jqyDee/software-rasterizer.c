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
