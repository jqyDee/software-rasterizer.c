#include "mesh.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../world.h"
#include "../engine/dir.h"
#include "../assets/primitives.h"
#include "../math/collision.h"
#include "parser.h"

#ifdef STATIC_RELEASE
#include "embedded_assets.h"
#endif

bool load_objs_files(world *world) {
  /* free old mesh data */
  for (size_t i = 0; i < world->mesh_data_count; i++) {
    free(world->mesh_data[i].vertices);
    free(world->mesh_data[i].uvs);
  }

#ifdef STATIC_RELEASE
  int obj_count = 0;
  for (int i = 0; i < embedded_asset_count; i++) {
    const char *dot = strrchr(embedded_assets[i].path, '.');
    if (dot && strcasecmp(dot, ".obj") == 0)
      obj_count++;
  }
  char **obj_paths = NULL;
#else
  const char *obj_exts[] = {".obj"};
  int obj_count = 0;
  char **obj_paths = scan_dir(ASSETS_OBJ_DIR, obj_exts, 1, &obj_count);
#endif

  size_t total = (size_t)(NUM_PRIMITIVES + obj_count);
  mesh *nd = realloc(world->mesh_data, total * sizeof(mesh));
  if (!nd) {
#ifndef STATIC_RELEASE
    for (int i = 0; i < obj_count; i++)
      free(obj_paths[i]);
    free(obj_paths);
#endif
    return false;
  }
  world->mesh_data = nd;
  world->mesh_data_count = 0;

  /* primitives */
  world->mesh_data[world->mesh_data_count++] = make_cube_mesh();
  world->mesh_data[world->mesh_data_count++] = make_sphere_mesh(20, 20);
  world->mesh_data[world->mesh_data_count++] = make_plane_mesh();
  world->mesh_data[world->mesh_data_count++] = make_cylinder_mesh(16);
  world->mesh_data[world->mesh_data_count++] = make_cone_mesh(16);
  world->mesh_data[world->mesh_data_count++] = make_torus_mesh(24, 16);
  for (size_t pi = 0; pi < NUM_PRIMITIVES; pi++)
    compute_mesh_aabb(&world->mesh_data[pi]);

#ifdef STATIC_RELEASE
  /* obj files from embedded assets */
  for (int i = 0; i < embedded_asset_count; i++) {
    const char *ep = embedded_assets[i].path;
    const char *edot = strrchr(ep, '.');
    if (!edot || strcasecmp(edot, ".obj") != 0)
      continue;
    mesh *m = &world->mesh_data[world->mesh_data_count];
    if (!load_obj_from_memory((const char *)embedded_assets[i].data,
                              embedded_assets[i].size, m)) {
      fprintf(stderr, "load_objs_files: failed to load embedded %s\n", ep);
      continue;
    }
    const char *eslash = strrchr(ep, '/');
    snprintf(m->name, sizeof(m->name), "%s", eslash ? eslash + 1 : ep);
    m->auto_tex_path[0] = '\0';
    const char *enamedot = strrchr(m->name, '.');
    size_t ebase = enamedot ? (size_t)(enamedot - m->name) : strlen(m->name);
    const char *etex_exts[] = {".png", ".jpg", ".jpeg"};
    for (int e = 0; e < 3; e++) {
      char ecand[512];
      snprintf(ecand, sizeof(ecand), "%s/%.*s%s", ASSETS_TEX_DIR, (int)ebase,
               m->name, etex_exts[e]);
      if (find_embedded_asset(ecand)) {
        snprintf(m->auto_tex_path, sizeof(m->auto_tex_path), "%s", ecand);
        break;
      }
    }
    compute_mesh_aabb(m);
    world->mesh_data_count++;
  }
  return true;
#else
  /* obj files from disk */
  for (int i = 0; i < obj_count; i++) {
    mesh *m = &world->mesh_data[world->mesh_data_count];
    if (!load_obj(obj_paths[i], m)) {
      fprintf(stderr, "load_objs_files: failed to load %s, skipping\n",
              obj_paths[i]);
      free(obj_paths[i]);
      continue;
    }
    const char *slash = strrchr(obj_paths[i], '/');
    snprintf(m->name, sizeof(m->name), "%s", slash ? slash + 1 : obj_paths[i]);

    /* look for matching texture in assets/textures/ by basename */
    m->auto_tex_path[0] = '\0';
    const char *dot = strrchr(m->name, '.');
    size_t base_len = dot ? (size_t)(dot - m->name) : strlen(m->name);
    const char *tex_exts[] = {".png", ".jpg", ".jpeg"};
    for (int e = 0; e < 3; e++) {
      char candidate[512];
      snprintf(candidate, sizeof(candidate), "%s/%.*s%s", ASSETS_TEX_DIR,
               (int)base_len, m->name, tex_exts[e]);
      FILE *f = fopen(candidate, "rb");
      if (f) {
        fclose(f);
        snprintf(m->auto_tex_path, sizeof(m->auto_tex_path), "%s", candidate);
        break;
      }
    }

    compute_mesh_aabb(m);
    world->mesh_data_count++;
    free(obj_paths[i]);
  }
  free(obj_paths);
  return true;
#endif
}
