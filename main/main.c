#include <stdio.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pwm_hw.h"

static const char *TAG = "servo-pot";

#define SERVO_GPIO      GPIO_NUM_5

#define POT_GPIO        GPIO_NUM_4
#define POT_ADC_UNIT    ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_3
#define POT_WORKING_ANGLE 300

#define US_AT_0         500
#define US_AT_180       2400

#define ADC_MAX         4095

#define UPDATE_MS       20


static uint32_t deg_to_us(int deg)
{
    if (deg < 0) {
        deg = 0;
    }

    if (deg > 180) {
        deg = 180;
    }

    return US_AT_0
           + ((uint32_t)deg * (US_AT_180 - US_AT_0)) / 180;
}


static void servo_write_deg(int deg)
{
    uint32_t us = deg_to_us(deg);

    pwm_hw_set_pulse_us(us);

    ESP_LOGI(TAG, "%d deg -> %u us", deg, (unsigned)us);
}


static int adc_to_deg(int adc)
{
    if (adc < 0) {
        adc = 0;
    }

    if (adc > ADC_MAX) {
        adc = ADC_MAX;
    }
    float part = 1- (float)adc / ADC_MAX;
    int clamped = (int)(part * POT_WORKING_ANGLE) - 120;
    if (clamped < 0) {
        clamped = 0;
    }
    return clamped;
}


void app_main(void)
{
    /*
     * Servo:
     *   GPIO5 -> SG90 signal
     *
     * Potentiometer:
     *   one side -> 3.3 V
     *   other side -> GND
     *   wiper -> GPIO4 / ADC
     */

    pwm_hw_init(SERVO_GPIO);

    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = POT_ADC_UNIT,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&adc_config, &adc_handle)
    );

    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            POT_ADC_CHANNEL,
            &adc_channel_config
        )
    );

    ESP_LOGI(TAG, "Servo on GPIO%d", (int)SERVO_GPIO);
    ESP_LOGI(TAG, "Potentiometer on GPIO%d", (int)POT_GPIO);
    ESP_LOGI(TAG, "Starting potentiometer control");

    while (1) {
        int adc_raw;

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                POT_ADC_CHANNEL,
                &adc_raw
            )
        );

        int angle = adc_to_deg(adc_raw);

        servo_write_deg(angle);

        vTaskDelay(pdMS_TO_TICKS(UPDATE_MS));
    }
}