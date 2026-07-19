#include "kart.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../math/rotation.h"
#include "../math/transformation.h"
#include "../world.h"

#define KART_MESH_NAME "turbo_dolphin"

/* Tuning. Scene scale reference: debug cam move_speed = 5 units/sec.
 * Core driving-feel constants live in kart_tuning (world->kart_tuning),
 * editable at runtime via the debug GUI. Remaining #defines below are
 * visual-easing / race-timing constants, not curated for live tuning. */
#define BOOST_ACCUM_RATE 0.2f    // boost meter fill speed
#define OVERSPEED_DECAY 20.0f    // units/sec² bleed-off above the cap
#define DRIFT_START_STEER 0.3f      // |stick| needed to initiate a drift
#define DRIFT_TURN_MIN 0.2f         // rad/sec auto-turn, stick held outward
#define DRIFT_TURN_MAX 1.4f         // rad/sec auto-turn, stick held inward
#define BOOST_VISUAL_EASE 6.0f   // 1/sec ramp speed of the boost FX blend
#define VISUAL_DRIFT_EXTRA 8.0f  // deg extra mesh yaw overselling the drift
#define VISUAL_LEAN_DRIFT 5.0f   // deg roll into the corner while drifting
#define VISUAL_LEAN_STEER 2.0f   // deg roll from normal steering (at speed)
#define VISUAL_EASE 8.0f         // 1/sec ease for the visual angles
#define CAM_YAW_EASE 3.5f        // 1/sec camera heading smoothing
#define WRONGWAY_GRACE 0.7f      // sec of backwards driving before warning

void kart_create(kart *kart, int gamepad_id);

void init_karts(world *world, size_t player_count) {
  // clamp to MAX_KART_COUNT
  if (player_count > MAX_KART_COUNT) {
    player_count = MAX_KART_COUNT;
  }

  world->karts = malloc(player_count * sizeof(kart));
  if (!world->karts)
    exit(2);

  for (size_t i = 0; i < player_count; i++) {
    kart_create(&world->karts[i], i);
  }
  world->kart_count = player_count;
}

void destroy_karts(world *world) {
  if (world->karts) {
    free(world->karts);
    world->karts = NULL;
  }
  world->kart_count = 0;
}

void kart_create(kart *kart, int gamepad_id) {
  kart->pos = (vec3f){0, 0, 0};
  kart->vel = (vec3f){0, 0, 0};
  /* yaw in radians, engine convention: forward = (sin(yaw), 0, cos(yaw)).
   * Start at PI = facing -Z, same direction the game camera faces on init. */
  kart->yaw = (float)M_PI;
  kart->forward_dir = rotate_y((vec3f){0, 0, 1}, (float)M_PI);
  kart->is_grounded = true;
  kart->drift_state = DRIFT_NONE;
  kart->drift_dir = 0.0f;
  kart->drift_angle = 0.0f;
  kart->drift_boost_accumulated = 0.0f;
  kart->boost_timer = 0.0f;
  kart->boost_visual = 0.0f;
  kart->visual_drift_yaw = 0.0f;
  kart->visual_lean = 0.0f;
  kart->cam_yaw = kart->yaw;
  kart->vertical_velocity = 0.0f;
  kart->gamepad_id = gamepad_id;
  kart->instance_idx = -1;

  kart->race_progress = 0.0f;
  kart->next_checkpoint = 1;
  kart->lap = 1;
  kart->lap_time = 0.0f;
  kart->last_lap_time = 0.0f;
  kart->best_lap_time = 0.0f;
  kart->wrongway_timer = 0.0f;
  kart->wrong_way = false;
}

void kart_spawn_at_start(world *world) {
  const track *t = &world->track_data;
  if (t->point_count < 3)
    return;

  /* start line = track point 0; heading = driving direction there */
  vec2f s0 = track_sample_pos(t, 0, 0.0f);
  vec2f s1 = track_sample_pos(t, 0, 0.1f);
  float heading = atan2f(s1.x - s0.x, s1.y - s0.y); /* fwd = (sin,0,cos) */
  float sh = sinf(heading), ch = cosf(heading);

  for (size_t k = 0; k < world->kart_count; k++) {
    kart *kart = &world->karts[k];

    /* two-column grid behind the line, MK style */
    size_t row = k / 2;
    float lateral = (k % 2 == 0 ? -1.0f : 1.0f) * 2.0f;
    float back = ((float)row + 1.0f) * 3.0f;
    kart->pos.x = s0.x + ch * lateral - sh * back;
    kart->pos.z = s0.y - sh * lateral - ch * back;
    kart->pos.y = 0.0f;

    kart->vel = (vec3f){0, 0, 0};
    kart->yaw = heading;
    kart->cam_yaw = heading;
    kart->forward_dir = rotate_y((vec3f){0, 0, 1}, heading);
    kart->vertical_velocity = 0.0f;
    kart->is_grounded = true;
    kart->drift_state = DRIFT_NONE;
    kart->drift_angle = 0.0f;
    kart->drift_boost_accumulated = 0.0f;
    kart->boost_timer = 0.0f;

    /* race state: grid sits behind the line, so lap 0 = "not started" —
     * the first start-line crossing begins lap 1 instead of finishing one */
    float p = track_progress_at(t, kart->pos.x, kart->pos.z);
    kart->race_progress = p;
    kart->next_checkpoint =
        ((int)(p * RACE_CHECKPOINTS) + 1) % RACE_CHECKPOINTS;
    kart->lap = 0;
    kart->lap_time = 0.0f;
    kart->wrongway_timer = 0.0f;
    kart->wrong_way = false;
  }
}

