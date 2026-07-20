#include "draw.h"
#include "../testing.h"

#include <math.h>
#include <omp.h>
#include <renderer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "../math/color.h"
#include "../math/vec.h"
#include "../world.h"
#include "buffers.h"
#include "tiling.h"

// Initializes all incremental edge-function data for a Pineda-style rasterizer.
// Output is winding-normalized: "inside pixel" always means all three e* >= 0.
INTERNAL_INLINE void setup_edge_walk(vec3f p0, vec3f p1, vec3f p2, float seed_x,
                                     float seed_y, edge_walk_t *ew) {
  // signed area — positive = CCW winding in screen space (Y-down)
  double signed_area =
      (double)(p2.x - p0.x) * (double)(p1.y - p0.y) - (double)(p2.y - p0.y) * (double)(p1.x - p0.x);
  ew->inv_signed_area = 1.0 / signed_area;

  // dw/dx (e*_dx) and dw/dy (e*_dy) for each edge — constant across triangle,
  // so the inner loop needs only one add per edge per step, not a full cross
  // product
  ew->e0_dx = (double)p2.y - (double)p1.y;
  ew->e0_dy = (double)p1.x - (double)p2.x;
  ew->e1_dx = (double)p0.y - (double)p2.y;
  ew->e1_dy = (double)p2.x - (double)p0.x;
  ew->e2_dx = (double)p1.y - (double)p0.y;
  ew->e2_dy = (double)p0.x - (double)p1.x;

  // evaluate all three edge functions at the first pixel center (+0.5 = center,
  // not corner)
  ew->e0_seed =
      ((double)seed_x - (double)p1.x) * ((double)p2.y - (double)p1.y) - ((double)seed_y - (double)p1.y) * ((double)p2.x - (double)p1.x);
  ew->e1_seed =
      ((double)seed_x - (double)p2.x) * ((double)p0.y - (double)p2.y) - ((double)seed_y - (double)p2.y) * ((double)p0.x - (double)p2.x);
  ew->e2_seed =
      ((double)seed_x - (double)p0.x) * ((double)p1.y - (double)p0.y) - ((double)seed_y - (double)p0.y) * ((double)p1.x - (double)p0.x);

  // CW-wound triangles produce signed_area < 0, making all weights negative
  // inside. Negate steps, seeds and inv_signed_area so "inside = all e >= 0"
  // works for either winding.
  if (signed_area < 0.0) {
    ew->e0_dx = -ew->e0_dx;
    ew->e0_dy = -ew->e0_dy;
    ew->e0_seed = -ew->e0_seed;
    ew->e1_dx = -ew->e1_dx;
    ew->e1_dy = -ew->e1_dy;
    ew->e1_seed = -ew->e1_seed;
    ew->e2_dx = -ew->e2_dx;
    ew->e2_dy = -ew->e2_dy;
    ew->e2_seed = -ew->e2_seed;
    ew->inv_signed_area = -ew->inv_signed_area;
  }
}

// Narrows [*lo, *hi] to the tightest subrange where edge function
// e(col) = e_row + (col - rowStartX) * e_dx could satisfy e(col) >= edge_bias.
// Only ever narrows, never widens. Sets *row_excluded if this edge rules out
// the entire row.
INTERNAL_INLINE void narrow_edge_bound(double e_row, double e_dx,
                                     double edge_bias, int rowStartX, int *lo,
                                     int *hi, bool *row_excluded) {
  const double kSlopeEps = 1e-9;
  if (e_dx > kSlopeEps) {
    double col_bound = (double)rowStartX + (edge_bias - e_row) / e_dx;
    int new_lo = (int)floor(col_bound);
    if (new_lo > *lo)
      *lo = new_lo;
  } else if (e_dx < -kSlopeEps) {
    double col_bound = (double)rowStartX + (edge_bias - e_row) / e_dx;
    int new_hi = (int)ceil(col_bound);
    if (new_hi < *hi)
      *hi = new_hi;
  } else if (e_row < edge_bias) {
    *row_excluded = true;
  }
}

// Returns the color for one pixel given its barycentric coords and depth.
// Perspective-correct texture lookup if st->tex is set, otherwise flat_color.
INTERNAL_INLINE Color fetch_pixel_albedo(const screen_tri_t *st, float b0,
                                         float b1, float b2, float inv_depth,
                                         bool skip_texture) {
  Color base;
  if (st->tex && !skip_texture) {
    const vec2f uv0 = st->uv[0], uv1 = st->uv[1], uv2 = st->uv[2];
    const float inv_z0 = st->inv_z[0], inv_z1 = st->inv_z[1],
                inv_z2 = st->inv_z[2];
    float z = 1.0f / inv_depth; // = actual z at this pixel
    // u/z is linear in screen space; interpolate u*inv_z barycentrically,
    // then multiply by z to recover perspective-correct u
    float u =
        (b0 * uv0.x * inv_z0 + b1 * uv1.x * inv_z1 + b2 * uv2.x * inv_z2) * z;
    float v =
        (b0 * uv0.y * inv_z0 + b1 * uv1.y * inv_z1 + b2 * uv2.y * inv_z2) * z;
    u -= floorf(u); // wrap to [0,1] for tiling
    v -= floorf(v);
    v = 1.0f - v; // UV V=0 is bottom; image row 0 is top: flip to match
    int tex_x = (int)(u * (float)st->tex_w);
    if (tex_x >= st->tex_w) {
      tex_x = st->tex_w - 1;
    }
    int tex_y = (int)(v * (float)st->tex_h);
    if (tex_y >= st->tex_h) {
      tex_y = st->tex_h - 1;
    }

    base = st->tex[tex_y * st->tex_w + tex_x];
  } else {
    base = st->flat_color;
  }

  return base;
}

