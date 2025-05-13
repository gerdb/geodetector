/**
 sebulli.com
 */

#ifndef GPS_H
#define GPS_H

#include "pico/stdlib.h"

#define JULIAN_DAY_0 2451545
#define DEG2RAD (2 * M_PI / 360.0)
#define RAD2DEG (360.0 / 2 / M_PI)

extern int fix;
extern float north;
extern float east;
extern float speed;
extern int dir_rel24;
extern t_target target;
extern int good_i;
extern int good_i_latch;
extern bool ghosttime;

bool gps_read(uart_inst_t *uart, char *buf, const size_t max_length);
void GPS_Init(void);
void GPS_Read(void);
void GPS_Debug(void);

#endif