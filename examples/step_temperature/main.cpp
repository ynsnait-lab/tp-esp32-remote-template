// =============================================================================
//  STEP "TEMPERATURE REELLE" — lit les 4 sources de temperature de la carte
//  avec les VRAIES conversions, les recoupe, et applique les VRAIS seuils CDC.
//
//  Sources :
//    1. TMP126 (SPI)      : capteur principal — raw[15:2] x 0,03125 degC/LSB
//    2. CTN1 (ADC GPIO26) : table constructeur NB12K00103JBB + interpolation
//    3. CTN2 (ADC GPIO25) : idem
//    4. INA237 T_die (I2C): raw[15:4] x 0,125 degC/LSB
//
//  Seuils REELS (CDC Ventec EXF-25) : CRITIQUE si T PCB > 75 degC.
//  Avertissement a 60 degC (marge d'anticipation, EXF-27).
//  Validation croisee : ecart max entre sources valides <= 5 degC.
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

// --- Seuils REELS du CDC (EXF-25) ---
static const float SEUIL_WARN_C = 60.0f;   // avertissement (anticipation)
static const float SEUIL_CRIT_C = 75.0f;   // critique = valeur CDC

// =============================================================================
//  Conversion CTN : table AVX NB12K00103JBB (aide de cours), -40..+120 degC
//  par pas de 5 degC. Recherche binaire + interpolation lineaire (entiers).
// =============================================================================
static const int NTC_N = 33, NTC_MIN = -40, NTC_STEP = 5;
static const uint16_t ntcTable[NTC_N] = {
    3222, 3195, 3161, 3118, 3064, 2998, 2919, 2825, 2716, 2592,
    2455, 2306, 2147, 1983, 1816, 1650, 1489, 1335, 1190, 1056,
     934,  824,  725,  637,  560,  492,  432,  380,  335,  295,
     260,  230,  204
};
// Renvoie la temperature en degC, ou NAN si defaut capteur (raw 0/4095).
float ntcCelsius(int raw) {
  if (raw <= 0 || raw >= 4095) return NAN;            // court-circuit / ouvert
  if (raw > ntcTable[0])       return (float)NTC_MIN; // plus froid que la table
  if (raw < ntcTable[NTC_N-1]) return (float)(NTC_MIN + (NTC_N-1)*NTC_STEP);
  int lo = 0, hi = NTC_N - 1;
  while (lo + 1 != hi) {
    int mid = (lo + hi) >> 1;
    if (ntcTable[mid] <= raw) hi = mid; else lo = mid;
  }
  int32_t diff = (int32_t)ntcTable[lo] - ntcTable[lo+1];
  int32_t dval = (int32_t)ntcTable[lo] - raw;
  float frac = (diff != 0) ? (float)dval / (float)diff : 0.0f;
  return NTC_MIN + (lo + frac) * NTC_STEP;
}

// =============================================================================
//  TMP126 (SPI) : temperature = complement a 2 sur [15:2], 0,03125 degC/LSB
// =============================================================================
uint16_t tmp126Read(uint8_t reg) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer16(0x0100 | reg);                       // bit8 = lecture
  uint16_t v = SPI.transfer16(0x0000);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  return v;
}
float tmp126Celsius() {
  return ((int16_t)tmp126Read(0x00) >> 2) * 0.03125f; // >>2 : decalage signe
}

// =============================================================================
//  INA237 (I2C) : DIETEMP = complement a 2 sur [15:4], 0,125 degC/LSB
// =============================================================================
uint16_t ina237Read(uint8_t reg) {
  Wire.beginTransmission(0x40);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(0x40, 2);
  uint16_t hi = Wire.read(), lo = Wire.read();
  return (hi << 8) | lo;
}
float ina237DieCelsius() {
  return ((int16_t)ina237Read(0x06) >> 4) * 0.125f;   // >>4 : decalage signe
}

// --- Buzzer 4 kHz (PWM LEDC) ---
void beep(int ms) { ledcWriteTone(0, 4000); delay(ms); ledcWriteTone(0, 0); }

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

  Serial.println("=== TEMPERATURE REELLE - seuils CDC : warn 60 / crit 75 degC ===");
  Serial.printf("TMP126 DEVICE_ID = 0x%04X (attendu 0x2126)\n", tmp126Read(0x0C));
  Serial.printf("INA237 MANUF_ID  = 0x%04X (attendu 0x5449)\n", ina237Read(0x3E));
}

void loop() {
  // --- Lectures avec les vraies conversions ---
  float tTmp = tmp126Celsius();
  int   r1 = analogRead(PIN_NTC_1), r2 = analogRead(PIN_NTC_2);
  float tN1 = ntcCelsius(r1), tN2 = ntcCelsius(r2);
  float tDie = ina237DieCelsius();

  Serial.printf("TMP126=%.2f | CTN1=%.1f (raw %d) | CTN2=%.1f (raw %d) | T_die=%.2f degC\n",
                tTmp, tN1, r1, tN2, r2, tDie);

  // --- Validation croisee (TI-04) : ecart max entre sources valides ---
  float vals[4]; int n = 0;
  if (!isnan(tTmp)) vals[n++] = tTmp;
  if (!isnan(tN1))  vals[n++] = tN1;
  if (!isnan(tN2))  vals[n++] = tN2;
  if (!isnan(tDie)) vals[n++] = tDie;
  if (n >= 2) {
    float mn = vals[0], mx = vals[0];
    for (int i = 1; i < n; i++) { mn = min(mn, vals[i]); mx = max(mx, vals[i]); }
    Serial.printf("  recoupe %d sources : ecart max %.2f degC %s\n",
                  n, mx - mn, (mx - mn) <= 5.0f ? "[OK]" : "[A VERIFIER]");
  }

  // --- Seuils REELS du CDC sur la temperature PCB (pire des sondes) ---
  float tPcb = max(isnan(tTmp) ? -999.0f : tTmp,
                   max(isnan(tN1) ? -999.0f : tN1, isnan(tN2) ? -999.0f : tN2));
  if (tPcb >= SEUIL_CRIT_C) {
    Serial.println("  !! CRITIQUE (> 75 degC, EXF-25)");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(400);
  } else if (tPcb >= SEUIL_WARN_C) {
    Serial.println("  ! avertissement (> 60 degC)");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(80);
  } else {
    digitalWrite(PIN_LED_RED, LOW); digitalWrite(PIN_LED_GREEN, HIGH);
  }
  delay(1000);
}
