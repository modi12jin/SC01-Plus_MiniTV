#pragma GCC push_options
#pragma GCC optimize("O3")

/***
 * Required libraries:
 * https://github.com/lovyan03/LovyanGFX.git
 * https://github.com/bitbank2/JPEGDEC.git
 */
#include <WiFi.h>
#include "LovyanGFX_Driver.h"
#include "FT6336U.h"
#include "VideoPlayer.h"
#include "src/audio_output/I2SOutput.h"
#include "src/ChannelData/NetworkChannelData.h"
#include "src/AudioSource/NetworkAudioSource.h"
#include "src/VideoSource/NetworkVideoSource.h"
#include "src/AVIParser/AVIParser.h"

const char *WIFI_SSID = "MERCURY_2FF2";
const char *WIFI_PASSWORD = "1234567890";
const char *FRAME_URL = "http://192.168.0.107:8123/frame";                // 图像网址
const char *AUDIO_URL = "http://192.168.0.107:8123/audio";                // 音频网址
const char *CHANNEL_INFO_URL = "http://192.168.0.107:8123/channel_info";  // 频道信息网址

VideoSource *videoSource = NULL;  // 视频源
AudioSource *audioSource = NULL;  // 音频源
VideoPlayer *videoPlayer = NULL;  // 视频播放器
AudioOutput *audioOutput = NULL;  // 音频输出
ChannelData *channelData = NULL;  // 通道数据

int channel = 0;              // 视频通道
bool BacklightChange = true;  //背光变化
bool state;
uint8_t gesture;
#define VIDEO_COUNT 2 //视频数量

#define I2S_DOUT 37
#define I2S_BCLK 36
#define I2S_LRC 35

#define I2C_SDA 6
#define I2C_SCL 5
#define TP_INT 7
#define TP_RST -1

LGFX tft;
FT6336U touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
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

  touch.begin();

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
  videoPlayer->setChannel(0);//视频通道
  videoPlayer->play();
  audioOutput->setVolume(4);//音量
}

void BacklightControl() {
  if (BacklightChange) {
    tft.setBrightness(255);  //亮度0~255范围
    BacklightChange = false;
  } else {
    tft.setBrightness(50);
    BacklightChange = true;
  }
}

void volumeUp() {
  audioOutput->volumeUp();
  Serial.println("VOLUME_UP");
}

void volumeDown() {
  audioOutput->volumeDown();
  Serial.println("VOLUME_DOWN");
}

void channelDown() {
  videoPlayer->playStatic();
  channel--;
  if (channel < 0) {
    channel = 0;
  }
  videoPlayer->setChannel(channel);
  videoPlayer->play();
  Serial.printf("CHANNEL_DOWN %d\n", channel);
}

void channelUp() {
  videoPlayer->playStatic();
  channel++;
  if(channel > VIDEO_COUNT){
    channel=VIDEO_COUNT;
  }
  videoPlayer->setChannel(channel);
  videoPlayer->play();
  Serial.printf("CHANNEL_UP %d\n", channel);
}

void loop() {
  state = touch.getGesture(&gesture);
  if (state) {
    switch (gesture) {
      case DoubleTap:        //双击
        BacklightControl();  //背光控制
        break;
      case SlideUp:     //向上滑动
        channelDown();  //频道-
        break;
      case SlideDown:  //向下滑动
        channelUp();   //频道+
        break;
      case SlideLeft:  //向左滑动
        volumeDown();  //音量-
        break;
      case SlideRight:  //向右滑动
        volumeUp();     //音量+
        break;
    }
  }
}
