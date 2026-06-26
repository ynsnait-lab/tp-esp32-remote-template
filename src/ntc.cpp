#include <Arduino.h>
#include <math.h>
#include "ntc.h"
#include "pins.h"

namespace {
  const float NTC_ADC_MAX  = 4095.0f;   // 12 bits
  const float NTC_R_PULLUP = 10000.0f;  // R604 / R606
  const float NTC_R0       = 10000.0f;  // R25 du NB12K00103 (10 k a 25 degC)
  const float NTC_T0_K     = 298.15f;   // 25 degC en kelvin
  const float NTC_BETA     = 3630.0f;   // B(25/85) du NB12K00103
}

namespace ntc {

void begin() {
  analogReadResolution(12);
  pinMode(PIN_NTC_1, INPUT);
  pinMode(PIN_NTC_2, INPUT);
}

int readRaw1() { return analogRead(PIN_NTC_1); }
int readRaw2() { return analogRead(PIN_NTC_2); }

float rawToCelsius(int raw) {
  if (raw <= 0 || raw >= (int)NTC_ADC_MAX) return NAN;          // hors plage / circuit ouvert
  float rNtc = NTC_R_PULLUP * raw / (NTC_ADC_MAX - raw);        // ratiometrique (Vref annulee)
  float invT = 1.0f / NTC_T0_K + (1.0f / NTC_BETA) * logf(rNtc / NTC_R0);
  return (1.0f / invT) - 273.15f;
}

float readCelsius1() { return rawToCelsius(readRaw1()); }
float readCelsius2() { return rawToCelsius(readRaw2()); }

} // namespace ntc