void draw_triangle_pixels_tiled(depthbuffer *depthbuffer,
                                framebuffer *framebuffer, idbuffer *idbuffer,
                                albedobuffer *albedobuffer,
                                normalbuffer *normalbuffer, int screen_width,
                                const screen_tri_t *st, int clip_x0,
                                int clip_y0, int clip_x1, int clip_y1,
                                long *iter, long *edge_pass, long *depth_pass,
                                bool skip_texture, bool skip_normalbuffer,
                                bool skip_albedobuffer, bool skip_framebuffer) {
  // clip triangle bbox to tile bounds: triangle may span multiple tiles
  int startX = st->bx0 > clip_x0 ? st->bx0 : clip_x0;
  int startY = st->by0 > clip_y0 ? st->by0 : clip_y0;
  int endX = st->bx1 < clip_x1 ? st->bx1 : clip_x1;
  int endY = st->by1 < clip_y1 ? st->by1 : clip_y1;
  if (startX > endX || startY > endY)
    return;

  // unpack into locals: used in the hot loop for depth computation and biased
  // cap
  const vec3f p0 = st->v[0], p1 = st->v[1], p2 = st->v[2];
  const float inv_z0 = st->inv_z[0], inv_z1 = st->inv_z[1],
              inv_z2 = st->inv_z[2];
  int instance_id = st->instance_id;

  edge_walk_t ew;
  setup_edge_walk(p0, p1, p2, startX + 0.5f, startY + 0.5f, &ew);

  // fp_bias is a positive seam-fill distance; negating it makes the inside test
  // e >= edge_bias less strict than e >= 0, so fringe pixels just outside
  // shared triangle edges are included and fill the gap instead of leaving dark
  // seams
  double edge_bias = -(double)st->fp_bias;
  for (int row = startY; row <= endY; row++) {
    int dy = row - startY;
    double e0 = ew.e0_seed + (double)dy * ew.e0_dy;
    double e1 = ew.e1_seed + (double)dy * ew.e1_dy;
    double e2 = ew.e2_seed + (double)dy * ew.e2_dy;
    int row_offset = row * screen_width;

    // scanline x-range tightening: most of [startX, endX] is outside the
    // triangle (bbox/tile binning is coarse), so bound the row analytically
    // before brute-testing every column. Padded by 1px and still gated by
    // the same exact per-pixel test below, so this can only skip pixels
    // that are provably always-false, never one the old scan would include.
    int rowLo = startX, rowHi = endX;
    bool row_excluded = false;
    narrow_edge_bound(e0, ew.e0_dx, edge_bias, startX, &rowLo, &rowHi,
                      &row_excluded);
    narrow_edge_bound(e1, ew.e1_dx, edge_bias, startX, &rowLo, &rowHi,
                      &row_excluded);
    narrow_edge_bound(e2, ew.e2_dx, edge_bias, startX, &rowLo, &rowHi,
                      &row_excluded);

    if (row_excluded || rowLo > rowHi) {
      continue;
    }

    rowLo -= 1;
    rowHi += 1;
    if (rowLo < startX)
      rowLo = startX;
    if (rowHi > endX)
      rowHi = endX;

    if (rowLo > startX) {
      double advance = (double)(rowLo - startX);
      e0 += advance * ew.e0_dx;
      e1 += advance * ew.e1_dx;
      e2 += advance * ew.e2_dx;
    }

    for (int col = rowLo; col <= rowHi; col++) {
      (*iter)++;
      if ((e0 >= edge_bias) && (e1 >= edge_bias) && (e2 >= edge_bias)) {
        (*edge_pass)++;
        // clamp biased-outside weights to 0 before normalizing: fringe pixels
        // that passed via bias must still produce valid (non-negative)
        // barycentric coords
        float b0 = (float)fmax(e0, 0.0) * (float)ew.inv_signed_area;
        float b1 = (float)fmax(e1, 0.0) * (float)ew.inv_signed_area;
        float b2 = (float)fmax(e2, 0.0) * (float)ew.inv_signed_area;
        // 1/z is linear in screen space, so interpolating inv_z barycentrically
        // gives perspective-correct depth without a per-pixel divide
        float inv_depth = b0 * inv_z0 + b1 * inv_z1 + b2 * inv_z2;
        // Biased (geometrically-outside) pixels: cap inv_depth to the farthest
        // vertex. Prevents near-plane-clipped vertex (inv_z $approx$ 10) from
        // inflating inv_depth and incorrectly overwriting closer geometry.
        if (e0 < 0.f || e1 < 0.f || e2 < 0.f) {
          float iz_farthest = fminf(fminf(inv_z0, inv_z1), inv_z2);
          if (inv_depth > iz_farthest)
            inv_depth = iz_farthest;
        }
        int pixel_idx = row_offset + col;
        if (inv_depth > depthbuffer[pixel_idx]) {
          (*depth_pass)++;
          depthbuffer[pixel_idx] = inv_depth;

          if (idbuffer) {
            idbuffer[pixel_idx] = instance_id;
          }

          if (framebuffer && !skip_framebuffer) {
            Color albedo =
                fetch_pixel_albedo(st, b0, b1, b2, inv_depth, skip_texture);
            framebuffer[pixel_idx] = color_scale(albedo, st->light_rgb);
            if (albedobuffer && !skip_albedobuffer) {
              float z = 1.0f / inv_depth;
              float shade = ((b0 * st->shadow[0] * inv_z0) +
                             (b1 * st->shadow[1] * inv_z1) +
                             (b2 * st->shadow[2] * inv_z2)) * z;
              albedobuffer[pixel_idx] = (Color){
                  (unsigned char)(albedo.r * shade),
                  (unsigned char)(albedo.g * shade),
                  (unsigned char)(albedo.b * shade),
                  albedo.a,
              };
            }
          }

          if (normalbuffer && !skip_normalbuffer) {
            // interpolate same as for inv_depth. no perspective correcting
            // needed, as normals are not texture coordinates
            vec3f n = (vec3f){
                b0 * st->normals[0].x + b1 * st->normals[1].x +
                    b2 * st->normals[2].x,
                b0 * st->normals[0].y + b1 * st->normals[1].y +
                    b2 * st->normals[2].y,
                b0 * st->normals[0].z + b1 * st->normals[1].z +
                    b2 * st->normals[2].z,
            };
            n = vec_normalize(n); // interpolation shrinks magnitude
            normalbuffer[pixel_idx] = (Color){
                (unsigned char)((n.x * 0.5f + 0.5f) * 255.0f),
                (unsigned char)((n.y * 0.5f + 0.5f) * 255.0f),
                (unsigned char)((n.z * 0.5f + 0.5f) * 255.0f),
                255,
            };
          }
        }
      }
      e0 += ew.e0_dx;
      e1 += ew.e1_dx;
      e2 += ew.e2_dx;
    }
  }
}

