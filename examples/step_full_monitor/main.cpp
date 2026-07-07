// =============================================================================
//  STEP "MONITORING COMPLET" — la carte entiere en un seul sketch autonome.
//  Toutes les grandeurs, toutes les vraies conversions, les vrais seuils CDC.
//
//  Au boot   : identites (INA237 0x5449, TMP126 0x2126) + calibration courant.
//  Chaque s  : VBUS [V], VSHUNT [mV], I registre [A], I loi d'Ohm [A],
//              P [W], T_die [degC], TMP126 [degC], CTN1/CTN2 [degC] (table),
//              recoupe temperatures, niveau d'alarme, IHM (LED + buzzer).
//
//  Calibration courant (datasheet INA237, ADCRANGE=0) :
//     I_max = 30 A (plage CDC EXF-09, pics AGV 25 A)
//     CURRENT_LSB = 30 / 2^15 = 915,5 uA/bit
//     SHUNT_CAL   = 819,2e6 x 915,5e-6 x 0,0005 = 375
//  Verification croisee du courant : I_ohm = VSHUNT / R_shunt (sans calibration)
//  doit concorder avec I_reg (registre CURRENT calibre).
//
//  Seuils reels CDC Ventec :
//     courant  : avertissement > 15 A (CDC 3.6) / critique > 20 A (EXF-25)
//     T PCB    : avertissement > 60 degC        / critique > 75 degC (EXF-25)
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
static const float R_SHUNT   = 0.0005f;             // X500 : 0,5 mOhm
static const float I_MAX     = 30.0f;               // plage CDC EXF-09
static const float I_LSB     = I_MAX / 32768.0f;    // 915,5 uA/bit
static const uint16_t SHUNT_CAL = (uint16_t)(819.2e6f * I_LSB * R_SHUNT); // 375

// --- Seuils reels CDC ---
static const float I_WARN = 15.0f, I_CRIT = 20.0f;      // A (CDC 3.6 / EXF-25)
static const float T_WARN = 60.0f, T_CRIT = 75.0f;      // degC (EXF-25)

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
uint32_t inaRead24(uint8_t reg) {                    // POWER = 24 bits
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
  if (raw <= 0 || raw >= 4095) return NAN;           // defaut capteur
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

  Serial.println("===== MONITORING COMPLET - carte Projet M1 2024 =====");

  // 1. Identites (preuve de communication)
  uint16_t idIna = inaRead16(0x3E), idTmp = tmpRead(0x0C);
  Serial.printf("INA237 MANUF_ID  = 0x%04X %s\n", idIna, idIna == 0x5449 ? "[OK]" : "[KO!]");
  Serial.printf("TMP126 DEVICE_ID = 0x%04X %s\n", idTmp, idTmp == 0x2126 ? "[OK]" : "[KO!]");

  // 2. Config INA237 : ADC continu bus+shunt+temp, puis calibration 30 A
  inaWrite16(0x01, 0xFB68);                          // ADC_CONFIG (valeur reset, explicite)
  inaWrite16(0x02, SHUNT_CAL);                       // SHUNT_CAL = 375
  Serial.printf("Calibration : I_max=%.0f A, LSB=%.1f uA, SHUNT_CAL=%u\n",
                I_MAX, I_LSB * 1e6f, SHUNT_CAL);
  Serial.printf("Seuils CDC : I warn/crit = %.0f/%.0f A, T warn/crit = %.0f/%.0f degC\n",
                I_WARN, I_CRIT, T_WARN, T_CRIT);
  Serial.println("Colonnes : VBUS V | VSHUNT mV | I_reg A | I_ohm A | P W | Tdie | TMP126 | CTN1 | CTN2");

  // 3. Self-test IHM
  digitalWrite(PIN_LED_GREEN, HIGH); delay(300);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);   delay(300);
  digitalWrite(PIN_LED_RED, LOW);
  beep(150);
}

void loop() {
  // --- INA237 : toutes les grandeurs electriques ---
  float vbus   = inaRead16(0x05) * 3.125e-3f;              // V   (3,125 mV/LSB)
  float vshunt = (int16_t)inaRead16(0x04) * 5.0e-6f;       // V   (5 uV/LSB)
  float iReg   = (int16_t)inaRead16(0x07) * I_LSB;         // A   (registre calibre)
  float iOhm   = vshunt / R_SHUNT;                         // A   (loi d'Ohm, recoupe)
  float power  = inaRead24(0x08) * 0.2f * I_LSB;           // W   (0,2 x LSB x reg)
  float tDie   = ((int16_t)inaRead16(0x06) >> 4) * 0.125f; // degC

  // --- Temperatures ---
  float tTmp = ((int16_t)tmpRead(0x00) >> 2) * 0.03125f;   // degC (0,03125/LSB)
  int   r1 = analogRead(PIN_NTC_1), r2 = analogRead(PIN_NTC_2);
  float tN1 = ntcCelsius(r1), tN2 = ntcCelsius(r2);

  Serial.printf("%.3f V | %+.4f mV | %+.3f A | %+.3f A | %.3f W | %.2f | %.2f | %.1f | %.1f\n",
                vbus, vshunt * 1000.0f, iReg, iOhm, power, tDie, tTmp, tN1, tN2);

  // --- Recoupe courant : registre calibre vs loi d'Ohm ---
  if (fabsf(iReg - iOhm) > 0.1f) {
    Serial.printf("  ! recoupe courant : I_reg et I_ohm divergent de %.3f A\n", fabsf(iReg - iOhm));
  }

  // --- Recoupe temperatures (TI-04) ---
  float vals[4]; int n = 0;
  if (!isnan(tTmp)) vals[n++] = tTmp;
  if (!isnan(tN1))  vals[n++] = tN1;
  if (!isnan(tN2))  vals[n++] = tN2;
  vals[n++] = tDie;
  float mn = vals[0], mx = vals[0];
  for (int i = 1; i < n; i++) { mn = min(mn, vals[i]); mx = max(mx, vals[i]); }
  Serial.printf("  recoupe %d sources T : ecart max %.2f degC %s\n",
                n, mx - mn, (mx - mn) <= 5.0f ? "[OK]" : "[A VERIFIER]");

  // --- Niveau d'alarme (seuils reels CDC) ---
  float tPcb = max(isnan(tTmp) ? -999.0f : tTmp,
                   max(isnan(tN1) ? -999.0f : tN1, isnan(tN2) ? -999.0f : tN2));
  float iAbs = fabsf(iReg);
  int niveau = 0;                                          // 0 OK, 1 warn, 2 crit
  if (iAbs >= I_CRIT || tPcb >= T_CRIT) niveau = 2;
  else if (iAbs >= I_WARN || tPcb >= T_WARN) niveau = 1;

  if (niveau == 2) {
    Serial.println("  !! ALARME CRITIQUE (EXF-25)");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(400);
  } else if (niveau == 1) {
    Serial.println("  ! avertissement");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(80);
  } else {
    digitalWrite(PIN_LED_RED, LOW); digitalWrite(PIN_LED_GREEN, HIGH);
  }
  delay(1000);
}
