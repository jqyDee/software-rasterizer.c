#include "perf.h"

#include <stdlib.h>
#include <string.h>

#include "../math/rotation.h"
#include "../world.h"
#include "raylib.h"

void perf_record(perf_metric_t *m, float ms) {
  m->avg_ms = m->avg_ms * 0.9f + ms * 0.1f;
  m->samples[m->next] = ms;
  m->next = (m->next + 1) % PERF_HISTORY_SIZE;
  if (m->sample_count < PERF_HISTORY_SIZE)
    m->sample_count++;
}

static int cmp_float_desc(const void *a, const void *b) {
  float fa = *(const float *)a, fb = *(const float *)b;
  return (fa < fb) - (fa > fb); /* descending: worst (highest) first */
}

void perf_recompute_1pct_low(perf_metric_t *m) {
  if (m->sample_count == 0)
    return;
  float sorted[PERF_HISTORY_SIZE];
  memcpy(sorted, m->samples, (size_t)m->sample_count * sizeof(float));
  qsort(sorted, (size_t)m->sample_count, sizeof(float), cmp_float_desc);
  int worst_n = m->sample_count / 100;
  if (worst_n < 1)
    worst_n = 1;
  float sum = 0.0f;
  for (int i = 0; i < worst_n; i++)
    sum += sorted[i];
  m->low_1pct_ms = sum / (float)worst_n;
}

