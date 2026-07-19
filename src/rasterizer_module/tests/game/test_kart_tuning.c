#include "../../game/kart_tuning.h"
#include "../unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_defaults_match_original_tuning_values(void) {
  kart_tuning t;
  kart_tuning_defaults(&t);
  TEST_ASSERT_EQUAL_FLOAT(10.0f, t.accel_rate);
  TEST_ASSERT_EQUAL_FLOAT(25.0f, t.max_speed);
  TEST_ASSERT_EQUAL_FLOAT(10.0f, t.max_reverse_speed);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, t.turn_rate);
  TEST_ASSERT_EQUAL_FLOAT(0.6f, t.friction_coefficient);
  TEST_ASSERT_EQUAL_FLOAT(2.5f, t.offroad_friction);
  TEST_ASSERT_EQUAL_FLOAT(10.0f, t.offroad_max_speed);
  TEST_ASSERT_EQUAL_FLOAT(20.0f, t.boost_accel);
  TEST_ASSERT_EQUAL_FLOAT(1.5f, t.boost_speed_mult);
  TEST_ASSERT_EQUAL_FLOAT(2.0f, t.boost_duration_max);
  TEST_ASSERT_EQUAL_FLOAT(0.2f, t.min_boost_charge);
  TEST_ASSERT_EQUAL_FLOAT(25.0f, t.max_drift_angle);
  TEST_ASSERT_EQUAL_FLOAT(10.0f, t.drift_slip_min);
  TEST_ASSERT_EQUAL_FLOAT(70.0f, t.drift_angle_response);
  TEST_ASSERT_EQUAL_FLOAT(5.0f, t.min_drift_speed);
  TEST_ASSERT_EQUAL_FLOAT(20.0f, t.gravity);
  TEST_ASSERT_EQUAL_FLOAT(8.0f, t.jump_velocity);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_match_original_tuning_values);
  return UNITY_END();
}