void draw_tiles_parallel(world *world, const viewport *vp, const int num_tiles,
                         const int tile_count_buf[MAX_TILES],
                         const int tile_start_buf[MAX_TILES],
                         const int bin_buf[MAX_BIN_ENTRIES],
                         const screen_tri_t screen_triangles[MAX_SCREEN_TRIS],
                         const int tiles_x, long *iter, long *edge_pass,
                         long *depth_pass) {
  depthbuffer *depthbuffer = world->renderer->depthbuffer;
  framebuffer *framebuffer = world->renderer->framebuffer;
  idbuffer *idbuffer =
      world->settings.show_debug_gui ? world->renderer->idbuffer : NULL;
  albedobuffer *albedobuffer = world->renderer->albedobuffer;
  normalbuffer *normalbuffer = world->renderer->normalbuffer;
  
  bool skip_texture = world->settings.debug_skip_texture;
  bool skip_normalbuffer = world->settings.debug_skip_normalbuffer;
  bool skip_albedobuffer = world->settings.debug_skip_albedobuffer;
  bool skip_framebuffer = world->settings.debug_skip_framebuffer;

  long local_iter = 0, local_edge = 0, local_depth = 0;

#pragma omp parallel for schedule(dynamic) reduction(+:local_iter, local_edge, local_depth) if (vp->h > world->settings.parallel_cutoff_rows)
  for (int tile_index = 0; tile_index < num_tiles; tile_index++) {
    // skip if tile buffer is empty
    if (tile_count_buf[tile_index] == 0)
      continue;

    // convert tile coordinates back to actual coordinates
    int x0, x1, y0, y1;
    tile_coordinates_to_triangle_coordinates(tile_index, tiles_x, vp->x, vp->y,
                                             vp->w, vp->h, &x0, &x1, &y0, &y1);
    int end = tile_start_buf[tile_index] + tile_count_buf[tile_index];

    // draw the actual triangles in the bin
    for (int bin_id = tile_start_buf[tile_index]; bin_id < end; bin_id++)
      draw_triangle_pixels_tiled(
          depthbuffer, framebuffer, idbuffer, albedobuffer, normalbuffer,
          world->renderer->screen_width, &screen_triangles[bin_buf[bin_id]], x0,
          y0, x1, y1, &local_iter, &local_edge, &local_depth, skip_texture,
          skip_normalbuffer, skip_albedobuffer, skip_framebuffer);
  }

  *iter += local_iter;
  *edge_pass += local_edge;
  *depth_pass += local_depth;
}

