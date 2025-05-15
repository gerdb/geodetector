/**
 sebulli.com
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "pico/binary_info.h"

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"

#include "global.h"
#include "copro.h"
#include "gps.h"
#include "audio.h"
#include "ws2812.pio.h"

t_light lights[2];
uint8_t buf_i2c[32];
grbw_t pixels[24];
int r = 0;
int r_cnt = 0;
uint16_t adc_result = 0;
uint32_t adc_result_filt = 0;
uint32_t adc_result_filtL = 0;
int lowvolt_cnt = 0;
uint32_t volts = 0;
bool lowvolt = false;

void core1_main(void)
{
    // Load PIO program
    PIO pio = pio1;
    int sm = 1;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, PIN_WS2812, 800000, WS_IS_RGBW);

    int retrigger = 0;

    lights[0].intens = 0;
    lights[1].intens = 0;
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(PIN_ADC);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    for (int i = 0; i < 2; i++)
    {
        lights[i].intens = 0;
        lights[i].pos = 0;
        lights[i].pos2 = 0;
    }
    while (1)
    {
        for (int i = 0; i < 12; i++)
        {
            int full = (r) / 16;
            int part = (r) % 16;
            int i1 = part * part;
            int i2 = (15 - part) * (15 - part);
            int i1r = 0;
            int i2r = 0;
            int i1g = 0;
            int i2g = 0;
            int i1b = 0;
            int i2b = 0;

            
            if (fix >= 3)
            {
                i1g = i1 / 8;
                i2g = i2 / 8;
                i1b = i1 / 4;
                i2b = i2 / 4;
            }
            else
            if (fix >= 2)
            {
                i1g = i1 / 8;
                i2g = i2 / 8;
            }
            else
            {
                i1r = i1;
                i2r = i2;
            }

            if (lowvolt)
            {
                if (i == full && part >= 15)
                {
                    pixels[i] = RGB(i1, 0, 0);
                }
                else
                {
                    pixels[i] = RGB(0, 0, 0);
                }
            }
            else
            {
                if (i == ((full + 11) % 12))
                {
                    pixels[i] = RGB(i2r, i2g, i2b);
                }
                else if (i == full)
                {
                    pixels[i] = RGB(i1r, i1g, i1b);
                }
                else
                {
                    pixels[i] = RGB(0, 0, 0);
                }
            }

            pixels[i + 12] = pixels[i];
        }

        /*
        // Show direction by white pixel
        if (fix >= 2)
        {
            pixels[dir_rel24] = 0xFFFFFF00;
        }
        */

        // Put all 24 pixel to neopixel-ring
        for (int i = 0; i < 24; i++)
        {
            pio_sm_put_blocking(pio, sm, pixels[i]);
        }

        r_cnt++;
        if (r_cnt >= 20)
        {
            r_cnt = 0;
            r++;
            if (r >= 192)
            {
                r = 0;
            }
        }

        retrigger++;
        if (retrigger >= 700)
        {
            retrigger = 0;
            lights[0].intens = 256;
        }
        if (retrigger == 300)
        {
            lights[1].intens = 256;
        }

        adc_result = adc_read();
        adc_result_filtL += (adc_result - adc_result_filt);
        adc_result_filt = adc_result_filtL / 128;

        // instead of 3.3V I use 3.278V to correct the resistors tolerance
        volts = 2 * (adc_result_filt * 3278) / 4096;
        if (volts > 1000 && volts < (4 * 1400))
        {
            if (lowvolt_cnt < 10000)
            {
                lowvolt_cnt++;
            }
            else
            {
                lowvolt = true;
            }
        }
        else if (volts > (4 * 1450))
        {
            if (lowvolt_cnt > 0)
            {
                lowvolt_cnt--;
            }
            else
            {
                lowvolt = false;
            }
        }

        buf_i2c[0] = 0;
        i2c_write_blocking(i2c1, ADDR, buf_i2c, 1, true);  // true to keep master control of bus
        i2c_read_blocking(i2c1, ADDR, buf_i2c, 16, false); // false - finished with bus
        t_target target_tmp;
        for (int i = 0; i < 16; i++)
        {
            target_tmp.ucBytes[i] = buf_i2c[i];
        }

        // Valid eeprom position?
        if (
            target_tmp.vals.magic == 0xDEADBEEF &&
            isnormal(target_tmp.vals.latitude) &&
            isnormal(target_tmp.vals.longitude) &&
            (target_tmp.vals.treasure_type >= TARGET_TREASURE) &&
            (target_tmp.vals.treasure_type <= TARGET_GHOST) &&
            (target_tmp.vals.latitude <= 90.0f) &&
            (target_tmp.vals.latitude >= -90.0f) &&
            (target_tmp.vals.longitude <= 90.0f) &&
            (target_tmp.vals.longitude >= -90.0f))
        {

            target.vals.latitude = target_tmp.vals.latitude;
            target.vals.longitude = target_tmp.vals.longitude;
            target.vals.treasure_type = target_tmp.vals.treasure_type;
        }

        if (cmd_store != TARGET_NONE)
        {

            if (fix >= 2)
            {
                buf_i2c[0] = 0;
                t_target target_tmp;

                target_tmp.vals.magic = 0xDEADBEEF;
                target_tmp.vals.latitude = north;
                target_tmp.vals.longitude = east;
                target_tmp.vals.treasure_type = cmd_store;

                if (cmd_store == TARGET_TREASURE)
                {
                    cmd_store = TARGET_NONE;
                    channel[1].play = true;
                }
                if (cmd_store == TARGET_GHOST)
                {
                    cmd_store = TARGET_NONE;
                    channel[2].play = true;
                }

                for (int i = 0; i < 16; i++)
                {
                    buf_i2c[i + 1] = target_tmp.ucBytes[i];
                }

                i2c_write_blocking(i2c1, ADDR, buf_i2c, 1 + 16, false); // false - finished with bus
            }
            else
            {
                cmd_store = TARGET_NONE;
                channel[3].play = true;
            }
        }

        int auto_off_cnt_limit;
        if (fix >= 2)
        {
            auto_off_cnt_limit = 10 * 60; // 10min
        }
        else
        {
            auto_off_cnt_limit = 30 * 60; // 30min
        }
        if (auto_off_cnt > auto_off_cnt_limit)
        {
            if (volts > 1000)
            {
                channel[4].play = true;
            }
            auto_off_cnt_limit -= 2;
        }

        sleep_ms(1);
    }
}

void CORE1_Debug(void)
{
    printf("volt %d\n", volts);
    printf("target %f %f %d %d \n", target.vals.latitude, target.vals.longitude, target.vals.magic, target.vals.treasure_type);
}