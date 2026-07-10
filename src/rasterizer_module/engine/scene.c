#include "scene.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void ensure_parent_dir(const char *path) {
  const char *slash = strrchr(path, '/');
  if (!slash)
    return;
  char dir[512];
  size_t len = (size_t)(slash - path);
  if (len >= sizeof(dir))
    return;
  memcpy(dir, path, len);
  dir[len] = '\0';
  mkdir(dir, 0755);
}

/* ── parse helpers ─────────────────────────────────────────── */

static const char *skip_ws(const char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  return p;
}

/* Find "key": within [start, end) and return pointer to value start. */
static const char *find_key(const char *start, const char *end,
                            const char *key) {
  char needle[128];
  int nlen = snprintf(needle, sizeof(needle), "\"%s\"", key);
  for (const char *p = start; p + nlen <= end; p++) {
    if (memcmp(p, needle, (size_t)nlen) == 0) {
      const char *q = skip_ws(p + nlen);
      if (q < end && *q == ':') {
        q = skip_ws(q + 1);
        return q < end ? q : NULL;
      }
    }
  }
  return NULL;
}

/* Parse "quoted string". Returns pointer past closing quote. */
static const char *parse_str(const char *p, const char *end, char *out,
                             size_t n) {
  if (p >= end || *p != '"')
    return p;
  p++;
  size_t i = 0;
  while (p < end && *p != '"' && i < n - 1)
    out[i++] = *p++;
  out[i] = '\0';
  if (p < end && *p == '"')
    p++;
  return p;
}

/* Parse [x, y, z]. Returns pointer past ']'. */
static const char *parse_vec3(const char *p, const char *end, float *x,
                              float *y, float *z) {
  if (p >= end || *p != '[')
    return p;
  p++;
  char *ep;
  *x = strtof(p, &ep);
  p = ep < end ? ep : end;
  p = skip_ws(p);
  if (p < end && *p == ',')
    p++;
  *y = strtof(p, &ep);
  p = ep < end ? ep : end;
  p = skip_ws(p);
  if (p < end && *p == ',')
    p++;
  *z = strtof(p, &ep);
  p = ep < end ? ep : end;
  p = skip_ws(p);
  if (p < end && *p == ']')
    p++;
  return p;
}

static float parse_float(const char *p) { return strtof(p, NULL); }

static int find_mesh_by_name(const world *world, const char *name) {
  for (int i = 0; i < (int)world->mesh_data_count; i++)
    if (strcmp(world->mesh_data[i].name, name) == 0)
      return i;
  return -1;
}

/* ── public API ─────────────────────────────────────────────── */

bool scene_save(const world *world, const char *path) {
  ensure_parent_dir(path);
  FILE *f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "scene_save: cannot open %s\n", path);
    return false;
  }

  fprintf(f, "{\n");
  fprintf(f,
          "  \"cam\": {\"pos\": [%.6f, %.6f, %.6f], "
          "\"pitch\": %.6f, \"yaw\": %.6f, \"fov\": %.6f},\n",
          (double)world->cam->pos.x, (double)world->cam->pos.y,
          (double)world->cam->pos.z, (double)world->cam->pitch,
          (double)world->cam->yaw, (double)world->cam->fov);
  fprintf(f, "  \"instances\": [\n");

  for (size_t i = 0; i < world->instance_count; i++) {
    const mesh_instance *inst = &world->instances[i];
    const char *mname = world->mesh_data[inst->mesh_idx].name;
    fprintf(f, "    {\n");
    fprintf(f, "      \"mesh\": \"%s\",\n", mname);
    fprintf(f, "      \"pos\": [%.6f, %.6f, %.6f],\n", (double)inst->pos.x,
            (double)inst->pos.y, (double)inst->pos.z);
    fprintf(f, "      \"rotation\": [%.6f, %.6f, %.6f],\n",
            (double)inst->rotation.x, (double)inst->rotation.y,
            (double)inst->rotation.z);
    fprintf(f, "      \"scale\": [%.6f, %.6f, %.6f],\n", (double)inst->scale.x,
            (double)inst->scale.y, (double)inst->scale.z);
    fprintf(f, "      \"tex_path\": \"%s\"\n", inst->tex_path);
    fprintf(f, "    }%s\n", i + 1 < world->instance_count ? "," : "");
  }

  fprintf(f, "  ]\n}\n");
  fclose(f);
  return true;
}

