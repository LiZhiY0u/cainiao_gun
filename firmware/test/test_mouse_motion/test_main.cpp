#include <Arduino.h>
#include <unity.h>
#include "MouseMotionFilter.h"

void setUp() {}
void tearDown() {}

void test_subpixel_motion_accumulates()
{
    MouseMotionFilter filter(0.01f);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(-1, filter.update(0, 25).x);
}

void test_negative_input_accumulates_in_opposite_direction()
{
    MouseMotionFilter filter(0.01f);
    filter.update(0, -25);
    filter.update(0, -25);
    filter.update(0, -25);
    TEST_ASSERT_EQUAL_INT8(1, filter.update(0, -25).x);
}

void test_axes_keep_current_final_hid_direction()
{
    MouseMotionFilter filter(4.0f / 250.0f);
    const MouseDelta delta = filter.update(250, 250);
    TEST_ASSERT_EQUAL_INT8(-4, delta.x);
    TEST_ASSERT_EQUAL_INT8(4, delta.y);
}

void test_output_is_clamped_to_hid_range()
{
    MouseMotionFilter filter(4.0f / 250.0f);
    const MouseDelta delta = filter.update(32767, 32767);
    TEST_ASSERT_EQUAL_INT8(-127, delta.x);
    TEST_ASSERT_EQUAL_INT8(127, delta.y);
}

void test_reset_discards_fractional_residual()
{
    MouseMotionFilter filter(0.01f);
    filter.update(0, 25);
    filter.update(0, 25);
    filter.reset();
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
    TEST_ASSERT_EQUAL_INT8(0, filter.update(0, 25).x);
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_subpixel_motion_accumulates);
    RUN_TEST(test_negative_input_accumulates_in_opposite_direction);
    RUN_TEST(test_axes_keep_current_final_hid_direction);
    RUN_TEST(test_output_is_clamped_to_hid_range);
    RUN_TEST(test_reset_discards_fractional_residual);
    UNITY_END();
}

void loop() {}
