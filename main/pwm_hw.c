#include "pwm_hw.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // GPIO 5
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // 13-bit resolution (8192 steps)
#define LEDC_FREQUENCY          (50)              // 50Hz (20ms period for SG90)

void pwm_hw_init(int gpio)
{
    // 1. Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio,
        .duty           = 0, // Set initial duty to 0
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void pwm_hw_set_pulse_us(uint32_t pulse_us)
{
    // SG90 safe pulse bounds roughly 500us (0°) to 2400us (180°)
    if (pulse_us > 20000) {
        pulse_us = 20000;
    }

    // Convert microsecond pulse width to 13-bit duty value (8192 steps per 20000us period)
    uint32_t duty = (pulse_us * 8192) / 20000;

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}