#include "pwm_hw.h"

#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_sig_map.h"
#include "soc/ledc_struct.h"
#include "soc/system_struct.h"

#define PWM_RES_BITS      13
#define PWM_PERIOD_TICKS  (1u << PWM_RES_BITS)
#define PWM_HZ            50
#define APB_HZ            80000000u
#define PERIOD_US         20000u

#define CLK_DIV ((uint32_t)(((uint64_t)APB_HZ << 8) / (PWM_HZ * PWM_PERIOD_TICKS)))

#define TMR   LEDC.timer_group[0].timer[0]
#define CH    LEDC.channel_group[0].channel[0]

static void pwm_hw_enable_clock(void)
{
    SYSTEM.perip_clk_en0.ledc_clk_en = 1;
    SYSTEM.perip_rst_en0.ledc_rst = 1;
    SYSTEM.perip_rst_en0.ledc_rst = 0;

    LEDC.conf.apb_clk_sel = 1;   // APB 80 MHz
    LEDC.conf.clk_en = 1;
}

void pwm_hw_init(int gpio)
{
    pwm_hw_enable_clock();

    /* ---- timer ---- */
    TMR.conf.rst = 1;                         // hold in reset while configuring
    TMR.conf.pause = 0;
    TMR.conf.duty_resolution = PWM_RES_BITS;
    TMR.conf.clock_divider = CLK_DIV;
    TMR.conf.low_speed_update = 1;                     // latch divider + resolution
    TMR.conf.rst = 0;                         // release reset -> counter runs

    /* ---- channel ---- */
    CH.conf0.timer_sel = 0;
    CH.conf0.idle_lv = 0;
    CH.hpoint.hpoint = 0;
    CH.duty.duty = 0;
    CH.conf1.duty_scale = 0;
    CH.conf1.duty_cycle = 1;
    CH.conf1.duty_num = 1;
    CH.conf1.duty_inc = 1;
    CH.conf0.sig_out_en = 1;
    CH.conf1.duty_start = 1;
    CH.conf0.low_speed_update = 1;                     // latch channel config

    /* ---- GPIO matrix ---- */
    gpio_reset_pin(gpio);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal(gpio, LEDC_LS_SIG_OUT0_IDX, false, false);
}

void pwm_hw_set_pulse_us(uint32_t pulse_us)
{
    if (pulse_us > PERIOD_US) pulse_us = PERIOD_US;

    const uint32_t duty = (pulse_us * PWM_PERIOD_TICKS) / PERIOD_US;

    CH.hpoint.hpoint = 0;
    CH.duty.duty = duty << 4;                 // 4 fractional bits
    CH.conf1.duty_scale = 0;
    CH.conf1.duty_cycle = 1;
    CH.conf1.duty_num = 1;
    CH.conf1.duty_inc = 1;

    CH.conf0.sig_out_en = 1;
    CH.conf1.duty_start = 1;
    CH.conf0.low_speed_update = 1;                     // must come last
}