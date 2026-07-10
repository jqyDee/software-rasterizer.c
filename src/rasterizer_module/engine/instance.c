#include "instance.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef STATIC_RELEASE
#include "embedded_assets.h"
#endif

void add_instance(world *world, int mesh_idx) {
  scene_push_undo(world);
  if (world->instance_count >= world->instance_capacity) {
    size_t new_cap =
        world->instance_capacity ? world->instance_capacity * 2 : 8;
    mesh_instance *ni =
        realloc(world->instances, new_cap * sizeof(mesh_instance));
    if (!ni)
      return;
    world->instances = ni;
    world->instance_capacity = new_cap;
  }
  /* spawn in front of camera */
  float sy = sinf(world->cam->yaw), cy = cosf(world->cam->yaw);
  vec3f spawn = {world->cam->pos.x + 3.0f * sy, world->cam->pos.y,
                 world->cam->pos.z + 3.0f * cy};
  mesh_instance *inst = &world->instances[world->instance_count];
  *inst = (mesh_instance){
      .mesh_idx = mesh_idx,
      .pos = spawn,
      .rotation = {0, 0, 0},
      .scale = {1, 1, 1},
      .tex_pixels = NULL,
      .tex_w = 0,
      .tex_h = 0,
  };
  inst->tex_path[0] = '\0';
  world->selected_instance = (int)world->instance_count;
  world->instance_count++;
  /* auto-load texture if mesh has one — must increment count first for bounds
   * check */
  const char *atp = world->mesh_data[mesh_idx].auto_tex_path;
  if (atp[0] != '\0') {
    snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", atp);
    load_instance_texture(world, world->selected_instance, inst->tex_path);
  }
}

void remove_instance(world *world, int idx) {
  if (idx < 0 || idx >= (int)world->instance_count)
    return;
  scene_push_undo(world);
  free(world->instances[idx].tex_pixels);
  int last = (int)world->instance_count - 1;
  world->instances[idx] = world->instances[last];
  world->instances[last].tex_pixels =
      NULL; /* ownership moved, don't double-free */
  world->instance_count--;
  if (world->selected_instance == idx)
    world->selected_instance = -1;
  else if (world->selected_instance == last)
    world->selected_instance = idx;
}

void load_instance_texture(world *world, int inst_idx, const char *path) {
  if (inst_idx < 0 || inst_idx >= (int)world->instance_count)
    return;
  mesh_instance *inst = &world->instances[inst_idx];
  free(inst->tex_pixels);
  inst->tex_pixels = NULL;
  inst->tex_w = inst->tex_h = 0;
  if (!path || path[0] == '\0') {
    inst->tex_path[0] = '\0';
    return;
  }
#ifdef STATIC_RELEASE
  const embedded_asset_t *ea = find_embedded_asset(path);
  Image img =
      ea ? LoadImageFromMemory(strrchr(path, '.'), ea->data, (int)ea->size)
         : LoadImage(path);
#else
  Image img = LoadImage(path);
#endif
  if (!img.data) {
    fprintf(stderr, "load_instance_texture: cannot load %s\n", path);
    return;
  }
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  inst->tex_w = img.width;
  inst->tex_h = img.height;
  inst->tex_pixels = (Color *)LoadImageColors(img);
  UnloadImage(img);
  snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", path);
  fprintf(stderr, "load_instance_texture: loaded %s (%dx%d)\n", path,
          inst->tex_w, inst->tex_h);
}

void reload_instance_textures(world *world) {
  for (size_t i = 0; i < world->instance_count; i++) {
    mesh_instance *inst = &world->instances[i];
    if (inst->tex_path[0] == '\0')
      continue;
    free(inst->tex_pixels);
    inst->tex_pixels = NULL;
    inst->tex_w = inst->tex_h = 0;
#ifdef STATIC_RELEASE
    const embedded_asset_t *rea = find_embedded_asset(inst->tex_path);
    Image img = rea ? LoadImageFromMemory(strrchr(inst->tex_path, '.'),
                                          rea->data, (int)rea->size)
                    : LoadImage(inst->tex_path);
#else
    Image img = LoadImage(inst->tex_path);
#endif
    if (!img.data)
      continue;
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    inst->tex_w = img.width;
    inst->tex_h = img.height;
    inst->tex_pixels = (Color *)LoadImageColors(img);
    UnloadImage(img);
  }
}