void link_kart_instances(world *world) {
  int mesh_idx = -1;
  for (size_t i = 0; i < world->mesh_data_count; i++) {
    if (strstr(world->mesh_data[i].name, KART_MESH_NAME)) {
      mesh_idx = (int)i;
      break;
    }
  }
  if (mesh_idx < 0) {
    fprintf(stderr, "link_kart_instances: no mesh named '%s' loaded\n",
            KART_MESH_NAME);
    return;
  }

  for (size_t k = 0; k < world->kart_count; k++) {
    kart *kart = &world->karts[k];

    /* claim an existing scene instance of the kart mesh (not taken by an
     * earlier kart) so spawn point can be placed in the editor */
    int found = -1;
    for (size_t i = 0; i < world->instance_count; i++) {
      if (world->instances[i].mesh_idx != mesh_idx)
        continue;
      bool claimed = false;
      for (size_t j = 0; j < k; j++)
        if (world->karts[j].instance_idx == (int)i)
          claimed = true;
      if (!claimed) {
        found = (int)i;
        break;
      }
    }

    /* no free instance in scene → append one at the kart's position */
    if (found < 0) {
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
      mesh_instance *inst = &world->instances[world->instance_count];
      *inst = (mesh_instance){
          .mesh_idx = mesh_idx,
          .pos = kart->pos,
          .rotation = {0, kart->yaw * RAD_TO_DEG, 0},
          .scale = {1, 1, 1},
          .tex_pixels = NULL,
          .tex_w = 0,
          .tex_h = 0,
      };
      inst->tex_path[0] = '\0';
      const char *atp = world->mesh_data[mesh_idx].auto_tex_path;
      if (atp[0] != '\0')
        snprintf(inst->tex_path, sizeof(inst->tex_path), "%s", atp);
      found = (int)world->instance_count;
      world->instance_count++;
    }
    /* the claimed instance is the kart's VISUAL only — spawn position
     * comes from the track start line (kart_spawn_at_start), never from
     * the scene (kart_sync_instances overwrites the instance pos anyway) */
    kart->instance_idx = found;
  }
}

void kart_sync_instances(world *world) {
  for (size_t k = 0; k < world->kart_count; k++) {
    kart *kart = &world->karts[k];
    if (kart->instance_idx < 0 ||
        kart->instance_idx >= (int)world->instance_count)
      continue;
    mesh_instance *inst = &world->instances[kart->instance_idx];
    inst->pos = kart->pos;
    inst->rotation.y = kart->yaw * RAD_TO_DEG + kart->visual_drift_yaw;
    inst->rotation.z = kart->visual_lean;
    inst->scale = (vec3f){0.05f, 0.05f, 0.05f};
  }
}

