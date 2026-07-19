#include "track.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../engine/instance.h"
#include "../world.h"

#define TRACK_MESH_NAME "__track__"
#define TRACK_MIN_WIDTH 1.5f
#define TRACK_MAX_WIDTH 30.0f
#define TRACK_TEXTURE "assets/textures/road.jpg"
#define TEX_TILE_LENGTH 8.0f /* world units of road per texture repeat */

void track_default(track *t) {
  /* 8-point oval, roughly the size of the default floor plane */
  const float rx = 35.0f, rz = 22.0f;
  t->point_count = 8;
  for (size_t i = 0; i < t->point_count; i++) {
    float a = (float)i / (float)t->point_count * 2.0f * (float)M_PI;
    t->points[i].pos = (vec2f){rx * cosf(a), rz * sinf(a)};
    t->points[i].width = 6.0f;
  }
}

bool track_save(const track *t, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "track_save: cannot open %s\n", path);
    return false;
  }
  fprintf(f, "{\n  \"points\": [\n");
  for (size_t i = 0; i < t->point_count; i++) {
    fprintf(f, "    { \"pos\": [%.3f, %.3f], \"width\": %.3f }%s\n",
            t->points[i].pos.x, t->points[i].pos.y, t->points[i].width,
            i + 1 < t->point_count ? "," : "");
  }
  fprintf(f, "  ]\n}\n");
  fclose(f);
  printf("INFO: track saved to %s (%zu points)\n", path, t->point_count);
  return true;
}

bool track_load(track *t, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return false;
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

  /* the only numbers in the file are x, z, width triplets in order —
   * scan every float and group by three */
  float vals[MAX_TRACK_POINTS * 3];
  size_t n = 0;
  const char *p = buf;
  while (*p && n < MAX_TRACK_POINTS * 3) {
    if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '.') {
      char *end;
      float v = strtof(p, &end);
      if (end != p) {
        vals[n++] = v;
        p = end;
        continue;
      }
    }
    p++;
  }
  free(buf);

  if (n < 9 || n % 3 != 0) { /* need at least 3 points */
    fprintf(stderr, "track_load: %s malformed (%zu values)\n", path, n);
    return false;
  }
  t->point_count = n / 3;
  for (size_t i = 0; i < t->point_count; i++) {
    t->points[i].pos = (vec2f){vals[i * 3], vals[i * 3 + 1]};
    t->points[i].width = vals[i * 3 + 2];
  }
  printf("INFO: track loaded from %s (%zu points)\n", path, t->point_count);
  return true;
}