bool scene_load(world *world, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "scene_load: cannot open %s\n", path);
    return false;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc((size_t)fsize + 1);
  if (!buf) {
    fclose(f);
    return false;
  }
  fread(buf, 1, (size_t)fsize, f);
  buf[fsize] = '\0';
  fclose(f);

  const char *bs = buf;
  const char *be = buf + fsize;

  /* cam */
  const char *cam_v = find_key(bs, be, "cam");
  if (cam_v) {
    /* find end of cam object */
    int depth = 0;
    const char *cam_e = cam_v;
    while (cam_e < be) {
      if (*cam_e == '{')
        depth++;
      else if (*cam_e == '}') {
        if (--depth == 0) {
          cam_e++;
          break;
        }
      }
      cam_e++;
    }
    const char *v;
    v = find_key(cam_v, cam_e, "pos");
    if (v)
      parse_vec3(v, cam_e, &world->cam->pos.x, &world->cam->pos.y,
                 &world->cam->pos.z);
    v = find_key(cam_v, cam_e, "pitch");
    if (v)
      world->cam->pitch = parse_float(v);
    v = find_key(cam_v, cam_e, "yaw");
    if (v)
      world->cam->yaw = parse_float(v);
    v = find_key(cam_v, cam_e, "fov");
    if (v) {
      world->cam->fov = parse_float(v);
      world->cam->focal_length =
          1.0f / tanf(world->cam->fov * (3.14159265f / 180.0f) / 2.0f);
    }
  }

  /* instances */
  world->instance_count = 0;
  world->selected_instance = -1;

  const char *arr_v = find_key(bs, be, "instances");
  if (!arr_v || *arr_v != '[') {
    free(buf);
    return true;
  }
  const char *p = arr_v + 1;

  while (p < be) {
    p = skip_ws(p);
    if (*p == ']')
      break;
    if (*p != '{') {
      p++;
      continue;
    }

    /* locate closing '}' for this object */
    int depth = 0;
    const char *obj_s = p, *obj_e = p;
    while (obj_e < be) {
      if (*obj_e == '{')
        depth++;
      else if (*obj_e == '}') {
        if (--depth == 0) {
          obj_e++;
          break;
        }
      }
      obj_e++;
    }

    char mname[256] = "";
    char tex_path[256] = "";
    vec3f pos = {0, 0, 0};
    vec3f rotation = {0, 0, 0};
    vec3f scale = {1, 1, 1};
    const char *v;

    v = find_key(obj_s, obj_e, "mesh");
    if (v && *v == '"')
      parse_str(v, obj_e, mname, sizeof(mname));

    v = find_key(obj_s, obj_e, "tex_path");
    if (v && *v == '"')
      parse_str(v, obj_e, tex_path, sizeof(tex_path));

    v = find_key(obj_s, obj_e, "pos");
    if (v)
      parse_vec3(v, obj_e, &pos.x, &pos.y, &pos.z);

    v = find_key(obj_s, obj_e, "rotation");
    if (v)
      parse_vec3(v, obj_e, &rotation.x, &rotation.y, &rotation.z);

    v = find_key(obj_s, obj_e, "scale");
    if (v)
      parse_vec3(v, obj_e, &scale.x, &scale.y, &scale.z);

    int mesh_idx = find_mesh_by_name(world, mname);
    if (mesh_idx >= 0) {
      if (world->instance_count >= world->instance_capacity) {
        size_t nc = world->instance_capacity ? world->instance_capacity * 2 : 8;
        mesh_instance *ni =
            realloc(world->instances, nc * sizeof(mesh_instance));
        if (!ni)
          break;
        world->instances = ni;
        world->instance_capacity = nc;
      }
      mesh_instance *inst = &world->instances[world->instance_count++];
      *inst = (mesh_instance){
          .mesh_idx = mesh_idx,
          .pos = pos,
          .rotation = rotation,
          .scale = scale,
          .tex_pixels = NULL,
          .tex_w = 0,
          .tex_h = 0,
      };
      if (tex_path[0] != '\0')
        snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", tex_path);
      else
        snprintf(inst->tex_path, sizeof(inst->tex_path), "%s",
                 world->mesh_data[mesh_idx].auto_tex_path);
    } else {
      fprintf(stderr, "scene_load: mesh '%s' not found, skipping\n", mname);
    }

    p = obj_e;
    p = skip_ws(p);
    if (*p == ',')
      p++;
  }

  free(buf);
  fprintf(stderr, "INFO: scene_load: loaded %s (%zu instances)\n", path,
          world->instance_count);
  return true;
}
