#pragma once

typedef struct world_s world;

/* 2D top-down track editor. Runs instead of the 3D pipeline while
 * settings.track_editor_active — handles its own mouse/keyboard input
 * and draws with raylib 2D primitives directly. */
void track_editor_update_and_draw(world *world);
