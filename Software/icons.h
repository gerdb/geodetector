/**
 sebulli.com
 */

#ifndef ICONS_H
#define ICONS_H

#define SERVO_OFFSET (-100) //-50
#define SERVO_60_DEG (630)  // 666

typedef enum
{
    WAIT = 0,
    TREASURE = 1,
    GHOST = 2
} Icons;

void ICONS_Init();
void ICONS_Select(Icons icon);
void ICONS_Task1ms();

#endif