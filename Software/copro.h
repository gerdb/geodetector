/**
 sebulli.com
 */

#ifndef COPRO_H
#define COPRO_H

#define WS_IS_RGB (true)
#define WS_IS_RGBW (!WS_IS_RGB)

typedef struct
{
    int intens;
    int pos;
    int pos2;
} t_light;

typedef uint32_t grbw_t; // the colour of a single pixel
#define RGBW(r, g, b, w) ((((grbw_t)(g) & 0xFF) << 24) | (((grbw_t)(b) & 0xFF) << 8) | (((grbw_t)(r) & 0xFF) << 16) | ((grbw_t)(w) & 0xFF))

#define RGB(r, g, b) ((((grbw_t)(g) & 0xFF) << 24) | (((grbw_t)(b) & 0xFF) << 8) | (((grbw_t)(r) & 0xFF) << 16) | ((grbw_t)(0) & 0xFF))

// device has default bus address of 0x50
#define ADDR _u(0x50)

extern bool lowvolt;
extern uint32_t volts;

void core1_main(void);
void CORE1_Debug(void);

#endif