#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../math/vec.h"
#include "../engine/input.h"
#include "kart_tuning.h"

typedef struct world_s world;
typedef struct track_s track;

#define MAX_KART_COUNT 4
#define RACE_CHECKPOINTS 20 // virtual checkpoints per lap (anti-cut)

typedef enum drift_state_e {
  DRIFT_NONE,
  DRIFT_DRIFTING,
  DRIFT_BOOST,
} drift_state;

typedef struct kart_s {
  vec3f pos;
  vec3f vel;         // world-space vel
  vec3f forward_dir; // facing dir
  float yaw;         // rot around Y axis

  // GROUND STATE
  bool is_grounded;
  float ground_friction;

  // DRIFT STATE
  drift_state drift_state;
  float drift_dir;               // locked at drift start: +1 right, -1 left
  float drift_angle;             // sideways slip
  float drift_boost_accumulated; // 0..1
  float boost_timer;             // remaining boost seconds (DRIFT_BOOST)
  float boost_visual;            // eased 0..1 for FOV punch + shader FX

  // PRESENTATION (visual only, no physics impact)
  float visual_drift_yaw; // eased extra mesh yaw (deg) overselling the drift
  float visual_lean;      // eased roll (deg) into the corner
  float cam_yaw;          // smoothed camera heading, follows travel dir

  // RACE STATE
  float race_progress;  // 0..1 along the track loop (wraps at start line)
  int next_checkpoint;  // sequential virtual checkpoint to pass next
  int lap;              // 1-based
  float lap_time;       // running seconds this lap
  float last_lap_time;  // 0 = none finished yet
  float best_lap_time;  // 0 = none set yet
  float wrongway_timer; // seconds of sustained backwards progress
  bool wrong_way;

  // AIR STATE
  float vertical_velocity;
  float air_velocity;

  // INPUT
  int gamepad_id;
  controller_input input;

  // RENDERING
  int instance_idx; /* index into world->instances, -1 = no visual */
} kart;

void init_karts(world *world, size_t player_count);
void destroy_karts(world *world);


void kart_update(kart *kart, const track *trk, const kart_tuning *tune,
                  const float delta_time);
void kart_reset_frame_input(kart *kart);

/* current speed as 0..1 fraction of top speed (for HUD / shader FX) */
float kart_speed_ratio(const kart *kart, const kart_tuning *tune);

/* place karts on a starting grid behind the track's start line (point 0,
 * facing the driving direction) and reset race state — call after the
 * track is loaded/built, and again whenever the track layout changes */
void kart_spawn_at_start(world *world);

/* claim scene instances as kart visuals (call after scene_load) */
void link_kart_instances(world *world);
/* copy kart pos/yaw into the linked mesh instances (call once per frame) */
void kart_sync_instances(world *world);
/* game-mode camera follow: trails each kart's smoothed heading (call once
 * per frame, game mode only) */
void kart_update_cameras(world *world);
