
#include <Arduino.h>
#include "I2SBase.h"
#include <esp_log.h>
#include <driver/i2s.h>

static const char *TAG = "AUDIO";

// number of frames to try and send at once (a frame is a left and right sample)
const size_t NUM_FRAMES_TO_SEND=1024;

I2SBase::I2SBase(i2s_port_t i2s_port) : m_i2s_port(i2s_port)
{
  m_tmp_frames = (int16_t *)malloc(2 * sizeof(int16_t) * NUM_FRAMES_TO_SEND);
}

void I2SBase::stop()
{
  // stop the i2S driver
  i2s_stop(m_i2s_port);
  i2s_driver_uninstall(m_i2s_port);
}

// high pass filter to remove any DC offset
typedef float REAL;
#define NBQ 4
REAL biquada[]={0.9998380079498859,-1.9996254961162483,0.9987226842352888,-1.998510171954402,0.9915562184741177,-1.9913436697006053,-0.9854172919732774};
REAL biquadb[]={0.9999999999999998,-1.9997906555550162,0.9999999999999999,-1.9998054508889258,1,-1.9998831022849304,-1};
REAL gain=1.0123741492041554;
REAL xyv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

REAL applyfilter(REAL v)
{
	int i,b,xp=0,yp=3,bqp=0;
	REAL out=v/gain;
	for (i=14; i>0; i--) {xyv[i]=xyv[i-1];}
	for (b=0; b<NBQ; b++)
	{
		int len=(b==NBQ-1)?1:2;
		xyv[xp]=out;
		for(i=0; i<len; i++) { out+=xyv[xp+len-i]*biquadb[bqp+i]-xyv[yp+len-i]*biquada[bqp+i]; }
		bqp+=len;
		xyv[yp]=out;
		xp=yp; yp+=len+1;
	}
	return out;
}

void I2SBase::write(const uint8_t *samples, int count)
{
  int sample_index = 0;
  while (sample_index < count)
  {
    int samples_to_send = 0;
    for (int i = 0; i < NUM_FRAMES_TO_SEND && sample_index < count; i++)
    {
      float sample = samples[i]/255.0f;
      sample = applyfilter(sample);
      // soft clipping
      sample = tanh(sample);
      // scale up to 16 bits
      sample = sample * 32767.0f;
      // volume adjustment
      sample = sample * mVolume / 10;
      int16_t sample16 = process_sample(sample);
      m_tmp_frames[i * 2] = sample16;
      m_tmp_frames[i * 2 + 1] = sample16;
      samples_to_send++;
      sample_index++;
    }
    // write data to the i2s peripheral
    size_t bytes_written = 0;
    esp_err_t res = i2s_write(m_i2s_port, m_tmp_frames, samples_to_send * sizeof(int16_t) * 2, &bytes_written, 1000 / portTICK_PERIOD_MS);
    if (res != ESP_OK)
    {
      ESP_LOGE(TAG, "Error sending audio data: %d", res);
    }
    if (bytes_written != samples_to_send * sizeof(int16_t) * 2)
    {
      ESP_LOGE(TAG, "Did not write all bytes");
    }
  }
}
