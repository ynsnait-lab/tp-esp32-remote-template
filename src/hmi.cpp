#include <Arduino.h>
#include "hmi.h"
#include "pins.h"

namespace {
  const int      BUZZER_CH   = 0;     // canal LEDC
  const uint32_t BUZZER_FREQ = 4000;  // 4 kHz : resonance du buzzer Murata PKLCS
  const uint8_t  BUZZER_RES  = 8;     // resolution 8 bits
}

namespace hmi {

void begin() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED,   LOW);
  ledcSetup(BUZZER_CH, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(PIN_BUZZER_PWM, BUZZER_CH);
  ledcWrite(BUZZER_CH, 0);
}

void ledGreen(bool on) { digitalWrite(PIN_LED_GREEN, on ? HIGH : LOW); }
void ledRed(bool on)   { digitalWrite(PIN_LED_RED,   on ? HIGH : LOW); }

void beep(uint16_t ms) {
  ledcWrite(BUZZER_CH, 128);  // rapport cyclique 50 %
  delay(ms);
  ledcWrite(BUZZER_CH, 0);
}

void selfTest() {
  ledGreen(true);  delay(300); ledGreen(false);
  ledRed(true);    delay(300); ledRed(false);
  beep(150);
}

} // namespace hmi
