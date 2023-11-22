#pragma GCC push_options
#pragma GCC optimize ("O3")

/***
 * Required libraries:
 * https://github.com/lovyan03/LovyanGFX.git
 * https://github.com/bitbank2/JPEGDEC.git
 */
#include <WiFi.h>
#include "LovyanGFX_Driver.h"
#include "VideoPlayer.h"
#include "src/audio_output/I2SOutput.h"
#include "src/ChannelData/NetworkChannelData.h"
#include "src/AudioSource/NetworkAudioSource.h"
#include "src/VideoSource/NetworkVideoSource.h"
#include "src/AVIParser/AVIParser.h"

const char *WIFI_SSID = "MERCURY_2FF2";
const char *WIFI_PASSWORD = "1234567890";
const char *FRAME_URL = "http://192.168.0.107:8123/frame";
const char *AUDIO_URL = "http://192.168.0.107:8123/audio";
const char *CHANNEL_INFO_URL = "http://192.168.0.107:8123/channel_info";

VideoSource *videoSource = NULL;
AudioSource *audioSource = NULL;
VideoPlayer *videoPlayer = NULL;
AudioOutput *audioOutput = NULL;
ChannelData *channelData = NULL;
LGFX tft;

#define I2S_DOUT 37
#define I2S_BCLK 36
#define I2S_LRC 35

void setup()
{
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // 禁用WiFi省电以提高速度
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.println("");
  Serial.println("WiFi connected");
  
  channelData = new NetworkChannelData(CHANNEL_INFO_URL, FRAME_URL, AUDIO_URL);
  videoSource = new NetworkVideoSource((NetworkChannelData *)channelData);
  audioSource = new NetworkAudioSource((NetworkChannelData *)channelData);

  tft.init();
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(2);

  // I2S扬声器引脚
  i2s_pin_config_t i2s_speaker_pins = {
    .mck_io_num = -1,
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = -1,
  };

  audioOutput = new I2SOutput(I2S_NUM_0, i2s_speaker_pins);
  audioOutput->start(16000);
  
  videoPlayer = new VideoPlayer(
      channelData,
      videoSource,
      audioSource,
      &tft,
      audioOutput);
  videoPlayer->start();
  videoPlayer->play();
}

void loop()
{
}
