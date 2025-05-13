/**
 sebulli.com
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#define PIN_UV_LED (2)
#define PIN_WS2812 (3)
#define PIN_SERVO (4)
#define PIN_TEST (5)
#define I2C_SDA_PIN (6)
#define I2C_SCL_PIN (7)
#define PIN_KEY (8)
#define PIN_PWR_HOLD (9)
#define PIN_ADC (26)

#define TARGET_NONE (0)
#define TARGET_TREASURE (1)
#define TARGET_GHOST (2)

typedef union
{
    struct
    {
        uint32_t magic;
        float latitude;  // north
        float longitude; // east
        uint32_t treasure_type;
    } vals;
    uint8_t ucBytes[16];
} t_target;

extern uint32_t auto_off_cnt;

extern int cmd_store;

#endif