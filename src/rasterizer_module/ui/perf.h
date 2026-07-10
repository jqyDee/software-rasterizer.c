#pragma once

typedef struct world_s world;

typedef struct {
  float clear_ms;
  float render_ms;
  float upload_ms;
  float blit_ms;
} perf_stats;

void profile(world *world);
