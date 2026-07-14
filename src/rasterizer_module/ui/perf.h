#pragma once

typedef struct world_s world;

// rolling history window for 1% low calculation — ~17s at 60fps, enough
// samples for "worst 1%" to mean something statistically (10+ samples)
#define PERF_HISTORY_SIZE 1024

typedef enum {
  PERF_FRAMETIME,
  PERF_CLEAR,
  PERF_RENDER,
  PERF_UPLOAD,
  PERF_UNIFORM,
  PERF_SHADING,
  PERF_UPDATE,
  PERF_GUI,
  PERF_INPUT,

  PERF_METRIC_COUNT,
} perf_metric_id;

typedef struct {
  float avg_ms;      /* EWMA average — steady-state cost */
  float low_1pct_ms; /* avg of the worst 1% of samples in history — stutter */
  float samples[PERF_HISTORY_SIZE]; /* ring buffer */
  int sample_count; /* samples written so far, caps at PERF_HISTORY_SIZE */
  int next;         /* ring buffer write cursor */
} perf_metric_t;

typedef struct {
  perf_metric_t metrics[PERF_METRIC_COUNT + 1];
} perf_stats;

void profile(world *world);
void perf_record(perf_metric_t *m, float ms);
void perf_recompute_1pct_low(perf_metric_t *m);
