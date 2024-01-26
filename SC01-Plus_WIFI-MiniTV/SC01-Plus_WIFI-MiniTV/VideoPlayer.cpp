#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "VideoPlayer.h"
#include "src/audio_output/AudioOutput.h"
#include "src/ChannelData/ChannelData.h"
#include "src/VideoSource/VideoSource.h"
#include "src/AudioSource/AudioSource.h"
#include <list>

void VideoPlayer::_framePlayerTask(void *param)
{
  VideoPlayer *player = (VideoPlayer *)param;
  player->framePlayerTask();
}

void VideoPlayer::_audioPlayerTask(void *param)
{
  VideoPlayer *player = (VideoPlayer *)param;
  player->audioPlayerTask();
}

VideoPlayer::VideoPlayer(ChannelData *channelData, VideoSource *videoSource, AudioSource *audioSource, LovyanGFX *display, AudioOutput *audioOutput)
    : mChannelData(channelData), mVideoSource(videoSource), mAudioSource(audioSource), mDisplay(display), mState(VideoPlayerState::STOPPED), mAudioOutput(audioOutput)
{
}

void VideoPlayer::start()
{
  mVideoSource->start();
  mAudioSource->start();
  // 启动帧播放器任务
  xTaskCreatePinnedToCore(
      _framePlayerTask,
      "Frame Player",
      10000,
      this,
      1,
      NULL,
      1);
  xTaskCreatePinnedToCore(_audioPlayerTask, "audio_loop", 10000, this, 1, NULL, 0);
}

void VideoPlayer::setChannel(int channel)
{
  mChannelData->setChannel(channel);
  // 将音频样本设置为 0 - TODO - 将其移到其他地方？
  mCurrentAudioSample = 0;
  mChannelVisible = millis();
  // 更新视频源
  mVideoSource->setChannel(channel);
}

void VideoPlayer::play()
{
  if (mState == VideoPlayerState::PLAYING)
  {
    return;
  }
  mState = VideoPlayerState::PLAYING;
  mVideoSource->setState(VideoPlayerState::PLAYING);
  mCurrentAudioSample = 0;
}

void VideoPlayer::stop()
{
  if (mState == VideoPlayerState::STOPPED)
  {
    return;
  }
  mState = VideoPlayerState::STOPPED;
  mVideoSource->setState(VideoPlayerState::STOPPED);
  mCurrentAudioSample = 0;
  mDisplay->fillScreen(TFT_BLACK);
}

void VideoPlayer::pause()
{
  if (mState == VideoPlayerState::PAUSED)
  {
    return;
  }
  mState = VideoPlayerState::PAUSED;
  mVideoSource->setState(VideoPlayerState::PAUSED);
}

void VideoPlayer::playStatic()
{
  if (mState == VideoPlayerState::STATIC)
  {
    return;
  }
  mState = VideoPlayerState::STATIC;
  mVideoSource->setState(VideoPlayerState::STATIC);
}

// 双缓冲 DMA 绘图，否则我们会损坏
uint16_t *dmaBuffer[2] = {NULL, NULL};
int dmaBufferIndex = 0;
int _doDraw(JPEGDRAW *pDraw)
{
  VideoPlayer *player = (VideoPlayer *)pDraw->pUser;
  int numPixels = pDraw->iWidth * pDraw->iHeight;
  if (dmaBuffer[dmaBufferIndex] == NULL)
  {
    dmaBuffer[dmaBufferIndex] = (uint16_t *)malloc(numPixels * 2);
  }
  memcpy(dmaBuffer[dmaBufferIndex], pDraw->pPixels, numPixels * 2);
  LovyanGFX *tft = player->mDisplay;
  tft->waitDMA();
  tft->setAddrWindow(pDraw->x, pDraw->y+24, pDraw->iWidth, pDraw->iHeight);
  tft->pushPixelsDMA(dmaBuffer[dmaBufferIndex], numPixels);
  dmaBufferIndex = (dmaBufferIndex + 1) % 2;
  return 1;
}

static unsigned short x = 12345, y = 6789, z = 42, w = 1729;

