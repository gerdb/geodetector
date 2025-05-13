/**
 sebulli.com
 */

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "global.h"
#include "icons.h"

int target_pos;
int target_icon;
int icon_1s_cnt = 0;
int act_icon = -1;
bool prepos = false;

void ICONS_Init()
{

   // Tell the SERVO pin that the PWM is in charge of its value.
   gpio_set_function(PIN_SERVO, GPIO_FUNC_PWM);
   // Figure out which slice we just connected to the PIN_SERVO pin
   uint slice_numServo = pwm_gpio_to_slice_num(PIN_SERVO);

   // Get some sensible defaults for the slice configuration. By default, the
   // counter is allowed to wrap over its maximum range (0 to 2**16-1)
   pwm_config configServo = pwm_get_default_config();

   // 125MHz / 125 / 20000 = 20ms
   configServo.top = 20000;
   // Set divider, reduces counter clock to sysclock/this value
   pwm_config_set_clkdiv(&configServo, 125.f);
   // Load the configuration into our PWM slice, and set it running.
   pwm_init(slice_numServo, &configServo, true);
   pwm_set_gpio_level(PIN_SERVO, -1 * (int32_t)SERVO_60_DEG + 1500 + SERVO_OFFSET);
}

void ICONS_Select(Icons icon)
{
   target_icon = (int)icon;
   printf("ICONS_Select %d\n", target_icon);
}

void ICONS_Task1ms()
{
   icon_1s_cnt++;
   if (icon_1s_cnt >= 1000)
   {
      icon_1s_cnt = 0;

      prepos = (target_icon != act_icon);
      act_icon = target_icon;

      // calculate servo pos (pulse time in µs)
      target_pos = ((int32_t)act_icon - 1) * (int32_t)SERVO_60_DEG + 1500 + SERVO_OFFSET;
      if (prepos)
      {
         target_pos += 50;
      }

      // printf("target_pos %d\n", target_pos);
   }

   // PT1 filter with 0.512s tau
   // target_pos_filtL += (target_pos - target_pos_filt);
   // target_pos_filt = target_pos_filtL / 64;
   // Set Servo position
   pwm_set_gpio_level(PIN_SERVO, target_pos);
}