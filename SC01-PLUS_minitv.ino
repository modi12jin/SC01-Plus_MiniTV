/***
 * Required libraries:
 * https://github.com/lovyan03/LovyanGFX.git
 * https://github.com/pschatzmann/arduino-libhelix.git
 * https://github.com/bitbank2/JPEGDEC.git
 */
#define FPS 20
#define MJPEG_BUFFER_SIZE (480 * 270 * 2 / 10) //MJPEG缓冲区大小
#define AUDIOASSIGNCORE 1  //音频分配核心
#define DECODEASSIGNCORE 1 //解码分配核心
#define DRAWASSIGNCORE 0 //绘图分配核心

#include <WiFi.h>
#include <FS.h>
#include <SD_MMC.h>
#include <Preferences.h>

Preferences preferences;
#define APP_NAME "video_player"
#define K_VIDEO_INDEX "video_index"
#define BASE_PATH "/Videos/" //基本路径
#define AAC_FILENAME "/44100.aac" //AAC文件名
#define MJPEG_FILENAME "/480_20fps.mjpeg" //MJPEG文件名
#define VIDEO_COUNT 20 //视频数量

#define SD_CS 41
#define SDMMC_CMD 40
#define SDMMC_CLK 39
#define SDMMC_D0 38

#define I2S_DOUT 37
#define I2S_BCLK 36
#define I2S_LRC 35

#define I2C_SDA 6
#define I2C_SCL 5
#define TP_INT 7
#define TP_RST -1

/* Audio */
#include "esp32_audio_task.h"

/* MJPEG Video */
#include "mjpeg_decode_draw_task.h"

/* LCD */
#include "LovyanGFX_Driver.h"
LGFX tft;

/* Touch */
#include "FT6336U.h"
FT6336U touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);

/* Variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned int video_idx = 1;
static bool BacklightChange = true;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw) {
  unsigned long s = millis();
  if (tft.getStartCount() > 0) {
    tft.startWrite();
  }
  tft.pushImageDMA(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, (lgfx::rgb565_t *)pDraw->pPixels);
  if (tft.getStartCount() > 0) {
    tft.endWrite();
  }
  total_show_video_ms += millis() - s;
  return 1;
} /* drawMCU() */

void setup() {
  disableCore0WDT();

  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);

  // Init Display
  tft.init();
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);

  xTaskCreate(
    touchTask,
    "touchTask",
    2000,
    NULL,
    1,
    NULL);

  Serial.println("Init I2S");

  esp_err_t ret_val = i2s_init(I2S_NUM_0, 44100, -1, I2S_BCLK, I2S_LRC, I2S_DOUT, -1);
  if (ret_val != ESP_OK) {
    Serial.printf("i2s_init failed: %d\n", ret_val);
    tft.println("i2s_init failed");
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  setVolume(0.1);

  Serial.println("Init FS");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
  if (!SD_MMC.begin("/root", true)) /* 1-bit SD bus mode */
  {
    Serial.println("ERROR: File system mount failed!");
    tft.println("ERROR: File system mount failed!");
    return;
  }

  preferences.begin(APP_NAME, false);
  video_idx = preferences.getUInt(K_VIDEO_INDEX, 1);
  Serial.printf("videoIndex: %d\n", video_idx);

  tft.setCursor(20, 30);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.printf("CH %d", video_idx);
  delay(1000);

  playVideoWithAudio(video_idx);
}

void loop() {
}

void touchTask(void *parameter) {
  touch.begin();
  bool state;
  uint8_t gesture;
  while (1) {
    state = touch.getGesture(&gesture);
    if (state) {
      switch (gesture) {
        case DoubleTap:
          BacklightControl();
          break;
        case SlideUp:
          Serial.println("SlideUp(RIGHT)");
          videoController(-1);
          break;
        case SlideDown:
          Serial.println("SlideDown(LEFT)");
          videoController(1);
          break;
        case SlideLeft:
          setVolume(0.1);
          break;
        case SlideRight:
          setVolume(0.5);
          break;
      }
    }
    vTaskDelay(1000);
  }
}

void BacklightControl() {
  if (BacklightChange) {
    tft.setBrightness(255);
    BacklightChange = false;
  } else {
    tft.setBrightness(50);
    BacklightChange = true;
  }
}

void playVideoWithAudio(int channel) {

  char aFilePath[40];
  sprintf(aFilePath, "%s%d%s", BASE_PATH, channel, AAC_FILENAME);

  File aFile = SD_MMC.open(aFilePath);
  if (!aFile || aFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    tft.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    return;
  }

  char vFilePath[40];
  sprintf(vFilePath, "%s%d%s", BASE_PATH, channel, MJPEG_FILENAME);

  File vFile = SD_MMC.open(vFilePath);
  if (!vFile || vFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    tft.printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    return;
  }

  Serial.println("Init video");

  mjpeg_setup(&vFile, MJPEG_BUFFER_SIZE, drawMCU, false, DECODEASSIGNCORE, DRAWASSIGNCORE);

  Serial.println("Start play audio task");

  BaseType_t ret = aac_player_task_start(&aFile, AUDIOASSIGNCORE);

  if (ret != pdPASS) {
    Serial.printf("Audio player task start failed: %d\n", ret);
    tft.printf("Audio player task start failed: %d\n", ret);
  }

  Serial.println("Start play video");

  start_ms = millis();
  curr_ms = millis();
  next_frame_ms = start_ms + (++next_frame * 1000 / FPS / 2);
  while (vFile.available() && mjpeg_read_frame())  // Read video
  {
    total_read_video_ms += millis() - curr_ms;
    curr_ms = millis();

    if (millis() < next_frame_ms)  // check show frame or skip frame
    {
      // Play video
      mjpeg_draw_frame();
      total_decode_video_ms += millis() - curr_ms;
      curr_ms = millis();
    } else {
      ++skipped_frames;
      //Serial.println("Skip frame");
    }

    while (millis() < next_frame_ms) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    curr_ms = millis();
    next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
  }
  int time_used = millis() - start_ms;
  int total_frames = next_frame - 1;
  Serial.println("AV end");
  vFile.close();
  aFile.close();

  videoController(1);
}

void videoController(int next) {

  video_idx += next;
  if (video_idx <= 0) {
    video_idx = VIDEO_COUNT;
  } else if (video_idx > VIDEO_COUNT) {
    video_idx = 1;
  }
  Serial.printf("video_idx : %d\n", video_idx);
  preferences.putUInt(K_VIDEO_INDEX, video_idx);
  preferences.end();
  esp_restart();
}