// =============================================================================
//  STEP "MONITORING COMPLET v2" — comme la v1 (toutes les grandeurs, vraies
//  conversions) avec une signature sonore claire :
//
//    * Au boot : 3 bips rapides = "je suis flashe, connecte, et je demarre".
//    * Si la temperature PCB depasse 36 degC : 3 bips toutes les 2 secondes
//      (et LED rouge). C'est LE SEUL cas ou le buzzer sonne.
//    * Sinon : LED verte, silence.
//
//  Seuil 36 degC = seuil de DEMONSTRATION, ~4 degC au-dessus de l'ambiante du
//  banc (~31-32 degC) -> etat silencieux stable au depart, declenchement franc
//  quand la carte chauffe. Le seuil reel du CDC reste 75 degC (EXF-25).
// =============================================================================
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>

// --- Broches (schema carte "Projet M1 2024") ---
static const int PIN_SCK = 18, PIN_MISO = 19, PIN_MOSI = 23, PIN_CS = 5;
static const int PIN_SDA = 21, PIN_SCL = 22;
static const int PIN_NTC_1 = 26, PIN_NTC_2 = 25;
static const int PIN_LED_GREEN = 14, PIN_LED_RED = 15, PIN_BUZZER = 13;

// --- Constantes de mesure ---
static const float R_SHUNT = 0.0005f;                 // X500 : 0,5 mOhm
static const float I_MAX   = 30.0f;                   // plage CDC EXF-09
static const float I_LSB   = I_MAX / 32768.0f;        // 915,5 uA/bit
static const uint16_t SHUNT_CAL = (uint16_t)(819.2e6f * I_LSB * R_SHUNT); // 375

// --- Seuil buzzer (DEMO) ---
static const float T_BUZZ_C = 36.0f;                  // 3 bips / 2 s au-dela

// =============================================================================
//  INA237 (I2C 0x40)
// =============================================================================
uint16_t inaRead16(uint8_t reg) {
  Wire.beginTransmission(0x40);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(0x40, 2) != 2) return 0;
  uint16_t hi = Wire.read(), lo = Wire.read();
  return (hi << 8) | lo;
}
uint32_t inaRead24(uint8_t reg) {
  Wire.beginTransmission(0x40);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(0x40, 3) != 3) return 0;
  uint32_t b2 = Wire.read(), b1 = Wire.read(), b0 = Wire.read();
  return (b2 << 16) | (b1 << 8) | b0;
}
bool inaWrite16(uint8_t reg, uint16_t v) {
  Wire.beginTransmission(0x40);
  Wire.write(reg);
  Wire.write((uint8_t)(v >> 8));
  Wire.write((uint8_t)(v & 0xFF));
  return Wire.endTransmission() == 0;
}

// =============================================================================
//  TMP126 (SPI)
// =============================================================================
uint16_t tmpRead(uint8_t reg) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer16(0x0100 | reg);
  uint16_t v = SPI.transfer16(0x0000);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  return v;
}

// =============================================================================
//  CTN : table AVX NB12K00103JBB, -40..+120 degC pas de 5, interpolation
// =============================================================================
static const int NTC_N = 33, NTC_MIN = -40, NTC_STEP = 5;
static const uint16_t ntcTable[NTC_N] = {
    3222, 3195, 3161, 3118, 3064, 2998, 2919, 2825, 2716, 2592,
    2455, 2306, 2147, 1983, 1816, 1650, 1489, 1335, 1190, 1056,
     934,  824,  725,  637,  560,  492,  432,  380,  335,  295,
     260,  230,  204
};
float ntcCelsius(int raw) {
  if (raw <= 0 || raw >= 4095) return NAN;
  if (raw > ntcTable[0])       return (float)NTC_MIN;
  if (raw < ntcTable[NTC_N-1]) return (float)(NTC_MIN + (NTC_N-1)*NTC_STEP);
  int lo = 0, hi = NTC_N - 1;
  while (lo + 1 != hi) {
    int mid = (lo + hi) >> 1;
    if (ntcTable[mid] <= raw) hi = mid; else lo = mid;
  }
  int32_t diff = (int32_t)ntcTable[lo] - ntcTable[lo+1];
  int32_t dval = (int32_t)ntcTable[lo] - raw;
  return NTC_MIN + (lo + (diff ? (float)dval/diff : 0.0f)) * NTC_STEP;
}

