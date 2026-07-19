#include <stdio.h>

#include "gui_stats.h"
#include "gui_window.h"

void draw_stats_window(world *world, gui_state_t *gs, float delta_time) {
  const float STATS_W = 340.0f;
  const float STATS_CONTENT = PAD + 11.0f * ROW_H + PAD;

  if (do_window(&gs->stats_win, "Stats", STATS_W, STATS_CONTENT)) {
    float x = gs->stats_win.pos.x + PAD;
    float y = gs->stats_win.pos.y + TITLE_H + PAD;
    float aw = STATS_W - PAD * 2.0f;
    char buf[96];

    perf_metric_t *frametime = &world->perf.metrics[PERF_FRAMETIME];
    perf_metric_t *clear = &world->perf.metrics[PERF_CLEAR];
    perf_metric_t *render = &world->perf.metrics[PERF_RENDER];
    perf_metric_t *upload = &world->perf.metrics[PERF_UPLOAD];
    perf_metric_t *uniform = &world->perf.metrics[PERF_UNIFORM];
    perf_metric_t *shading = &world->perf.metrics[PERF_SHADING];
    perf_metric_t *pupdate = &world->perf.metrics[PERF_UPDATE];
    perf_metric_t *pgui = &world->perf.metrics[PERF_GUI];
    perf_metric_t *pinput = &world->perf.metrics[PERF_INPUT];

    int fps_1pct_low = frametime->low_1pct_ms > 0.0f
                           ? (int)(1000.0f / frametime->low_1pct_ms)
                           : 0;
    snprintf(buf, sizeof(buf),
             "FPS: %d (1%% low: %d)  frametime: %.2f (1%% low: %.2f) ms",
             delta_time > 0.0f ? (int)(1.0f / delta_time) : 0, fps_1pct_low,
             frametime->avg_ms, frametime->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    snprintf(buf, sizeof(buf), "buf: %dx%d   disp: %dx%d",
             world->renderer->screen_width, world->renderer->screen_height,
             world->renderer->display_width, world->renderer->display_height);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    snprintf(buf, sizeof(buf), "cam: %.2f  %.2f  %.2f", world->cam->pos.x,
             world->cam->pos.y, world->cam->pos.z);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    GuiLine((Rectangle){x, y, aw, 1}, NULL);
    y += 6;

    float total = clear->avg_ms + render->avg_ms + upload->avg_ms +
                  uniform->avg_ms + shading->avg_ms;
    snprintf(buf, sizeof(buf), "clear:   %.2f ms  (1%% low: %.2f)",
             clear->avg_ms, clear->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "render:  %.2f ms  (1%% low: %.2f)",
             render->avg_ms, render->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "upload:  %.2f ms  (1%% low: %.2f)",
             upload->avg_ms, upload->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "uniform: %.2f ms  (1%% low: %.2f)",
             uniform->avg_ms, uniform->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "shading: %.2f (1%% low: %.2f)  [total: %.2f]",
             shading->avg_ms, shading->low_1pct_ms, total);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;

    GuiLine((Rectangle){x, y, aw, 1}, NULL);
    y += 6;

    snprintf(buf, sizeof(buf), "input:   %.2f ms  (1%% low: %.2f)",
             pinput->avg_ms, pinput->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "update:  %.2f ms  (1%% low: %.2f)",
             pupdate->avg_ms, pupdate->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
    y += ROW_H;
    snprintf(buf, sizeof(buf), "gui:     %.2f ms  (1%% low: %.2f)",
             pgui->avg_ms, pgui->low_1pct_ms);
    GuiLabel((Rectangle){x, y, aw, ROW_H}, buf);
  }
}
