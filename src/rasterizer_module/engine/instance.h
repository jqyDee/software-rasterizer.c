#pragma once

#include "../world.h"

void load_instance_texture(world *world, int inst_idx, const char *path);
void remove_instance(world *world, int idx);
void load_instance_texture(world *world, int inst_idx, const char *path);
void reload_instance_textures(world *world);

void add_instance(world *world, int mesh_idx);
void remove_instance(world *world, int idx);

/* true for the track instance and any kart's own visual — these are
 * engine-managed (rebuilt/re-linked on scene load, repositioned every
 * frame) and must not be deleted or hand-edited via the scene editor */
bool instance_is_protected(const world *world, int idx);
