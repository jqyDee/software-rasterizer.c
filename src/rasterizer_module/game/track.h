#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../math/vec.h"

typedef struct world_s world;

#define MAX_TRACK_POINTS 64
#define TRACK_SAMPLES_PER_SEG 12
#define TRACK_FILE "assets/tracks/track.json"

typedef struct track_point_s {
  vec2f pos;   /* world coords: .x = x, .y = z (top-down) */
  float width; /* half-width of the road at this point */
} track_point;

/* closed-loop race track: a Catmull-Rom spline runs through all control
 * points, the road is the ribbon of `width` around that centerline */
typedef struct track_s {
  track_point points[MAX_TRACK_POINTS];
  size_t point_count;
} track;

void track_default(track *t); /* starter oval */
bool track_save(const track *t, const char *path);
bool track_load(track *t, const char *path);

/* Catmull-Rom sample on segment seg (points[seg] → points[seg+1]), u in
 * [0,1]. Segments wrap: seg may be any index < point_count. */
vec2f track_sample_pos(const track *t, size_t seg, float u);
float track_sample_width(const track *t, size_t seg, float u);

/* left/right road edge at a sample (perpendicular offset by width) */
void track_sample_edges(const track *t, size_t seg, float u, vec2f *left,
                        vec2f *right);

/* (re)generate the drivable track mesh + its instance from the spline */
void track_build_mesh(world *world);

/* true when world-space (x,z) lies on the road ribbon */
bool track_is_on_road(const track *t, float x, float z);

/* normalized position 0..1 along the track loop for world-space (x,z) —
 * monotonically increasing when driving the intended direction, wraps at
 * the start line. Basis for laps, checkpoints and race positions. */
float track_progress_at(const track *t, float x, float z);
