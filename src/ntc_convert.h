#pragma once
#include <math.h>
// =============================================================================
//  Conversion PURE ADC -> degC pour la CTN NB12K00103 (10k a 25 degC, Beta 3630).
//  AUCUNE dependance Arduino/materiel -> compilable et TESTABLE sur PC.
//  Montage : 3V3 -- R_pullup(10k) -- noeud -- NTC -- GND (ratiometrique).
// =============================================================================
constexpr float NTC_ADC_MAX  = 4095.0f;   // ADC 12 bits
constexpr float NTC_R_PULLUP = 10000.0f;  // R604 / R606
constexpr float NTC_R0       = 10000.0f;  // R25 (10k a 25 degC)
constexpr float NTC_T0_K     = 298.15f;   // 25 degC en kelvin
constexpr float NTC_BETA     = 3630.0f;   // B(25/85)

// Renvoie la temperature en degC, ou NAN si la valeur brute est hors plage
// (0 ou pleine echelle = circuit ouvert / court-circuit).
inline float ntcRawToCelsius(int raw) {
  if (raw <= 0 || raw >= (int)NTC_ADC_MAX) {
    return NAN;
  }
  float rNtc = NTC_R_PULLUP * raw / (NTC_ADC_MAX - raw);          // Vref annulee
  float invT = 1.0f / NTC_T0_K + (1.0f / NTC_BETA) * logf(rNtc / NTC_R0);
  return (1.0f / invT) - 273.15f;
}
