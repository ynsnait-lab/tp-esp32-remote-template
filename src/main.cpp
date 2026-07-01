#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "pins.h"
#include "hmi.h"
#include "ina237.h"
#include "temperature.h"
#include "mem.h"

// =============================================================================
//  Firmware de validation - carte "Projet M1 2024" (ESP32-PICO-D4)
//  Orchestration du bring-up : chaque bloc est exerce et trace sur la liaison
//  serie (115200 bauds).
//    [1] HMI         : LED verte/rouge + buzzer
//    [2] I2C         : scan du bus + INA237 (id, tensions, temperature die)
//    [3] Temperature : TMP126 + 2 CTN + T_die INA237 (lecture + alarmes + recoupe)
//    [4] Memoire     : capacite flash + detection/test PSRAM
// =============================================================================

INA237 ina;

// --- Seuils de surveillance (ajustables) ---
static const float SEUIL_TEMP_C    = 35.0f;    // alarme si T depasse (degC)
static const float SEUIL_COURANT_A = 8.0f;     // alarme si |I| depasse (A) - cf. config groupe 8000 mA
static const float R_SHUNT_OHM     = 0.0005f;  // shunt X500 : PROVISOIRE, a confirmer

static void scanI2C() {
  Serial.println("[2] I2C : scan du bus...");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("    -> 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("    => %u peripherique(s)\n", found);
}

// Teste l'INA237 et renvoie sa temperature de puce (NAN si absent).
static float testINA237() {
  Serial.println("    INA237 (capteur de courant) :");
  if (!ina.begin(0x40)) {
    Serial.printf("    => ABSENT/erreur (MANUFACTURER_ID lu=0x%04X, attendu 0x5449)\n",
                  ina.manufacturerId());
    return NAN;
  }
  Serial.printf("    => present (MANUFACTURER_ID=0x%04X)\n", ina.manufacturerId());
  Serial.printf("       VBUS   = %.3f V\n",  ina.busVoltage());
  Serial.printf("       VSHUNT = %.6f V\n",  ina.shuntVoltage());
  Serial.printf("       T_die  = %.2f degC\n", ina.dieTemperature());
  // Mesure de courant : decommenter une fois R_shunt (X500) confirme.
  //   ina.configureCurrent(/*maxCurrentA=*/10.0f, /*rShuntOhm=*/0.0005f);
  //   Serial.printf("       I = %.3f A\n", ina.current());
  return ina.dieTemperature();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("===== Projet M1 2024 - Validation electronique =====");
  Serial.println("Carte : ESP32-PICO-D4");
  Serial.printf("I2C SDA=%d SCL=%d | SPI SCK=%d MISO=%d MOSI=%d CS=%d\n",
                PIN_I2C_SDA, PIN_I2C_SCL,
                PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS);
  Serial.printf("LED_G=%d LED_R=%d BUZZER=%d | NTC1=%d NTC2=%d\n",
                PIN_LED_GREEN, PIN_LED_RED, PIN_BUZZER_PWM, PIN_NTC_1, PIN_NTC_2);
  Serial.println("====================================================");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // [1] HMI
  Serial.println("[1] HMI : sequence LED + bip");
  hmi::begin();
  hmi::selfTest();

  // [2] I2C + INA237
  scanI2C();
  float inaDie = testINA237();
  ina.configureCurrent(/*maxCurrentA=*/10.0f, R_SHUNT_OHM);  // active la mesure de courant (R_shunt provisoire)

  // [3] Sous-systeme temperature (TMP126 + CTN + recoupe avec T_die INA237)
  temperature::begin();
  temperature::report(inaDie);

  // [4] Memoire (flash + PSRAM)
  mem::report();
  mem::psramSelfTest();

  Serial.println("--- Bring-up termine ---");
}

// =============================================================================
//  Boucle de surveillance : mesure temperature + courant/tension, compare aux
//  seuils, et signale toute alarme sur l'IHM (LED + buzzer). Suit la FSM.
// =============================================================================
void loop() {
  // --- Mesures ---
  float tC   = temperature::tmp.temperature();  // TMP126 (degC)
  float vbus = ina.busVoltage();                // tension bus (V)
  float cur  = ina.current();                   // courant (A) - NAN si INA absent

  // --- Log serie ---
  Serial.printf("T=%.2f degC | VBUS=%.3f V", tC, vbus);
  if (!isnan(cur)) Serial.printf(" | I=%.3f A", cur);
  Serial.println();

  // --- Surveillance : comparaison aux seuils ---
  bool alarme = false;
  if (tC > SEUIL_TEMP_C) {
    Serial.println("  !! ALARME temperature (seuil depasse)");
    alarme = true;
  }
  if (!isnan(cur) && fabs(cur) > SEUIL_COURANT_A) {
    Serial.println("  !! ALARME courant (seuil depasse)");
    alarme = true;
  }

  // --- IHM : rouge + bip si alarme, vert sinon ---
  if (alarme) {
    hmi::ledGreen(false);
    hmi::ledRed(true);
    hmi::beep(120);
  } else {
    hmi::ledRed(false);
    hmi::ledGreen(true);
  }

  delay(1000);
}
