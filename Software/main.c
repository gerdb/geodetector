/**
 sebulli.com
 */
#define MYDEBUG
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"

#include "global.h"
#include "icons.h"
#include "copro.h"
#include "gps.h"
#include "audio.h"

void MAIN_Debug(void);

bool flag1ms = false;
int timer1s_cnt = 0;
int timer10s_cnt = 0;
uint8_t icon = 0;
int32_t intens_UV = 0;
int32_t fade = 0;
uint32_t auto_off_cnt = 0;
int key_debounce_cnt = 0;
int cmd_store = TARGET_NONE;

void on_pwm_wrap()
{
    flag1ms = true;

    static bool going_up = true;
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(PIN_UV_LED));

    // Square the fade value to make the LED's brightness appear more linear
    // Note this range matches with the wrap value
    pwm_set_gpio_level(PIN_UV_LED, fade * (50 + fade / 2)); // fade);
}

int main()
{

    stdio_init_all();

    // Hold power
    gpio_init(PIN_PWR_HOLD);
    gpio_set_dir(PIN_PWR_HOLD, GPIO_OUT);
    gpio_put(PIN_PWR_HOLD, 0);

    gpio_init(PIN_TEST);
    gpio_set_dir(PIN_TEST, GPIO_OUT);
    gpio_put(PIN_TEST, 1);

    gpio_init(PIN_KEY);
    gpio_set_dir(PIN_KEY, GPIO_IN);
    gpio_set_pulls(PIN_KEY, false, true);

    GPS_Init();

    // Tell the LED pin that the PWM is in charge of its value.
    gpio_set_function(PIN_UV_LED, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(PIN_UV_LED);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), on_pwm_wrap);
    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();

    // 125MHz / 12.52 / 10000 = 1kHz
    config.top = 10000;
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 12.5f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    ICONS_Init();

    AUDIO_Init();

    multicore_launch_core1(core1_main);

    while (true)
    {
        if (flag1ms)
        {
            flag1ms = false;

            // 1ms tasks
            ICONS_Task1ms();

            if (gpio_get(PIN_KEY) != 0)
            {
                if (key_debounce_cnt < 3100)
                {
                    key_debounce_cnt++;
                }
                if (key_debounce_cnt == 1000)
                {
                    cmd_store = TARGET_TREASURE;
                }
                if (key_debounce_cnt == 3000)
                {
                    cmd_store = TARGET_GHOST;
                }
            }
            else
            {
                if (key_debounce_cnt > 0)
                {
                    key_debounce_cnt--;
                }
            }

            timer1s_cnt++;
            timer10s_cnt++;

            if (timer1s_cnt >= 1000)
            {
                timer1s_cnt = 0;
                auto_off_cnt++;
                if (speed > 0.5f)
                {
                    auto_off_cnt = 0;
                }

#ifdef MYDEBUG
                CORE1_Debug();
                GPS_Debug();
                MAIN_Debug();
#endif

                if (fix >= 2)
                {
                    if (target.vals.treasure_type == 2)
                    {
                        ICONS_Select(GHOST);
                    }
                    else
                    {
                        ICONS_Select(TREASURE);
                    }
                }
                else
                {
                    ICONS_Select(WAIT);
                }
            }

            /*
            if (timer10s_cnt >= 10000)
            {
                timer10s_cnt = 0;
                if (icon == 0) ICONS_Select(TREASURE);
                if (icon == 1) ICONS_Select(GHOST);
                if (icon == 2) ICONS_Select(TREASURE);
                if (icon == 3) ICONS_Select(WAIT);
                icon++;
                if (icon >= 4)
                {
                    icon = 0;
                }
            }
            */
        }

        GPS_Read();

        // intens_UV 0..2047
        intens_UV = vco1cnt / 16384 - 3072;
        if (intens_UV < -2048)
        {
            intens_UV += 4096;
        }
        if (intens_UV < 0)
        {
            intens_UV = 0 - intens_UV;
        }
        fade = (intens_UV * 100) / 2048;

        AUDIO_Fill_Buffer();
    }

    return 0;
}

void MAIN_Debug(void)
{
    printf("auto_off_cnt %d\n", auto_off_cnt);
}