void kart_update(kart *kart, const track *trk, const kart_tuning *tune,
                  const float delta_time) {

  controller_input input = kart->input;

  // JUMPING
  if (input.jump_requested && kart->is_grounded) {
    kart->vertical_velocity = tune->jump_velocity;
    kart->is_grounded = false;
  }

  // DRIFTING — direction locks at initiation (MK-style), so entering
  // requires actual steering input; holding the button while straight
  // starts the drift the moment the stick moves
  if (input.drift_requested && kart->is_grounded) {
    if (kart->drift_state == DRIFT_NONE &&
        fabsf(input.steering_input) > DRIFT_START_STEER) {
      kart->drift_state = DRIFT_DRIFTING;
      kart->drift_dir = input.steering_input > 0.0f ? 1.0f : -1.0f;
      kart->drift_boost_accumulated = 0.0f;
    }
  } else if (kart->drift_state == DRIFT_DRIFTING) {
    /* release: convert accumulated charge into a timed boost —
     * longer drift = longer boost */
    if (kart->drift_boost_accumulated > tune->min_boost_charge) {
      kart->drift_state = DRIFT_BOOST;
      kart->boost_timer = kart->drift_boost_accumulated * tune->boost_duration_max;
    } else {
      kart->drift_state = DRIFT_NONE;
    }
    /* drift_angle not reset here — it eases back to 0 in the steering block */
  }

  // BOOST countdown
  if (kart->drift_state == DRIFT_BOOST) {
    kart->boost_timer -= delta_time;
    if (kart->boost_timer <= 0.0f) {
      kart->boost_timer = 0.0f;
      kart->drift_state = DRIFT_NONE;
    }
  }

  // ---- presentation state (visual only, physics untouched) ----
  {
    // eased 0..1 blend driving the FOV punch + shader boost FX
    float target = kart->drift_state == DRIFT_BOOST ? 1.0f : 0.0f;
    float blend = BOOST_VISUAL_EASE * delta_time;
    if (blend > 1.0f)
      blend = 1.0f;
    kart->boost_visual += (target - kart->boost_visual) * blend;

    // mesh oversell + lean: drift cocks the model further into the corner
    // than the physical slip; normal steering leans a little at speed
    float target_extra = 0.0f;
    float target_lean = 0.0f;
    if (kart->drift_state == DRIFT_DRIFTING) {
      target_extra = kart->drift_dir * VISUAL_DRIFT_EXTRA;
      target_lean = kart->drift_dir * VISUAL_LEAN_DRIFT;
    } else {
      target_lean = input.steering_input * VISUAL_LEAN_STEER *
                    kart_speed_ratio(kart, tune);
    }
    float vblend = VISUAL_EASE * delta_time;
    if (vblend > 1.0f)
      vblend = 1.0f;
    kart->visual_drift_yaw += (target_extra - kart->visual_drift_yaw) * vblend;
    kart->visual_lean += (target_lean - kart->visual_lean) * vblend;

    // camera heading follows the TRAVEL direction (yaw - drift_angle), so
    // the drift rotation is visible on screen instead of hidden by a
    // nose-locked camera; smoothed for natural lag in every corner
    float cam_target = kart->yaw - kart->drift_angle * DEG_TO_RAD;
    float cblend = CAM_YAW_EASE * delta_time;
    if (cblend > 1.0f)
      cblend = 1.0f;
    kart->cam_yaw += (cam_target - kart->cam_yaw) * cblend;
  }

  // STEERING (yaw in radians; stick right = turn right)
  if (kart->is_grounded) {
    float target_angle = 0.0f;

    if (kart->drift_state == DRIFT_DRIFTING) {
      /* locked drift: kart always turns in drift_dir on its own; the
       * stick only tightens (inward) or widens (outward) the arc.
       * tightness 0..1: stick fully outward = 0, fully inward = 1 */
      float stick_inward = input.steering_input * kart->drift_dir;
      float tightness = (stick_inward + 1.0f) * 0.5f;
      float turn_rate =
          DRIFT_TURN_MIN + tightness * (DRIFT_TURN_MAX - DRIFT_TURN_MIN);
      kart->yaw += kart->drift_dir * turn_rate * delta_time;

      /* constant outward slip, stronger the tighter the drift */
      target_angle = kart->drift_dir *
                     (tune->drift_slip_min +
                      tightness * (tune->max_drift_angle - tune->drift_slip_min));

      /* boost charge scales with tightness; needs real speed */
      float speed_sq =
          kart->vel.x * kart->vel.x + kart->vel.z * kart->vel.z;
      if (speed_sq > tune->min_drift_speed * tune->min_drift_speed) {
        float slip_ratio = fabsf(kart->drift_angle) / tune->max_drift_angle;
        kart->drift_boost_accumulated +=
            BOOST_ACCUM_RATE * slip_ratio * delta_time;
        if (kart->drift_boost_accumulated > 1.0f) {
          kart->drift_boost_accumulated = 1.0f;
        }
      }
    } else {
      /* normal grip steering */
      kart->yaw += input.steering_input * tune->turn_rate * delta_time;
    }

    kart->forward_dir = rotate_y((vec3f){0, 0, 1}, kart->yaw);

    /* smooth slip toward target so slides build up and release
     * instead of snapping */
    float angle_step = tune->drift_angle_response * delta_time;
    float angle_diff = target_angle - kart->drift_angle;
    if (angle_diff > angle_step)
      angle_diff = angle_step;
    if (angle_diff < -angle_step)
      angle_diff = -angle_step;
    kart->drift_angle += angle_diff;
  }

  // ACCELERATION AND FRICTION
  if (kart->is_grounded) {
    /* travel direction lags the nose by drift_angle (MK-style slide:
     * the kart points into the corner more than it moves) */
    vec3f move_dir = rotate_y(
        (vec3f){0, 0, 1}, kart->yaw - kart->drift_angle * DEG_TO_RAD);

    /* signed speed: projection of velocity onto move_dir — NOT forward_dir,
     * that would scrub cos(drift_angle) of the speed away every frame.
     * negative = reversing (sqrtf/magnitude would lose the sign) */
    float speed = kart->vel.x * move_dir.x + kart->vel.z * move_dir.z;

    // Off the road ribbon = grass: heavy drag, low speed cap
    bool on_road = track_is_on_road(trk, kart->pos.x, kart->pos.z);
    float friction =
        on_road ? tune->friction_coefficient : tune->offroad_friction;
    float max_speed = on_road ? tune->max_speed : tune->offroad_max_speed;

    // Apply acceleration (boost raises both the push and the cap;
    // an active boost also overrides grass — charged shortcuts work)
    float accel = input.accel_input * tune->accel_rate;
    if (kart->drift_state == DRIFT_BOOST) {
      accel += tune->boost_accel;
      max_speed = tune->max_speed * tune->boost_speed_mult;
      friction = tune->friction_coefficient;
    }
    speed += accel * delta_time;

    // Apply friction (no drift discount — drifting grants no free speed,
    // the reward comes from the release boost)
    speed *= (1.0f - friction * delta_time);

    /* clamp: above the cap (boost just ended) bleed off gradually
     * instead of snapping down in one frame */
    if (speed > max_speed)
      speed = fmaxf(max_speed, speed - OVERSPEED_DECAY * delta_time);
    speed = fmaxf(-tune->max_reverse_speed, speed);

    // Reapply velocity along the (possibly slipping) travel direction
    kart->vel.x = move_dir.x * speed;
    kart->vel.z = move_dir.z * speed;
  }

  // GRAVITY
  if (!kart->is_grounded) {
    kart->vertical_velocity -= tune->gravity * delta_time;
    kart->pos.y += kart->vertical_velocity * delta_time;

    // Air rotation (optional: allow stick to rotate in air)
    // For now, just apply gravity

    // Ground collision (simple Y-check)
    if (kart->pos.y <= 0.0f) {
      kart->pos.y = 0.0f;
      kart->vertical_velocity = 0.0f;
      kart->is_grounded = true;
    }
  }

  // UPDATE POS
  kart->pos.x += kart->vel.x * delta_time;
  kart->pos.z += kart->vel.z * delta_time;

  // ---- race progress / laps ----
  {
    float p = track_progress_at(trk, kart->pos.x, kart->pos.z);
    float d = p - kart->race_progress;
    if (d > 0.5f) /* wrapped across the start line */
      d -= 1.0f;
    if (d < -0.5f)
      d += 1.0f;
    kart->race_progress = p;

    kart->lap_time += delta_time;

    /* wrong-way: sustained backwards progress (brief reversing while
     * maneuvering stays under the grace period) */
    if (d < -0.00001f)
      kart->wrongway_timer += delta_time;
    else
      kart->wrongway_timer = 0.0f;
    kart->wrong_way = kart->wrongway_timer > WRONGWAY_GRACE;

    /* sequential virtual checkpoints at i/N around the loop — each must
     * be passed in order, so cutting across grass can't skip ahead.
     * Passing checkpoint 0 (start line) with the sequence complete
     * finishes the lap. */
    float cp_pos = (float)kart->next_checkpoint / RACE_CHECKPOINTS;
    float rel = p - cp_pos;
    if (rel < 0.0f)
      rel += 1.0f;
    if (rel < 1.0f / RACE_CHECKPOINTS) {
      if (kart->next_checkpoint == 0) {
        if (kart->lap == 0) {
          /* grid sits behind the line: the first crossing STARTS lap 1 */
          kart->lap = 1;
          kart->lap_time = 0.0f;
        } else {
          kart->last_lap_time = kart->lap_time;
          if (kart->best_lap_time <= 0.0f ||
              kart->lap_time < kart->best_lap_time)
            kart->best_lap_time = kart->lap_time;
          kart->lap_time = 0.0f;
          kart->lap++;
        }
      }
      kart->next_checkpoint = (kart->next_checkpoint + 1) % RACE_CHECKPOINTS;
    }
  }
}

void kart_reset_frame_input(kart *kart) {
  kart->input.jump_requested = false;
}

float kart_speed_ratio(const kart *kart, const kart_tuning *tune) {
  float s = sqrtf(kart->vel.x * kart->vel.x + kart->vel.z * kart->vel.z);
  float r = s / tune->max_speed;
  return r > 1.0f ? 1.0f : r;
}
