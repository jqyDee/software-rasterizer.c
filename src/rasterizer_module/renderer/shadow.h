#pragma once

#include "../math/vec.h"
#include "../world.h"
#include "renderer.h"

// rough shadow mapping — GPU/Phong lighting path only, single shadow-casting
// light (world->lights[0]). See docs/math.typ "Shadow Mapping" for the math.

void compute_shadow_light_basis(world *world);
float sample_shadow(const renderer *renderer, vec3f world_pos);
void world_to_light_screen(const renderer *renderer, vec3f world_pos,
                          float *sx, float *sy, float *closeness);

void draw_shadow_tiles_parallel(renderer *r, const int num_tiles,
                                const int tile_count_buf[MAX_TILES],
                                const int tile_start_buf[MAX_TILES],
                                const int bin_buf[MAX_BIN_ENTRIES],
                                const screen_tri_t tris[MAX_SCREEN_TRIS],
                                const int tiles_x);

void draw_shadow_map(world *world);
