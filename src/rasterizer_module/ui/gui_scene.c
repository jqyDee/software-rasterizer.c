#include <stdio.h>
#include <string.h>

#include "gui_scene.h"
#include "gui_window.h"

#include "../engine/instance.h"
#include "../engine/scene.h"
#include "../game/kart.h"
#include "../game/track.h"

void draw_scene_window(world *world, gui_state_t *gs, float delta_time) {
  const float SCENE_W = 220.0f;
  const float SCENE_CONTENT = PAD + 2.0f * ROW_H + GAP +
                              (float)world->instance_count * ROW_H + GAP +
                              4.0f * ROW_H + PAD;

  if (do_window(&gs->scene_win, "Scene", SCENE_W, SCENE_CONTENT)) {
    static char scene_name[64] = "scene";
    static bool name_edit = false;
    static char status_msg[80] = "";
    static float status_timer = 0.0f;
    static bool status_ok = true;
    status_timer -= delta_time;

    float x = gs->scene_win.pos.x + PAD;
    float aw = SCENE_W - PAD * 2.0f;

    float y_ctrl = gs->scene_win.pos.y + TITLE_H + PAD;
    float y_remove = y_ctrl + ROW_H;
    float y_list = y_remove + ROW_H + GAP;
    float y_filesep = y_list + (float)world->instance_count * ROW_H + GAP;
    float y_name = y_filesep + ROW_H;
    float y_filebtns = y_name + ROW_H;
    float y_status = y_filebtns + ROW_H;

    /* instance list (back layer — drawn first) — the track instance and
     * kart visuals are engine-managed (see instance_is_protected) and
     * shown locked: greyed out, not selectable, can't be dragged or
     * removed */
    for (int i = 0; i < (int)world->instance_count; i++) {
      const char *mname = world->mesh_data[world->instances[i].mesh_idx].name;
      bool locked = instance_is_protected(world, i);
      char label[64];
      snprintf(label, sizeof(label), locked ? "[%d] %s (locked)" : "[%d] %s",
               i, mname);

      bool selected = (world->selected_instance == i);
      if (locked)
        GuiDisable();
      GuiToggle((Rectangle){x, y_list + (float)i * ROW_H, aw, ROW_H - 2}, label,
                &selected);
      if (locked)
        GuiEnable();
      if (!locked) {
        if (selected && world->selected_instance != i)
          world->selected_instance = i;
        else if (!selected && world->selected_instance == i)
          world->selected_instance = -1;
      }
    }

    if (world->selected_instance < 0 ||
        instance_is_protected(world, world->selected_instance))
      GuiDisable();
    if (GuiButton((Rectangle){x, y_remove, aw, ROW_H - 2}, "Remove Selected"))
      remove_instance(world, world->selected_instance);
    if (world->selected_instance < 0 ||
        instance_is_protected(world, world->selected_instance))
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
        if (status_ok) {
          reload_instance_textures(world);
          /* scene_load wipes and rebuilds world->instances from scratch, so
           * the track mesh and every kart's visual link (kart->instance_idx)
           * must be re-established — same sequence init_world runs at boot */
          track_build_mesh(world);
          link_kart_instances(world);
          kart_spawn_at_start(world);
        }
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
                       &gs->sel_mesh, gs->dd_open))
      gs->dd_open = !gs->dd_open;
    if (!gs->dd_open) {
      if (GuiButton((Rectangle){x + dd_w + GAP, y_ctrl, add_w, ROW_H - 2},
                    "+ Add"))
        add_instance(world, gs->sel_mesh);
    }
  } else {
    gs->dd_open = false;
  }
}
