/**
 sebulli.com
 */

#ifndef AUDIO_H
#define AUDIO_H

#define SAMPLES_PER_BUFFER 256

typedef struct
{
    int len;
    int pos;
    bool play;
    bool accented;
    const int16_t *wav_data;
} t_channel;

extern uint32_t vco1cnt;
extern t_channel channel[5];

void AUDIO_Fill_Buffer(void);
void AUDIO_Init(void);

#endif