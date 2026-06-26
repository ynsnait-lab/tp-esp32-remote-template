#include <Arduino.h>
#include <math.h>
#include "temperature.h"
#include "ntc.h"
#include "pins.h"

namespace temperature {

TMP126 tmp;

void begin() {
  ntc::begin();
  tmp.begin();
  if (tmp.present()) {
    tmp.setComparatorMode(true);                 // ALERT = detecteur de seuil
    tmp.setLowLimit(DEFAULT_LOW_C);
    tmp.setHighLimit(DEFAULT_HIGH_C);
    tmp.enableAlerts(/*high=*/true, /*low=*/true);
  }
}

// Petit utilitaire d'affichage : valeur ou "n/a" si NAN.
static void printC(const char* label, float c) {
  if (isnan(c)) Serial.printf("    %-10s : n/a\n", label);
  else          Serial.printf("    %-10s : %.2f degC\n", label, c);
}

void report(float inaDieC) {
  Serial.println("[T] Sous-systeme temperature :");

  // 1. TMP126
  float tTmp = NAN;
  if (tmp.present()) {
    tTmp = tmp.temperature();
    printC("TMP126", tTmp);
    Serial.printf("       seuils: bas=%.1f  haut=%.1f degC\n",
                  tmp.lowLimit(), tmp.highLimit());
    uint16_t st = tmp.alertStatus();
    Serial.printf("       ALERT_STATUS=0x%04X  (HIGH=%d LOW=%d)\n",
                  st, (st & TMP126::ST_THIGH) ? 1 : 0,
                      (st & TMP126::ST_TLOW)  ? 1 : 0);
  } else {
    Serial.printf("    TMP126     : ABSENT (DEVICE_ID=0x%04X, attendu 0x2126)\n",
                  tmp.deviceId());
  }

  // 2. CTN
  float t1 = ntc::readCelsius1();
  float t2 = ntc::readCelsius2();
  Serial.printf("    CTN1 (GPIO%d) raw=%d -> ", PIN_NTC_1, ntc::readRaw1()); printC("", t1);
  Serial.printf("    CTN2 (GPIO%d) raw=%d -> ", PIN_NTC_2, ntc::readRaw2()); printC("", t2);

  // 3. T_die INA237 (fournie par l'appelant)
  printC("T_die INA", inaDieC);

  // --- Validation croisee : les sources valides doivent concorder ---
  float vals[5]; int n = 0;
  if (!isnan(tTmp))   vals[n++] = tTmp;
  if (!isnan(t1))     vals[n++] = t1;
  if (!isnan(t2))     vals[n++] = t2;
  if (!isnan(inaDieC))vals[n++] = inaDieC;
  if (n >= 2) {
    float mn = vals[0], mx = vals[0];
    for (int i = 1; i < n; i++) { mn = min(mn, vals[i]); mx = max(mx, vals[i]); }
    float spread = mx - mn;
    Serial.printf("    => recoupe %d sources, ecart max = %.2f degC %s\n",
                  n, spread, spread <= 5.0f ? "[OK]" : "[A VERIFIER]");
  }
}

} // namespace temperature
