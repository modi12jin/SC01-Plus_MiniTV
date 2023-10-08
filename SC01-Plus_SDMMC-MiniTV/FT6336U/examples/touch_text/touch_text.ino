#include "FT6336U.h"

#define I2C_SDA 6
#define I2C_SCL 5
#define TP_INT 7
#define TP_RST 4

FT6336U touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);

bool FingerNum;
bool state;
uint8_t gesture;
uint16_t touchX, touchY;

void setup() {
  Serial.begin(115200);
  touch.begin();
}

void loop() {

  FingerNum = touch.getTouch(&touchX, &touchY);
  state =touch.getGesture(&gesture);
  if (state) {
    Serial.printf("gesture:%x\n", gesture);
  }
  if (FingerNum) {
    Serial.printf("X:%d,Y:%d\n", touchX, touchY);
  }

  delay(200);
}