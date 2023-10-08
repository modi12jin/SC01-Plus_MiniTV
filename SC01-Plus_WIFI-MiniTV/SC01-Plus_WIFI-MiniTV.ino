#include "LovyanGFX_Driver.h"
LGFX tft;

void setup() {
  // put your setup code here, to run once:
  tft.init();
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // put your main code here, to run repeatedly:
}
