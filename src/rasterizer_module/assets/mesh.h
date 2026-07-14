#pragma once

#include "raylib.h"
#include <stddef.h>

#include "../math/vec.h"

typedef struct world_s world;

typedef struct mesh_s {
  vec3f *vertices; /* flat: 3 per triangle */
  vec2f *uvs;      /* parallel to vertices; NULL = no UVs */
  vec3f *normals;  /* parallel to vertices; NULL = no per-vertex normals (falls back to flat face normal) */
  size_t vertex_count;
  char name[256];          /* owned copy of display name */
  char auto_tex_path[512]; /* auto-detected texture path for this mesh, or "" */
  vec3f aabb_min, aabb_max; /* object-space bounds, computed once on load */
} mesh;

typedef struct mesh_instance_s {
  int mesh_idx; /* index into world->mesh_data */
  vec3f pos;
  vec3f rotation;    /* pitch/yaw/roll in degrees */
  vec3f scale;       /* per-axis non-uniform scale */
  Color *tex_pixels; /* NULL = no texture */
  int tex_w, tex_h;
  char
      tex_path[256]; /* path used to load tex_pixels, persists across reloads */
} mesh_instance;

typedef struct texture_entry_s {
  char name[256]; /* filename without path */
  char path[512]; /* full relative path    */
  Color *pixels;
  int w, h;
} texture_entry;

bool load_objs_files(world *world);
