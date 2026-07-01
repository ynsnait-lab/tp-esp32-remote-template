#include <Arduino.h>
#include "ntc.h"
#include "ntc_convert.h"   // conversion pure (testable sur PC)
#include "pins.h"

namespace ntc {

void begin() {
  analogReadResolution(12);          // 0..4095
  pinMode(PIN_NTC_1, INPUT);
  pinMode(PIN_NTC_2, INPUT);
}

int readRaw1() { return analogRead(PIN_NTC_1); }
int readRaw2() { return analogRead(PIN_NTC_2); }

// Delegue le calcul a la fonction pure (meme code teste en unitaire).
float rawToCelsius(int raw) { return ntcRawToCelsius(raw); }

float readCelsius1() { return rawToCelsius(readRaw1()); }
float readCelsius2() { return rawToCelsius(readRaw2()); }

} // namespace ntc
