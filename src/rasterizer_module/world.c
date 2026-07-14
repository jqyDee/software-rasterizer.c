#include "world.h"

#include <math/vec.h>
#include <renderer/lighting.h>
#include <stdio.h>
#include <stdlib.h>

#include "assets/primitives.h"
#include "engine/cam.h"
#include "engine/instance.h"
#include "engine/scene.h"
#include "engine/texture.h"
#include "raylib.h"

bool init_world(world *world, int display_w, int display_h) {
  renderer *rendererM = malloc(sizeof(renderer));
  if (!rendererM)
    return false;
  *rendererM = (renderer){0};

  world->undo_len = 0;
  world->redo_len = 0;
  world->perf = (perf_stats){0};

  init_cam(&world->game_cam);
  world->debug_cam = world->game_cam;
  world->cam = &world->game_cam;
  world->player_vy = 0.0f;

  world->renderer = rendererM;
  world->settings = (settings){
      .show_debug_gui = false,
      .render_width = BASE_RENDER_WIDTH,
      .parallel_cutoff_rows = CUT_OFF_PARALLEL_DRAWING,
      .near_plane = NEAR_PLANE,
      .collision_enabled = true,
      .mouse_sensitivity = 0.003f,
      .move_speed = 5.0f,
      .debug_cam_speed_factor = 3.0f,
      .gravity = 10.0f,
      .jump_speed = 5.0f,
      .ground_y = 0.0f,
      .camera_radius = 0.35f,
      .seam_bias = 0.0f,
      .ambient_light = 0.8f,
      .lighting_mode = LIGHTING_GPU_PHONG,
  };
  world->selected_instance = -1;
  world->mesh_data = NULL;
  world->mesh_data_count = 0;
  world->instances = NULL;
  world->instance_count = 0;
  world->instance_capacity = 0;
  world->tex_lib = NULL;
  world->tex_lib_count = 0;

  // TODO: add multiple light source and light source editing in the scene
  // editor
  world->lights[0] = (light_t){
      .light_dir = vec_normalize(((vec3f){0.0f, 1.0f, 0.0f})),
      .color = WHITE,
      .intensity = 0.5f,
  };
  world->light_count = 1;

  resize_renderer_to(world, display_w, display_h);
  load_texture_library(world);

  if (!load_objs_files(world)) {
    destroy_world(world);
    return false;
  }

  /* default instances: one per discovered obj + floor plane */
  size_t n_objs = world->mesh_data_count - NUM_PRIMITIVES;
  size_t init_n = n_objs + 1;
  world->instance_capacity = init_n < 8 ? 8 : init_n * 2;
  world->instances = malloc(world->instance_capacity * sizeof(mesh_instance));
  if (!world->instances) {
    destroy_world(world);
    return false;
  }

  for (size_t i = 0; i < n_objs; i++) {
    int midx = (int)(NUM_PRIMITIVES + i);
    mesh_instance inst = {
        .mesh_idx = midx,
        .pos = {5.0f * (float)i, 0.0f, 0.0f},
        .rotation = {0, 0, 0},
        .scale = {1, 1, 1},
        .tex_pixels = NULL,
        .tex_w = 0,
        .tex_h = 0,
    };
    inst.tex_path[0] = '\0';
    const char *atp = world->mesh_data[midx].auto_tex_path;
    if (atp[0] != '\0')
      snprintf(inst.tex_path, sizeof(inst.tex_path), "%s", atp);
    world->instances[world->instance_count++] = inst;
  }
  /* floor: plane primitive scaled 20×20, dropped to y=-1 */
  {
    mesh_instance floor_inst = {
        .mesh_idx = 2,
        .pos = {0, -1, 0},
        .rotation = {0, 0, 0},
        .scale = {20, 1, 20},
        .tex_pixels = NULL,
        .tex_w = 0,
        .tex_h = 0,
    };
    floor_inst.tex_path[0] = '\0';
    world->instances[world->instance_count++] = floor_inst;
  }

  /* try to restore last session; silently keep defaults if not found */
  scene_load(world, "assets/scenes/scene.json");

  /* load textures for all instances that have a tex_path */
  reload_instance_textures(world);

  return true;
}

void destroy_world(world *world) {
  if (!world)
    return;

  if (world->renderer->depthbuffer)
    free(world->renderer->depthbuffer);
  if (world->renderer->framebuffer)
    free(world->renderer->framebuffer);
  if (world->renderer->idbuffer)
    free(world->renderer->idbuffer);

  if (world->renderer)
    free(world->renderer);

  if (world->mesh_data) {
    for (size_t i = 0; i < world->mesh_data_count; i++) {
      free(world->mesh_data[i].vertices);
      free(world->mesh_data[i].uvs);
    }
    free(world->mesh_data);
  }

  if (world->instances) {
    for (size_t i = 0; i < world->instance_count; i++)
      free(world->instances[i].tex_pixels);
    free(world->instances);
  }

  free_texture_library(world);

  for (int i = 0; i < world->undo_len; i++)
    free(world->undo_stack[i].instances);
  for (int i = 0; i < world->redo_len; i++)
    free(world->redo_stack[i].instances);
}