unsigned short xorshift16()
{
  unsigned short t = x ^ (x << 5);
  x = y;
  y = z;
  z = w;
  w = w ^ (w >> 1) ^ t ^ (t >> 3);
  return w & 0xFFFF;
}

void VideoPlayer::framePlayerTask()
{
  uint16_t *staticBuffer = NULL;
  uint8_t *jpegBuffer = NULL;
  size_t jpegBufferLength = 0;
  size_t jpegLength = 0;
  // 用于计算帧率
  std::list<int> frameTimes;
  while (true)
  {
    if (mState == VideoPlayerState::STOPPED || mState == VideoPlayerState::PAUSED)
    {
      // 无事可做——只需等待
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    if (mState == VideoPlayerState::STATIC)
    {
      // 将随机像素绘制到屏幕上以模拟静态
      // 我们将一次处理 8 行像素以节省 RAM
      int width = mDisplay->width();
      int height = 8;
      if (staticBuffer == NULL)
      {
        staticBuffer = (uint16_t *)malloc(width * height * 2);
      }
      for (int i = 0; i < mDisplay->height(); i++)
      {
        for (int p = 0; p < width * height; p++)
        {
          int grey = xorshift16() >> 8;
          staticBuffer[p] = mDisplay->color565(grey, grey, grey);
        }
        mDisplay->waitDMA();
        mDisplay->pushPixelsDMA(staticBuffer, width * height);
        mDisplay->endWrite();
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
    // 获取下一帧
    if (!mVideoSource->getVideoFrame(&jpegBuffer, jpegBufferLength, jpegLength))
    {
      // 尚未准备好框架
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }
    frameTimes.push_back(millis());
    // 将帧速率运行时间保持在5秒
    while (frameTimes.size() > 0 && frameTimes.back() - frameTimes.front() > 5000)
    {
      frameTimes.pop_front();
    }
    mDisplay->startWrite();
    if (mJpeg.openRAM(jpegBuffer, jpegLength, _doDraw))
    {
      mJpeg.setUserPointer(this);
      mJpeg.setPixelType(RGB565_BIG_ENDIAN);
      mJpeg.decode(0, 0, 0);
      mJpeg.close();
    }

#if CORE_DEBUG_LEVEL > 0
    // 显示频道指示器
    if (millis() - mChannelVisible < 2000)
    {
      mDisplay->setCursor(20, 20);
      mDisplay->setTextColor(TFT_GREEN, TFT_BLACK);
      mDisplay->printf("%d", mChannelData->getChannelNumber());
    }
    // 在右上角显示帧速率
    mDisplay->setCursor(mDisplay->width() - 50, 20);
    mDisplay->setTextColor(TFT_GREEN, TFT_BLACK);
    mDisplay->printf("%d", frameTimes.size() / 5);
    mDisplay->endWrite();
#endif
  }
}

void VideoPlayer::audioPlayerTask()
{
  size_t bufferLength = 16000;
  int8_t *audioData = (int8_t *)malloc(16000);
  while (true)
  {
    if (mState != VideoPlayerState::PLAYING)
    {
      // 无事可做——只需等待
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    // 获取要播放的音频数据
    int audioLength = mAudioSource->getAudioSamples(&audioData, bufferLength, mCurrentAudioSample);
    // 我们已经到达通道的尽头了吗？
    if (audioLength == 0)
    {
      // 我们想要循环播放视频，因此重置频道数据并重新开始
      mChannelData->setChannel(mChannelData->getChannelNumber());
      mCurrentAudioSample = 0;
      mVideoSource->updateAudioTime(0);
      continue;
    }
    if (audioLength > 0)
    {
      // 播放音频
      for (int i = 0; i < audioLength; i += 1000)
      {
        mAudioOutput->write(audioData + i, min(1000, audioLength - i));
        mCurrentAudioSample += min(1000, audioLength - i);
        if (mState != VideoPlayerState::PLAYING)
        {
          mCurrentAudioSample = 0;
          mVideoSource->updateAudioTime(0);
          break;
        }
        mVideoSource->updateAudioTime(1000 * mCurrentAudioSample / 16000);
      }
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}
