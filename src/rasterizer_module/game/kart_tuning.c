#include "kart_tuning.h"

void kart_tuning_defaults(kart_tuning *t) {
  t->accel_rate = 10.0f;
  t->max_speed = 25.0f;
  t->max_reverse_speed = 10.0f;
  t->turn_rate = 1.0f;
  t->friction_coefficient = 0.6f;
  t->offroad_friction = 2.5f;
  t->offroad_max_speed = 10.0f;
  t->boost_accel = 20.0f;
  t->boost_speed_mult = 1.5f;
  t->boost_duration_max = 2.0f;
  t->min_boost_charge = 0.2f;
  t->max_drift_angle = 25.0f;
  t->drift_slip_min = 10.0f;
  t->drift_angle_response = 70.0f;
  t->min_drift_speed = 5.0f;
  t->gravity = 20.0f;
  t->jump_velocity = 8.0f;
}
