#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "pins.h"
#include "hmi.h"
#include "ina237.h"
#include "temperature.h"
#include "ntc.h"
#include "mem.h"
#include "alarm_logic.h"
#include "ring_buffer.h"
#include "log_format.h"

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

// Historique des dernieres temperatures (log en RAM, sans allocation dynamique).
static RingBuffer<float, 64> histTemp;

// --- Seuils de surveillance (avertissement / critique) ---
static const float TEMP_WARN_C  = 40.0f;    // avertissement temperature (degC)
static const float TEMP_CRIT_C  = 55.0f;    // critique temperature
static const float CUR_WARN_A   = 8.0f;     // avertissement courant (A) - cf. config groupe 8000 mA
static const float CUR_CRIT_A   = 10.0f;    // critique courant
static const float R_SHUNT_OHM  = 0.0005f;  // shunt X500 (valide par le groupe : 0,5 mOhm)

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
  ina.configureCurrent(/*maxCurrentA=*/10.0f, R_SHUNT_OHM);  // active la mesure de courant
  ina.initAdc();                                     // mode continu bus+shunt+temp (Ref #3)
  // Seuils d'alarme materiels INA237 (Ref #5), alignes sur les seuils critiques :
  ina.setShuntOverLimit(CUR_CRIT_A * R_SHUNT_OHM);   // sur-courant (via tension shunt)
  ina.setBusOverLimit(5.5f);                         // bus > 5,5 V = anormal
  ina.setBusUnderLimit(2.8f);                        // bus < 2,8 V = brown-out
  ina.setTempLimit(60.0f);                           // die > 60 degC

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
  // --- Commande serie : "dump" -> restitue l'historique des temperatures (Ref #14)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "dump") {
      Serial.printf("HIST,%u\n", (unsigned)histTemp.size());
      for (size_t i = 0; i < histTemp.size(); i++) {
        Serial.printf("H,%u,%.2f\n", (unsigned)i, histTemp.at(i));
      }
    }
  }

  // --- Mesures ---
  float tC   = temperature::tmp.temperature();  // TMP126 (degC)
  float tNtc = ntc::readCelsius1();             // CTN 1 ambiante (degC) - NAN si hors plage
  float vbus = ina.busVoltage();                // tension bus (V)
  float cur  = ina.current();                   // courant (A) - NAN si INA absent

  // --- Niveau d'alarme (logique pure, testee unitairement) ---
  AlarmLevel niveau = alarmLevelHigh(tC, TEMP_WARN_C, TEMP_CRIT_C);
  if (!isnan(cur)) {
    niveau = alarmWorst(niveau, alarmLevelHigh(fabs(cur), CUR_WARN_A, CUR_CRIT_A));
  }

  // --- Alarmes materielles INA237 : polling DIAG_ALRT (Ref #6, ALERT non cablee) ---
  uint16_t diag = ina.diagAlert();
  const uint16_t DIAG_FAULTS = INA237::DIAG_TMPOL | INA237::DIAG_SHNTOL |
                               INA237::DIAG_BUSOL | INA237::DIAG_BUSUL;
  if (diag & DIAG_FAULTS) {
    Serial.printf("  !! DIAG_ALRT=0x%04X (alarme materielle INA237)\n", diag);
    niveau = AlarmLevel::CRITICAL;
  }

  // --- Log : historique en RAM + ligne CSV sur le port serie ---
  histTemp.push(tC);
  char ligne[80];
  formatLogLine(ligne, sizeof(ligne), millis(), tC,
                isnan(tNtc) ? 0.0f : tNtc, vbus,
                isnan(cur)  ? 0.0f : cur, static_cast<int>(niveau));
  Serial.print("LOG,");
  Serial.println(ligne);   // format : t_ms,temp_tmp126,temp_ntc1,vbus,courant,niveau

  // --- IHM selon le niveau (cf. FSM) ---
  switch (niveau) {
    case AlarmLevel::CRITICAL:
      Serial.println("  !! ALARME CRITIQUE");
      hmi::ledGreen(false); hmi::ledRed(true); hmi::beep(300);
      break;
    case AlarmLevel::WARNING:
      Serial.println("  ! avertissement");
      hmi::ledGreen(false); hmi::ledRed(true); hmi::beep(80);
      break;
    case AlarmLevel::OK:
    default:
      hmi::ledRed(false); hmi::ledGreen(true);
      break;
  }

  delay(1000);
}