void profile(world *world) {
  const int screen_width = world->renderer->screen_width;
  const int screen_height = world->renderer->screen_height;

#define PROFILE(metric_id, code)                                              \
  do {                                                                        \
    double _t0 = GetTime();                                                   \
    code;                                                                     \
    float _ms = (float)((GetTime() - _t0) * 1000.0);                         \
    perf_record(&world->perf.metrics[metric_id], _ms);                       \
  } while (0)

  PROFILE(PERF_CLEAR, {
    clear_color_buffer(world->renderer->framebuffer, screen_width,
                       screen_height, BLACK);
    clear_depthbuffer(world->renderer->depthbuffer, screen_width,
                      screen_height);
    clear_idbuffer(world->renderer->idbuffer, screen_width, screen_height);
    /* alpha 0 = "no geometry here" sentinel — the phong shader samples the
     * skybox on these pixels; geometry writes always set alpha 255 */
    clear_color_buffer(world->renderer->albedobuffer, screen_width,
                       screen_height, (Color){0, 0, 0, 0});
    clear_color_buffer(world->renderer->normalbuffer, screen_width,
                       screen_height, (Color){128, 128, 255, 255}); // encode(0,0,1)
  });

  PROFILE(PERF_RENDER, render(world));

  if (world->settings.lighting_mode == LIGHTING_GPU_PHONG) {
    PROFILE(PERF_UPLOAD, {
      UpdateTexture(world->renderer->albedo_texture,
                    world->renderer->albedobuffer);
      UpdateTexture(world->renderer->normal_texture,
                    world->renderer->normalbuffer);
    });

    // CPU-side cost of packing light data into shader uniforms — fixed
    // cost, scales only with light_count/viewport_count, independent of
    // screen resolution.
    vec3f light_colors_norm[MAX_LIGHT_SOURCES];
    float light_intensities[MAX_LIGHT_SOURCES];
    int light_count = (int)world->light_count;

    // split-screen: debug free-fly / no-karts still render a single
    // full-frame viewport (matches render()'s own fallback); game mode
    // shades one viewport per active kart — all in the SAME draw call, the
    // shader picks the right camera/light row per pixel (see phong.fs)
    bool single_view = world->settings.show_debug_gui || world->kart_count == 0;
    int active_viewports = single_view ? 1 : (int)world->kart_count;

    Vector4 viewport_rects[MAX_KART_COUNT];
    vec3f cam_right[MAX_KART_COUNT], cam_up[MAX_KART_COUNT],
        cam_fwd[MAX_KART_COUNT];
    float focal_len[MAX_KART_COUNT], aspect_ratio[MAX_KART_COUNT];
    float boost_amount[MAX_KART_COUNT] = {0}, speed_amount[MAX_KART_COUNT] = {0};
    Vector2 buffer_size;
    buffer_size.x = (float)world->renderer->screen_width;
    buffer_size.y = (float)world->renderer->screen_height;

    PROFILE(PERF_UNIFORM, {
      for (int i = 0; i < light_count; i++) {
        light_colors_norm[i] = ((vec3f){world->lights[i].color.r / 255.0f,
                                        world->lights[i].color.g / 255.0f,
                                        world->lights[i].color.b / 255.0f});
        light_intensities[i] = world->lights[i].intensity;
      }

      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_light_colors_loc,
                      light_colors_norm, SHADER_UNIFORM_VEC3, light_count);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_light_intensities_loc,
                      light_intensities, SHADER_UNIFORM_FLOAT, light_count);
      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_light_count_loc, &light_count,
                     SHADER_UNIFORM_INT);
      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_ambient_loc,
                     &world->settings.ambient_light, SHADER_UNIFORM_FLOAT);
      // world->light_dirs_cam is vec3f[MAX_KART_COUNT][MAX_LIGHT_SOURCES] —
      // contiguous, uploads directly as a flat MAX_KART_COUNT*MAX_LIGHT_SOURCES
      // array matching lightDirs[MAX_VIEWPORTS][MAX_LIGHTS] in the shader
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_light_dirs_loc,
                      world->light_dirs_cam, SHADER_UNIFORM_VEC3,
                      MAX_KART_COUNT * MAX_LIGHT_SOURCES);

      int use_sky = world->renderer->sky_texture.id != 0;
      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_use_sky_loc, &use_sky,
                     SHADER_UNIFORM_INT);

      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_buffer_size_loc, &buffer_size,
                     SHADER_UNIFORM_VEC2);
      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_viewport_count_loc,
                     &active_viewports, SHADER_UNIFORM_INT);

      float shader_time = (float)GetTime();
      SetShaderValue(world->renderer->phong_shader,
                     world->renderer->phong_time_loc, &shader_time,
                     SHADER_UNIFORM_FLOAT);

      for (int i = 0; i < active_viewports; i++) {
        viewport vp = single_view
                          ? viewport_full_frame(world->renderer)
                          : compute_kart_viewport(i, active_viewports,
                                                  world->renderer->screen_width,
                                                  world->renderer->screen_height);
        viewport_rects[i].x = (float)vp.x;
        viewport_rects[i].y = (float)vp.y;
        viewport_rects[i].z = (float)vp.w;
        viewport_rects[i].w = (float)vp.h;

        const cam *c = single_view ? world->cam : &world->game_cams[i];
        cam_right[i] = rotate_y(rotate_x((vec3f){1, 0, 0}, c->pitch), c->yaw);
        cam_up[i] = rotate_y(rotate_x((vec3f){0, 1, 0}, c->pitch), c->yaw);
        cam_fwd[i] = rotate_y(rotate_x((vec3f){0, 0, 1}, c->pitch), c->yaw);
        focal_len[i] = c->focal_length;
        aspect_ratio[i] = vp.aspect_ratio;

        /* speed-boost screen FX: eased blend from this viewport's kart */
        int kart_idx = single_view ? 0 : i;
        if (world->kart_count > (size_t)kart_idx) {
          boost_amount[i] = world->karts[kart_idx].boost_visual;
          speed_amount[i] = kart_speed_ratio(&world->karts[kart_idx],
                                             &world->kart_tuning);
        }
      }

      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_viewport_rects_loc,
                      viewport_rects, SHADER_UNIFORM_VEC4, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_cam_right_loc, cam_right,
                      SHADER_UNIFORM_VEC3, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_cam_up_loc, cam_up,
                      SHADER_UNIFORM_VEC3, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_cam_fwd_loc, cam_fwd,
                      SHADER_UNIFORM_VEC3, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_focal_loc, focal_len,
                      SHADER_UNIFORM_FLOAT, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_aspect_loc, aspect_ratio,
                      SHADER_UNIFORM_FLOAT, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_boost_loc, boost_amount,
                      SHADER_UNIFORM_FLOAT, MAX_KART_COUNT);
      SetShaderValueV(world->renderer->phong_shader,
                      world->renderer->phong_speed_loc, speed_amount,
                      SHADER_UNIFORM_FLOAT, MAX_KART_COUNT);
    });

    // GPU-bound: draw + fragment shader execution. One full-window draw
    // call shades every viewport at once — the fragment shader looks up
    // which viewport rect each pixel falls in (see phong.fs) and uses that
    // viewport's row of the arrays set above. No scissor, no per-viewport
    // draw calls.
    PROFILE(PERF_SHADING, {
      BeginShaderMode(world->renderer->phong_shader);
      // Texture-sampler uniforms must be registered after BeginShaderMode:
      // SetShaderValueTexture only queues the texture for lazy binding "on
      // drawing" (raylib's rlSetUniformSampler), and BeginShaderMode
      // flushes the pending render batch whenever the shader changes,
      // discarding that queued registration before the real draw call
      // below ever sees it.
      SetShaderValueTexture(world->renderer->phong_shader,
                            world->renderer->phong_normal_map_loc,
                            world->renderer->normal_texture);
      if (world->renderer->sky_texture.id != 0)
        SetShaderValueTexture(world->renderer->phong_shader,
                              world->renderer->phong_sky_map_loc,
                              world->renderer->sky_texture);
      DrawTexturePro(
          world->renderer->albedo_texture,
          (Rectangle){0, 0, world->renderer->albedo_texture.width,
                      world->renderer->albedo_texture.height},
          (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
          (Vector2){0, 0}, 0.0f, WHITE);
      EndShaderMode();
    });
  } else {
    // CPU path: framebuffer is already fully lit (flat Lambertian, per
    // triangle) by draw_triangle_pixels_tiled/color_scale in draw.c — just
    // upload and blit it directly, no shader involved.
    PROFILE(PERF_UPLOAD, UpdateTexture(world->renderer->screen_texture,
                                       world->renderer->framebuffer));

    // no per-pixel uniform packing or shader pass in this mode — record 0
    // so the Stats overlay reflects the real (zero) cost instead of a
    // stale value from the last time GPU mode ran.
    perf_record(&world->perf.metrics[PERF_UNIFORM], 0.0f);

    PROFILE(PERF_SHADING,
            DrawTexturePro(
                world->renderer->screen_texture,
                (Rectangle){0, 0, world->renderer->screen_texture.width,
                            world->renderer->screen_texture.height},
                (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                (Vector2){0, 0}, 0.0f, WHITE));
  }

#undef PROFILE

  // frametime isn't a measured code block (it's raylib's own per-frame
  // timer), so it's recorded directly rather than through PROFILE
  perf_record(&world->perf.metrics[PERF_FRAMETIME], GetFrameTime() * 1000.0f);

  // recompute 1% lows once per frame — sorting a ~1024-float copy per
  // metric is microseconds, negligible next to everything else above
  for (int i = 0; i < PERF_METRIC_COUNT; i++)
    perf_recompute_1pct_low(&world->perf.metrics[i]);
}
