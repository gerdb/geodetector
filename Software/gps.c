/**
 sebulli.com
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "global.h"
#include "gps.h"

// Define uart properties for the GPS
const uint32_t GPS_BAUDRATE = 9600;
const uint8_t GPS_TX = 0, GPS_RX = 1;
const uint8_t DATABITS = 8;
const uint8_t STARTBITS = 1;
const uint8_t STOPBITS = 1;

// Calculate the delay between bytes
const uint8_t BITS_PER_BYTE = STARTBITS + DATABITS + STOPBITS;
const uint32_t MICROSECONDS_PER_SECOND = 1000000;
const uint32_t GPS_DELAY = BITS_PER_BYTE * MICROSECONDS_PER_SECOND / GPS_BAUDRATE;
enum GPS_MSG
{
    NONE = 0,
    RMC = 1,
    GGA = 2
};

t_target target =
    {
        0xDEADBEEF,
        0.0f,
        0.0f,
        TARGET_NONE};

int fix = 0;
int fix_old = 0;

bool active = false;
float north = 0.0f;
float east = 0.0f;
float speed = 0.0f;
float dir_move = 0.0f;
float dir_target = 0.0f;
float dir_rel = 0.0f;
int dir_rel24 = 0;
int quality = 0;
float fact_longitude = 1.0f;
float rel_dir = 0.0f;
float distance = 0.0f;
float distance_log = 0.0f;
int distance_i = 0;
int good_i = 0;
int good_i_latch = 0;

int year = 0;
int month = 0;
int day = 0;
int sats = 0;
int sunrise = 0;
int sunset = 0;
int daytime = 0;
int julian_date = 0;

bool ghosttime = false;
bool fairytime = false;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char message[256];
uart_inst_t *gps_uart = uart0;

bool gps_read(uart_inst_t *uart, char *buf, const size_t max_length)
{
    bool retval = false;
    static int buf_i = 0;
    static int pos = 0;
    static enum GPS_MSG gps_msg = NONE;

    if (uart_is_readable(uart))
    {
        char c = uart_getc(uart);
        if (c == '$')
        {
            buf_i = 0;
            pos = 0;
            gps_msg = NONE;
        }
        else if (c == ',')
        {
            buf[buf_i] = '\0';
            buf_i = 0;
            pos++;
            if (pos == 1)
            {
                if (buf[0] == 'G' && buf[1] == 'P')
                {
                    if (buf[2] == 'R' && buf[3] == 'M' && buf[4] == 'C')
                    {
                        gps_msg = RMC;
                    }
                    if (buf[2] == 'G' && buf[3] == 'G' && buf[4] == 'A')
                    {
                        gps_msg = GGA;
                    }
                }
            }

            if (gps_msg == RMC)
            {
                if (pos == 2)
                {
                    int t = (int)atof(buf);
                    daytime = 3600 * (t / 10000);
                    daytime += 60 * ((t % 10000) / 100);
                    daytime += t % 100;
                }
                if (pos == 3)
                {
                    active = buf[0] == 'A';
                }
                if (pos == 4)
                {
                    north = atof(buf) * 0.01f;
                }
                if (pos == 5)
                {
                    if (buf[0] == 'S')
                    {
                        north = -north;
                    }
                }
                if (pos == 6)
                {
                    east = atof(buf) * 0.01f;
                }
                if (pos == 7)
                {
                    if (buf[0] == 'W')
                    {
                        east = -east;
                    }
                }
                if (pos == 8)
                {
                    speed = atof(buf);
                }
                if (pos == 9)
                {
                    dir_move = atof(buf);
                }
                if (pos == 10)
                {
                    int date = atoi(buf);
                    day = date / 10000;
                    month = (date % 10000) / 100;
                    year = date % 100;
                    retval = true;
                }
            }

            if (gps_msg == GGA)
            {
                if (pos == 7)
                {
                    quality = atoi(buf);
                    if (quality == 2)
                    {
                        fix = 3;
                    }
                    else if (quality == 0)
                    {
                        fix = 0;
                    }
                    else
                    {
                        fix = 2;
                    }
                }
                if (pos == 8)
                {
                    sats = atoi(buf);
                }
            }
        }
        else if (buf_i < max_length - 1)
        {
            buf[buf_i] = c;
            buf_i++;
        }
    }
    return retval;
}

int getJulianDate()
{
    // (days since Jan 1, 2000 + 2451545):
    if ((year % 4) == 0)
    {
        months[1] = 29;
    }
    int monthdays = 0;
    for (int i = 0; i < (month - 1); i++)
    {
        monthdays += months[i];
    }

    int julian = day - 1 + monthdays + year * 365 + year / 4 + 1 + 7168 + 2451545;

    return julian;
}

int getSecs(double julian)
{
    double d = julian - 2440587.5;
    double i = (int)d;
    return (int)((d - (double)i) * 86400.0);
}

void Calculate_Local()
{
    fact_longitude = cosf(DEG2RAD * north);

    static double _const_9 = 0.0009;
    static double _const_69 = 0.0069;
    static double _const_53 = 0.0053;
    double lw = -east;
    double ln = north;
    julian_date = getJulianDate();
    double Jdate = (double)julian_date;
    double n_s = (Jdate - JULIAN_DAY_0 - _const_9) - (lw / 360.0);
    int n = round(n_s);

    double J_s = JULIAN_DAY_0 + _const_9 + (lw / 360.0) + n;
    double M = fmod((357.5291 + 0.98560028 * (J_s - JULIAN_DAY_0)), 360.0);
    double C = (1.9148 * sin(DEG2RAD * M)) + (0.0200 * sin(DEG2RAD * 2 * M)) + (0.0003 * sin(DEG2RAD * 3 * M));
    double lam = fmod((M + 102.9372 + C + 180), 360);
    double Jtransit = J_s + (_const_53 * sin(DEG2RAD * M)) - (_const_69 * sin(DEG2RAD * 2 * lam));
    for (int i = 0; i < 4; i++)
    {
        M = fmod((357.5291 + 0.98560028 * (Jtransit - JULIAN_DAY_0)), 360.0);
        Jtransit = J_s + (_const_53 * sin(2 * M_PI / 360 * M)) - (_const_69 * sin(2 * lam));
        C = (1.9148 * sin(DEG2RAD * M)) + (0.0200 * sin(DEG2RAD * 2 * M)) + (0.0003 * sin(DEG2RAD * 3 * M));
        lam = fmod((M + 102.9372 + C + 180), 360);
        Jtransit = J_s + (_const_53 * sin(DEG2RAD * M)) - (_const_69 * sin(DEG2RAD * 2 * lam));
    }
    double rho = RAD2DEG * asin(sin(DEG2RAD * lam) * sin(DEG2RAD * 23.45));
    double H = RAD2DEG * acos((sin(DEG2RAD * (-0.83)) - sin(DEG2RAD * ln) * sin(DEG2RAD * rho)) / (cos(DEG2RAD * ln) * cos(DEG2RAD * rho)));
    double Jss = JULIAN_DAY_0 + _const_9 + ((H + lw) / 360) + n;
    double Jset = Jss + (_const_53 * sin(DEG2RAD * M)) - (_const_69 * sin(DEG2RAD * 2 * lam));
    double Jrise = Jtransit - (Jset - Jtransit);

    sunset = getSecs(Jset);
    sunrise = getSecs(Jrise);
}

float fasterlog2(float x)
{
    union
    {
        float f;
        uint32_t i;
    } vx = {x};
    float y = vx.i;
    y *= 1.0 / (1 << 23);
    return y - 126.94269504f;
}

float fasterlog(float x)
{
    return 0.69314718f * fasterlog2(x);
}
float fasterlog10(float x)
{
    return 0.30102999f * fasterlog2(x);
}

void Calc()
{
    float d_lat = target.vals.latitude - north;
    float d_lon = (target.vals.longitude - east) * fact_longitude;
    // 1deg = 111km
    distance = sqrtf(d_lon * d_lon + d_lat * d_lat) * 111000.f;
    distance_log = fasterlog10(distance * 0.33f);
    float tmp = distance_log * 1365.f;
    if (tmp < 0.0f)
    {
        tmp = 0.0f;
    }
    if (tmp > 4095.0f)
    {
        tmp = 4095.0f;
    }
    distance_i = (int)tmp;
    if (distance > 0.1f)
    {
        dir_target = RAD2DEG * (atan2f(d_lat, d_lon));
        dir_rel = dir_target - dir_move;
        if (dir_rel < -180.0f)
        {
            dir_rel += 360.0f;
        }

        // distance dependent good value
        good_i = 4095 - distance_i;

        // minimum 1
        if (good_i < 1)
        {
            good_i = 1;
        }

        // -180° ... 180° -> 0°.. 180°
        // 0° ..  +-180° -> 90° .. -90°

        float bad_f;

        if (dir_rel >= 0.f)
        {
            bad_f = dir_rel;
        }
        else
        {
            bad_f = -dir_rel;
        }

        bad_f -= 45.f;

        //  0 ... +-45° -> 0°,  45°..90° -> 0.. 45°
        if (bad_f < 0.f)
        {
            bad_f = 0.f;
        }

        if (bad_f > 45.f)
        {
            bad_f = 45.f;
        }

        //  0 ... +-45° -> 0.. 100% (4095)
        bad_f *= 91.f;
        int bad_i = (int)(bad_f);
        if (bad_i <= 1)
        {
            bad_i = 0;
        }

        good_i -= bad_i;

        // deg / 360 * 24 + 11,5 LEDs
        dir_rel24 = ((dir_rel + 172.5f) * 0.06666f);
        if (dir_rel24 < 0)
        {
            dir_rel24 += 24;
        }
        dir_rel24 %= 24;
    }

    // Ghostime 1h after sunset until 1 hour before sunrise
    ghosttime = daytime > (sunset + 3600) || daytime < (sunrise - 3600);
    // Fairytime: half an hour before sunrise until 2 hours after sunrise
    fairytime = daytime > (sunrise - 1800) && daytime < (sunrise + 7200);
}

float Rel_Dir(float lon1, float lon2, float lat1, float lat2, float dir)
{
    float d_lon = lon2 - lon1;
    float d_lat = (lat2 - lat1) * fact_longitude;
}

void GPS_Init(void)
{
    uart_init(gps_uart, GPS_BAUDRATE);

    // Don't convert \n to \r\n
    uart_set_translate_crlf(gps_uart, false);

    // Enable the uart functionality for the pins connected to the GPS
    gpio_set_function(GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX, GPIO_FUNC_UART);
}

void GPS_Read(void)
{
    // Read a line from the GPS data
    if (gps_read(gps_uart, message, sizeof(message)))
    {
        // first fix?
        if (fix >= 2 && (fix != fix_old))
        {
            Calculate_Local();
        }

        fix_old = fix;

        Calc();
    }
}

void GPS_Debug(void)
{
    printf("a:%d n:%4.4f e:%5.4f s:%3.2f dm:%3.2f q:%d sats:%d fix:%d\n", active, north, east, speed, dir_move, quality, sats, fix);
    printf("f:%5.4f dist:%5.4f dist_log:%5.4f dist_i:%d dt:%5.4f dr:%5.4f\n", fact_longitude, distance, distance_log, distance_i, dir_target, dir_rel);
    printf("dr:%5.4f good_i:%d\n", dir_rel, good_i);
    printf("sunset:%d sunrise:%d time:%d\n", sunset, sunrise, daytime);
}
