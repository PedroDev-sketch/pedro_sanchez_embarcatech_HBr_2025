#include <stdio.h>
#include "src/unity.h"

void setUp(void) {}
void tearDown(void) {}

float adc_to_celsius(uint16_t adc_val)
{
    const float conversion = 3.3f / (1 << 12);
    float voltage = adc_val * conversion;
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    return temperature;
}

void adc_to_celsius_unity_test(void)
{
    uint16_t tension = 876;
    float expected_temperature = 27.0f;
    float buffer = 1.0f;
    TEST_ASSERT_FLOAT_WITHIN(buffer, expected_temperature, adc_to_celsius(tension));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(adc_to_celsius_unity_test);
    return UNITY_END();
}