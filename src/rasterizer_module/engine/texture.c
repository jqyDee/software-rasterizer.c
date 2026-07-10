#include "texture.h"
#include "../testing.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "undo.h"
#include "dir.h"

#ifdef STATIC_RELEASE
#include "embedded_assets.h"
#endif

void free_texture_library(world *world) {
  for (int i = 0; i < world->tex_lib_count; i++)
    free(world->tex_lib[i].pixels);
  free(world->tex_lib);
  world->tex_lib = NULL;
  world->tex_lib_count = 0;
}

void load_texture_library(world *world) {
  free_texture_library(world);
#ifdef STATIC_RELEASE
  int count = 0;
  for (int i = 0; i < embedded_asset_count; i++) {
    const char *dot = strrchr(embedded_assets[i].path, '.');
    if (dot && (strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".jpg") == 0 ||
                strcasecmp(dot, ".jpeg") == 0))
      count++;
  }
  if (count == 0)
    return;
  world->tex_lib = malloc((size_t)count * sizeof(texture_entry));
  if (!world->tex_lib)
    return;
  int loaded = 0;
  for (int i = 0; i < embedded_asset_count; i++) {
    const char *p = embedded_assets[i].path;
    const char *dot = strrchr(p, '.');
    if (!dot || (strcasecmp(dot, ".png") != 0 && strcasecmp(dot, ".jpg") != 0 &&
                 strcasecmp(dot, ".jpeg") != 0))
      continue;
    Image img = LoadImageFromMemory(dot, embedded_assets[i].data,
                                    (int)embedded_assets[i].size);
    if (!img.data)
      continue;
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    texture_entry *te = &world->tex_lib[loaded++];
    snprintf(te->path, sizeof(te->path), "%s", p);
    const char *slash = strrchr(p, '/');
    snprintf(te->name, sizeof(te->name), "%s", slash ? slash + 1 : p);
    te->w = img.width;
    te->h = img.height;
    te->pixels = (Color *)LoadImageColors(img);
    UnloadImage(img);
  }
  world->tex_lib_count = loaded;
  fprintf(stderr, "INFO: texture library: %d textures loaded (embedded)\n",
          loaded);
#else
  const char *exts[] = {".png", ".jpg", ".jpeg"};
  int count = 0;
  char **paths = scan_dir(ASSETS_TEX_DIR, exts, 3, &count);
  if (!paths || count == 0) {
    free(paths);
    return;
  }

  world->tex_lib = malloc((size_t)count * sizeof(texture_entry));
  if (!world->tex_lib) {
    for (int i = 0; i < count; i++)
      free(paths[i]);
    free(paths);
    return;
  }

  int loaded = 0;
  for (int i = 0; i < count; i++) {
    Image img = LoadImage(paths[i]);
    if (!img.data) {
      free(paths[i]);
      continue;
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    texture_entry *te = &world->tex_lib[loaded++];
    snprintf(te->path, sizeof(te->path), "%s", paths[i]);
    const char *slash = strrchr(paths[i], '/');
    snprintf(te->name, sizeof(te->name), "%s", slash ? slash + 1 : paths[i]);
    te->w = img.width;
    te->h = img.height;
    te->pixels = (Color *)LoadImageColors(img);
    UnloadImage(img);
    free(paths[i]);
  }
  free(paths);
  world->tex_lib_count = loaded;
  fprintf(stderr, "INFO: texture library: %d textures loaded\n", loaded);
#endif
}

void apply_lib_texture(world *world, int inst_idx, int lib_idx) {
  if (inst_idx < 0 || inst_idx >= (int)world->instance_count)
    return;
  scene_push_undo(world);
  mesh_instance *inst = &world->instances[inst_idx];
  free(inst->tex_pixels);
  inst->tex_pixels = NULL;
  inst->tex_w = inst->tex_h = 0;
  inst->tex_path[0] = '\0';
  if (lib_idx < 0 || lib_idx >= world->tex_lib_count)
    return;
  texture_entry *te = &world->tex_lib[lib_idx];
  size_t sz = (size_t)(te->w * te->h) * sizeof(Color);
  inst->tex_pixels = malloc(sz);
  if (!inst->tex_pixels)
    return;
  memcpy(inst->tex_pixels, te->pixels, sz);
  inst->tex_w = te->w;
  inst->tex_h = te->h;
  snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", te->path);
}

void init_texture(renderer *renderer) {
  Image image =
      GenImageColor(renderer->screen_width, renderer->screen_height, BLANK);
  renderer->screen_texture = LoadTextureFromImage(image);
  UnloadImage(image);
}
