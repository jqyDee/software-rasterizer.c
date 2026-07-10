#include "undo.h"
#include "../testing.h"

#include <stdlib.h>
#include <string.h>

#include "../world.h"
#include "instance.h"

INTERNAL undo_frame_t snapshot_current(world *world) {
  undo_frame_t f = {
      .instance_count = world->instance_count,
      .selected_instance = world->selected_instance,
      .instances = malloc(world->instance_count * sizeof(mesh_instance)),
  };
  if (f.instances) {
    memcpy(f.instances, world->instances,
           world->instance_count * sizeof(mesh_instance));
    for (size_t i = 0; i < f.instance_count; i++)
      f.instances[i].tex_pixels = NULL; /* derived from tex_path on restore */
  }
  return f;
}

INTERNAL bool snapshot_equals_current(const undo_frame_t *f, const world *world) {
  if (f->instance_count != world->instance_count)
    return false;
  if (f->selected_instance != world->selected_instance)
    return false;
  for (size_t i = 0; i < f->instance_count; i++) {
    const mesh_instance *a = &f->instances[i];
    const mesh_instance *b = &world->instances[i];
    if (a->mesh_idx != b->mesh_idx)
      return false;
    if (memcmp(&a->pos, &b->pos, sizeof(vec3f)))
      return false;
    if (memcmp(&a->rotation, &b->rotation, sizeof(vec3f)))
      return false;
    if (memcmp(&a->scale, &b->scale, sizeof(vec3f)))
      return false;
    if (strncmp(a->tex_path, b->tex_path, sizeof(a->tex_path)))
      return false;
  }
  return true;
}

INTERNAL void restore_snapshot(world *world, undo_frame_t *f) {
  for (size_t i = 0; i < world->instance_count; i++) {
    free(world->instances[i].tex_pixels);
    world->instances[i].tex_pixels = NULL;
  }

  if (f->instance_count > world->instance_capacity) {
    mesh_instance *ni =
        realloc(world->instances, f->instance_count * sizeof(mesh_instance));
    if (!ni)
      return;
    world->instances = ni;
    world->instance_capacity = f->instance_count;
  }
  world->instance_count = f->instance_count;
  world->selected_instance = f->selected_instance;
  memcpy(world->instances, f->instances,
         f->instance_count * sizeof(mesh_instance));
  free(f->instances);
  f->instances = NULL;
  reload_instance_textures(world);
}

void scene_push_undo(world *world) {
  if (world->instance_count == 0)
    return;
  /* clear redo */
  for (int i = 0; i < world->redo_len; i++)
    free(world->redo_stack[i].instances);
  world->redo_len = 0;

  /* drop oldest if full */
  if (world->undo_len == MAX_UNDO) {
    free(world->undo_stack[0].instances);
    memmove(&world->undo_stack[0], &world->undo_stack[1],
            (MAX_UNDO - 1) * sizeof(undo_frame_t));
    world->undo_len--;
  }

  undo_frame_t f = snapshot_current(world);
  if (!f.instances && world->instance_count > 0)
    return; /* alloc failed */
  world->undo_stack[world->undo_len++] = f;
}

void scene_undo(world *world) {
  if (world->undo_len == 0)
    return;
  bool noop =
      snapshot_equals_current(&world->undo_stack[world->undo_len - 1], world);
  if (!noop && world->redo_len < MAX_UNDO) {
    undo_frame_t f = snapshot_current(world);
    if (f.instances || world->instance_count == 0)
      world->redo_stack[world->redo_len++] = f;
  }
  restore_snapshot(world, &world->undo_stack[--world->undo_len]);
}

void scene_redo(world *world) {
  if (world->redo_len == 0)
    return;
  bool noop =
      snapshot_equals_current(&world->redo_stack[world->redo_len - 1], world);
  if (!noop && world->undo_len < MAX_UNDO) {
    undo_frame_t f = snapshot_current(world);
    if (f.instances || world->instance_count == 0)
      world->undo_stack[world->undo_len++] = f;
  }
  restore_snapshot(world, &world->redo_stack[--world->redo_len]);
}