static float catmull_rom(float p0, float p1, float p2, float p3, float u) {
  float u2 = u * u, u3 = u2 * u;
  return 0.5f * ((2.0f * p1) + (-p0 + p2) * u +
                 (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u2 +
                 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * u3);
}

vec2f track_sample_pos(const track *t, size_t seg, float u) {
  size_t n = t->point_count;
  const track_point *a = &t->points[(seg + n - 1) % n];
  const track_point *b = &t->points[seg % n];
  const track_point *c = &t->points[(seg + 1) % n];
  const track_point *d = &t->points[(seg + 2) % n];
  return (vec2f){catmull_rom(a->pos.x, b->pos.x, c->pos.x, d->pos.x, u),
                 catmull_rom(a->pos.y, b->pos.y, c->pos.y, d->pos.y, u)};
}

float track_sample_width(const track *t, size_t seg, float u) {
  size_t n = t->point_count;
  float w = catmull_rom(t->points[(seg + n - 1) % n].width,
                        t->points[seg % n].width,
                        t->points[(seg + 1) % n].width,
                        t->points[(seg + 2) % n].width, u);
  if (w < TRACK_MIN_WIDTH)
    w = TRACK_MIN_WIDTH;
  if (w > TRACK_MAX_WIDTH)
    w = TRACK_MAX_WIDTH;
  return w;
}

void track_sample_edges(const track *t, size_t seg, float u, vec2f *left,
                        vec2f *right) {
  vec2f c = track_sample_pos(t, seg, u);
  /* tangent via small forward step (wraps naturally through sampling) */
  float u2 = u + 0.01f;
  size_t seg2 = seg;
  if (u2 > 1.0f) {
    u2 -= 1.0f;
    seg2 = (seg + 1) % t->point_count;
  }
  vec2f c2 = track_sample_pos(t, seg2, u2);
  float tx = c2.x - c.x, tz = c2.y - c.y;
  float len = sqrtf(tx * tx + tz * tz);
  if (len < 0.00001f) {
    tx = 1.0f;
    tz = 0.0f;
    len = 1.0f;
  }
  tx /= len;
  tz /= len;
  /* left perpendicular of (tx,tz) in the XZ plane */
  float px = -tz, pz = tx;
  float w = track_sample_width(t, seg, u);
  *left = (vec2f){c.x + px * w, c.y + pz * w};
  *right = (vec2f){c.x - px * w, c.y - pz * w};
}

bool track_is_on_road(const track *t, float x, float z) {
  if (t->point_count < 3)
    return true; /* no track = everything is road */

  /* nearest distance to the sampled centerline polyline, compared against
   * the road half-width at the closest sample */
  size_t samples = t->point_count * TRACK_SAMPLES_PER_SEG;
  float best_d_sq = 1e30f;
  float best_w = 0.0f;
  for (size_t k = 0; k < samples; k++) {
    size_t seg = k / TRACK_SAMPLES_PER_SEG;
    float u0 = (float)(k % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;
    size_t k1 = (k + 1) % samples;
    size_t seg1 = k1 / TRACK_SAMPLES_PER_SEG;
    float u1 = (float)(k1 % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;

    vec2f a = track_sample_pos(t, seg, u0);
    vec2f b = track_sample_pos(t, seg1, u1);

    /* project (x,z) onto segment a-b */
    float abx = b.x - a.x, abz = b.y - a.y;
    float len_sq = abx * abx + abz * abz;
    float uu = 0.0f;
    if (len_sq > 0.00001f) {
      uu = ((x - a.x) * abx + (z - a.y) * abz) / len_sq;
      if (uu < 0.0f)
        uu = 0.0f;
      if (uu > 1.0f)
        uu = 1.0f;
    }
    float cx = a.x + abx * uu, cz = a.y + abz * uu;
    float dx = x - cx, dz = z - cz;
    float d_sq = dx * dx + dz * dz;
    if (d_sq < best_d_sq) {
      best_d_sq = d_sq;
      best_w = track_sample_width(t, seg, u0);
    }
  }
  return best_d_sq <= best_w * best_w;
}

float track_progress_at(const track *t, float x, float z) {
  if (t->point_count < 3)
    return 0.0f;

  /* nearest point on the sampled centerline polyline; position along the
   * loop approximated by (sample index + in-segment fraction) / samples —
   * not arc-length exact, but strictly monotonic along the track, which
   * is all laps/checkpoints need */
  size_t samples = t->point_count * TRACK_SAMPLES_PER_SEG;
  float best_d_sq = 1e30f;
  float best_pos = 0.0f;
  for (size_t k = 0; k < samples; k++) {
    size_t seg = k / TRACK_SAMPLES_PER_SEG;
    float u0 = (float)(k % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;
    size_t k1 = (k + 1) % samples;
    size_t seg1 = k1 / TRACK_SAMPLES_PER_SEG;
    float u1 = (float)(k1 % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;

    vec2f a = track_sample_pos(t, seg, u0);
    vec2f b = track_sample_pos(t, seg1, u1);

    float abx = b.x - a.x, abz = b.y - a.y;
    float len_sq = abx * abx + abz * abz;
    float uu = 0.0f;
    if (len_sq > 0.00001f) {
      uu = ((x - a.x) * abx + (z - a.y) * abz) / len_sq;
      if (uu < 0.0f)
        uu = 0.0f;
      if (uu > 1.0f)
        uu = 1.0f;
    }
    float cx = a.x + abx * uu, cz = a.y + abz * uu;
    float dx = x - cx, dz = z - cz;
    float d_sq = dx * dx + dz * dz;
    if (d_sq < best_d_sq) {
      best_d_sq = d_sq;
      best_pos = ((float)k + uu) / (float)samples;
    }
  }
  return best_pos;
}

void track_build_mesh(world *world) {
  track *t = &world->track_data;
  if (t->point_count < 3)
    return;

  /* find or append the dedicated track mesh slot */
  int mesh_idx = -1;
  for (size_t i = 0; i < world->mesh_data_count; i++) {
    if (strcmp(world->mesh_data[i].name, TRACK_MESH_NAME) == 0) {
      mesh_idx = (int)i;
      break;
    }
  }
  if (mesh_idx < 0) {
    mesh *nm = realloc(world->mesh_data,
                       (world->mesh_data_count + 1) * sizeof(mesh));
    if (!nm)
      return;
    world->mesh_data = nm;
    mesh_idx = (int)world->mesh_data_count++;
    memset(&world->mesh_data[mesh_idx], 0, sizeof(mesh));
    snprintf(world->mesh_data[mesh_idx].name,
             sizeof(world->mesh_data[mesh_idx].name), "%s", TRACK_MESH_NAME);
  }
  mesh *m = &world->mesh_data[mesh_idx];

  size_t samples = t->point_count * TRACK_SAMPLES_PER_SEG;
  size_t vcount = samples * 2 * 3; /* 2 triangles per quad, 3 verts each */
  vec3f *verts = malloc(vcount * sizeof(vec3f));
  vec3f *normals = malloc(vcount * sizeof(vec3f));
  vec2f *uvs = malloc(vcount * sizeof(vec2f));
  vec2f *lefts = malloc(samples * sizeof(vec2f));
  vec2f *rights = malloc(samples * sizeof(vec2f));
  float *arcs = malloc((samples + 1) * sizeof(float));
  if (!verts || !normals || !uvs || !lefts || !rights || !arcs) {
    free(verts);
    free(normals);
    free(uvs);
    free(lefts);
    free(rights);
    free(arcs);
    return;
  }

  /* edges + cumulative centerline arc length (arcs[samples] = full loop) */
  arcs[0] = 0.0f;
  vec2f prev_c = track_sample_pos(t, 0, 0.0f);
  for (size_t k = 0; k < samples; k++) {
    size_t seg = k / TRACK_SAMPLES_PER_SEG;
    float u = (float)(k % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;
    track_sample_edges(t, seg, u, &lefts[k], &rights[k]);

    size_t k1 = (k + 1) % samples;
    size_t seg1 = k1 / TRACK_SAMPLES_PER_SEG;
    float u1 = (float)(k1 % TRACK_SAMPLES_PER_SEG) / TRACK_SAMPLES_PER_SEG;
    vec2f c = track_sample_pos(t, seg1, u1);
    float dx = c.x - prev_c.x, dz = c.y - prev_c.y;
    arcs[k + 1] = arcs[k] + sqrtf(dx * dx + dz * dz);
    prev_c = c;
  }

  /* texture V along the road in whole tiles: round the tile count so the
   * loop seam lands exactly on a repeat boundary (no visible seam) */
  float total_len = arcs[samples] > 0.001f ? arcs[samples] : 1.0f;
  float tiles = roundf(total_len / TEX_TILE_LENGTH);
  if (tiles < 1.0f)
    tiles = 1.0f;
  float v_scale = tiles / total_len;

  /* slight lift above the y=0 ground plane so the road wins the depth test */
  const float road_y = 0.02f;
  size_t vi = 0;
  for (size_t k = 0; k < samples; k++) {
    size_t k1 = (k + 1) % samples;
    vec3f l0 = {lefts[k].x, road_y, lefts[k].y};
    vec3f r0 = {rights[k].x, road_y, rights[k].y};
    vec3f l1 = {lefts[k1].x, road_y, lefts[k1].y};
    vec3f r1 = {rights[k1].x, road_y, rights[k1].y};
    /* U spans the road left→right, V runs along it; arcs[k+1] (not
     * arcs[k1]) so the wrap quad ends at V = tiles, which the sampler
     * wraps back to 0 seamlessly */
    float v0 = arcs[k] * v_scale;
    float v1 = arcs[k + 1] * v_scale;
    verts[vi] = l0; uvs[vi++] = (vec2f){0.0f, v0};
    verts[vi] = l1; uvs[vi++] = (vec2f){0.0f, v1};
    verts[vi] = r0; uvs[vi++] = (vec2f){1.0f, v0};
    verts[vi] = r0; uvs[vi++] = (vec2f){1.0f, v0};
    verts[vi] = l1; uvs[vi++] = (vec2f){0.0f, v1};
    verts[vi] = r1; uvs[vi++] = (vec2f){1.0f, v1};
  }
  for (size_t i = 0; i < vcount; i++)
    normals[i] = (vec3f){0.0f, 1.0f, 0.0f};

  free(lefts);
  free(rights);
  free(arcs);

  free(m->vertices);
  free(m->uvs);
  free(m->normals);
  m->vertices = verts;
  m->normals = normals;
  m->uvs = uvs;
  m->vertex_count = vcount;

  m->aabb_min = m->aabb_max = verts[0];
  for (size_t i = 1; i < vcount; i++) {
    vec3f v = verts[i];
    if (v.x < m->aabb_min.x) m->aabb_min.x = v.x;
    if (v.y < m->aabb_min.y) m->aabb_min.y = v.y;
    if (v.z < m->aabb_min.z) m->aabb_min.z = v.z;
    if (v.x > m->aabb_max.x) m->aabb_max.x = v.x;
    if (v.y > m->aabb_max.y) m->aabb_max.y = v.y;
    if (v.z > m->aabb_max.z) m->aabb_max.z = v.z;
  }

  /* ensure exactly one instance shows the track */
  int inst_idx = -1;
  for (size_t i = 0; i < world->instance_count; i++) {
    if (world->instances[i].mesh_idx == mesh_idx) {
      inst_idx = (int)i;
      break;
    }
  }
  if (inst_idx < 0) {
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
    inst_idx = (int)world->instance_count++;
    mesh_instance *inst = &world->instances[inst_idx];
    *inst = (mesh_instance){
        .mesh_idx = mesh_idx,
        .pos = {0, 0, 0},
        .rotation = {0, 0, 0},
        .scale = {1, 1, 1},
        .tex_pixels = NULL,
        .tex_w = 0,
        .tex_h = 0,
    };
    inst->tex_path[0] = '\0';
  }

  /* road texture: adopt the default if none set and the file exists;
   * load only when pixels are missing (init reload pass also covers it) */
  mesh_instance *inst = &world->instances[inst_idx];
  if (inst->tex_path[0] == '\0') {
    FILE *tf = fopen(TRACK_TEXTURE, "rb");
    if (tf) {
      fclose(tf);
      snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", TRACK_TEXTURE);
    }
  }
  if (inst->tex_path[0] != '\0' && !inst->tex_pixels)
    load_instance_texture(world, inst_idx, inst->tex_path);
  printf("INFO: track mesh rebuilt (%zu samples, %zu tris)\n", samples,
         vcount / 3);
}
