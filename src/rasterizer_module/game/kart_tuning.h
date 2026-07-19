#pragma once

typedef struct kart_tuning_s {
  float accel_rate;
  float max_speed;
  float max_reverse_speed;
  float turn_rate;
  float friction_coefficient;
  float offroad_friction;
  float offroad_max_speed;
  float boost_accel;
  float boost_speed_mult;
  float boost_duration_max;
  float min_boost_charge;
  float max_drift_angle;
  float drift_slip_min;
  float drift_angle_response;
  float min_drift_speed;
  float gravity;
  float jump_velocity;
} kart_tuning;

void kart_tuning_defaults(kart_tuning *t);
