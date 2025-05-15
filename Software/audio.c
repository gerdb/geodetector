/**
 sebulli.com
 */

#include <stdio.h>
#include <stdint.h>

#include "hardware/pwm.h"
#include "global.h"
#include "gps.h"
#include "copro.h"
#include "audio.h"

#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#include "sound/sin.h"

#include "sound/330441529.wav.h"
#include "sound/ring.wav.h"
#include "sound/ringring.wav.h"
#include "sound/invalid.wav.h"
#include "sound/beepbeep.wav.h"

struct audio_buffer_pool *ap;
int32_t vco1_a = 0;
uint32_t vco7cnt = 0;
uint32_t vco1cnt = (SIN_LENGTH * 12288);
uint32_t vcoacntold = 0;
int32_t vcoapause = 0;
int pause = 1;
uint32_t vcoapausecnt = 0;
uint32_t fm_cnt = 0;
t_channel channel[5];

bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S LRCK"));

struct audio_buffer_pool *init_audio()
{

   static audio_format_t audio_format = {
       .format = AUDIO_BUFFER_FORMAT_PCM_S16,
       .sample_freq = 44100,
       .channel_count = 1,
   };

   static struct audio_buffer_format producer_format = {
       .format = &audio_format,
       .sample_stride = 2};

   struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                     SAMPLES_PER_BUFFER); // todo correct size
   bool __unused ok;
   const struct audio_format *output_format;

   struct audio_i2s_config config = {
       .data_pin = PICO_AUDIO_I2S_DATA_PIN,
       .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
       .dma_channel = 0,
       .pio_sm = 0,
   };

   output_format = audio_i2s_setup(&audio_format, &config);
   if (!output_format)
   {
      panic("PicoAudio: Unable to open audio device.\n");
   }

   ok = audio_i2s_connect(producer_pool);
   assert(ok);
   audio_i2s_set_enabled(true);
   return producer_pool;
}

void AUDIO_Init(void)
{

   // Initialize all channels
   for (int i = 0; i < 4; i++)
   {
      channel[i].pos = 0;
      channel[i].play = false;
   }

   // Assign wav length
   channel[0].len = WAV_LENGTH_330441529;
   channel[1].len = WAV_LENGTH_RING;
   channel[2].len = WAV_LENGTH_RINGRING;
   channel[3].len = WAV_LENGTH_INVALID;
   channel[4].len = WAV_LENGTH_BEEPBEEP;
   // Assign wav
   channel[0].wav_data = WAV_DATA_330441529;
   channel[1].wav_data = WAV_DATA_RING;
   channel[2].wav_data = WAV_DATA_RINGRING;
   channel[3].wav_data = WAV_DATA_INVALID;
   channel[4].wav_data = WAV_DATA_BEEPBEEP;

   ap = init_audio();
}

void AUDIO_Fill_Buffer(void)
{
   // Output the sound
   struct audio_buffer *buffer = take_audio_buffer(ap, false);
   if (buffer)
   {
      int16_t *samples = (int16_t *)buffer->buffer->bytes;

      for (uint i = 0; i < buffer->max_sample_count; i++)
      {
         int mixer = 0;

         // vco1_a = 4096+ 32768 + 7*SIN[vco1cnt/16384]/8;
         vco1_a = 32768 + SIN[vco1cnt / 16384];

         int i0 = fm_cnt / 65536;
         int i1 = (i0 + 1) % 2000;
         int m1 = fm_cnt % 65536;
         int m0 = 65535 - m1;
         int32_t p0 = channel[0].wav_data[i0];
         int32_t p1 = channel[0].wav_data[i1];

         int p = (p0 * m0 + p1 * m1) / 65536;

         mixer += (vco1_a * p) / 32768;

         int32_t vco7_f = SIN[vco7cnt / 16384];
         int32_t vco1_f = SIN[vco1cnt / 16384];

         int vco = good_i_latch;

         fm_cnt += 49152 + vco * 8 + vco7_f / 32 + vco1_f / 32;

         if ((fm_cnt) >= 2000 * 65536)
         {
            fm_cnt -= 2000 * 65536;
         }

         // vco1cnt += 1024;
         if (pause != 2)
         {
            vco1cnt += 1024;
         }
         // going over the lowest point of sin (+270°)
         if ((vco1cnt >= (SIN_LENGTH * 12288)) && (vcoacntold < (SIN_LENGTH * 12288)))
         {

            pause = 2;
         }

         if (pause == 2)
         {
            good_i_latch = good_i;
            vcoapause = (1023 - vco) * 200;
            if (vcoapause < 0)
            {
               vcoapause = 0;
            }
            if (good_i > 0)
            {
               vcoapausecnt++;
            }

            if (vcoapausecnt > vcoapause)
            {
               vcoapausecnt = 0;
               if (target.vals.treasure_type == 2 && !ghosttime)
               {
                  // not ghosttime. We stay silent
               }
               else if (fix >= 2 && target.vals.treasure_type != 0 && volts > 1000)
               {

                  pause = 0;
               }
            }
         }

         vcoacntold = vco1cnt;

         if ((vco1cnt) >= (SIN_LENGTH * 16384))
         {
            vco1cnt -= SIN_LENGTH * 16384;
         }

         vco7cnt += 7 * 1024;
         if ((vco7cnt) >= (SIN_LENGTH * 16384))
         {
            vco7cnt -= SIN_LENGTH * 16384;
         }

         // mix all channels
         for (int i = 1; i < 5; i++)
         {

            // Channel playing?
            if (channel[i].play)
            {
               mixer += channel[i].wav_data[channel[i].pos];

               // next sample
               channel[i].pos++;
               // Stop at the end
               if (channel[i].pos >= channel[i].len)
               {
                  channel[i].pos = 0;
                  channel[i].play = false;
               }
            }
         }

         // volume output
         mixer = 1 * mixer / 8;

         // limit it
         if (mixer < -32768)
         {
            mixer = -32768;
         }
         if (mixer > 32767)
         {
            mixer = 32767;
         }

         samples[i] = (int16_t)mixer;
      }
      buffer->sample_count = buffer->max_sample_count;
      give_audio_buffer(ap, buffer);
   }
}