// --- Buzzer 4 kHz ---
void beep(int ms) { ledcWriteTone(0, 4000); delay(ms); ledcWriteTone(0, 0); }
void tripleBeep(int onMs, int gapMs) {
  for (int i = 0; i < 3; i++) { beep(onMs); delay(gapMs); }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  Wire.begin(PIN_SDA, PIN_SCL);
  analogReadResolution(12);
  ledcSetup(0, 4000, 10);
  ledcAttachPin(PIN_BUZZER, 0);

  Serial.println("===== MONITORING COMPLET v2 - carte Projet M1 2024 =====");

  // >>> Signature sonore de demarrage : 3 bips rapides = "connecte, ca part" <<<
  tripleBeep(70, 80);

  uint16_t idIna = inaRead16(0x3E), idTmp = tmpRead(0x0C);
  Serial.printf("INA237 MANUF_ID  = 0x%04X %s\n", idIna, idIna == 0x5449 ? "[OK]" : "[KO!]");
  Serial.printf("TMP126 DEVICE_ID = 0x%04X %s\n", idTmp, idTmp == 0x2126 ? "[OK]" : "[KO!]");

  inaWrite16(0x01, 0xFB68);                          // ADC continu bus+shunt+temp
  inaWrite16(0x02, SHUNT_CAL);                       // calibration 30 A
  Serial.printf("Calibration : I_max=%.0f A, LSB=%.1f uA, SHUNT_CAL=%u\n",
                I_MAX, I_LSB * 1e6f, SHUNT_CAL);
  Serial.printf("Buzzer : 3 bips / 2 s si T PCB > %.0f degC (seuil DEMO ; CDC reel 75 degC)\n",
                T_BUZZ_C);
  Serial.println("Colonnes : VBUS V | VSHUNT mV | I_reg A | I_ohm A | P W | Tdie | TMP126 | CTN1 | CTN2");
}

void loop() {
  // --- Mesures (vraies conversions) ---
  float vbus   = inaRead16(0x05) * 3.125e-3f;
  float vshunt = (int16_t)inaRead16(0x04) * 5.0e-6f;
  float iReg   = (int16_t)inaRead16(0x07) * I_LSB;
  float iOhm   = vshunt / R_SHUNT;
  float power  = inaRead24(0x08) * 0.2f * I_LSB;
  float tDie   = ((int16_t)inaRead16(0x06) >> 4) * 0.125f;
  float tTmp   = ((int16_t)tmpRead(0x00) >> 2) * 0.03125f;
  int   r1 = analogRead(PIN_NTC_1), r2 = analogRead(PIN_NTC_2);
  float tN1 = ntcCelsius(r1), tN2 = ntcCelsius(r2);

  Serial.printf("%.3f V | %+.4f mV | %+.3f A | %+.3f A | %.3f W | %.2f | %.2f | %.1f | %.1f\n",
                vbus, vshunt * 1000.0f, iReg, iOhm, power, tDie, tTmp, tN1, tN2);

  // --- Temperature PCB = pire des sondes PCB (TMP126 + 2 CTN) ---
  float tPcb = max(isnan(tTmp) ? -999.0f : tTmp,
                   max(isnan(tN1) ? -999.0f : tN1, isnan(tN2) ? -999.0f : tN2));

  if (tPcb > T_BUZZ_C) {
    // >>> SEUL cas sonore : T > 36 degC -> 3 bips, cycle de 2 secondes <<<
    Serial.printf("  !! T PCB = %.2f degC > %.0f degC -> 3 bips\n", tPcb, T_BUZZ_C);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
    tripleBeep(100, 120);          // ~0,66 s de sequence sonore
    delay(1340);                   // complement -> periode ~2 s
  } else {
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(1000);                   // rythme nominal : 1 mesure / s, silence
  }
}
