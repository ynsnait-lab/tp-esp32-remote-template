// =============================================================================
//  STEP "MONITORING COMPLET v3" — v1 + INJECTION DE DEFAUT LOGICIELLE.
//
//  Principe (fault injection) : impossible de chauffer la carte a 80 degC ou
//  d'injecter 25 A a distance -> on force periodiquement les VALEURS MESUREES
//  a des niveaux critiques, juste apres la mesure, pour exercer toute la
//  chaine decision -> IHM -> retour a la normale. Les mesures reelles
//  continuent d'etre affichees a cote des valeurs forcees.
//
//  Cycle de 15 secondes :
//    t = 0..9 s   : fonctionnement NORMAL (mesures reelles, LED verte)
//    t = 10..14 s : INJECTION [FORCE] : T = 80 degC, I = 25 A -> CRITIQUE
//                   (LED rouge + bip long chaque seconde)
//    puis retour automatique a la normale (LED verte) -> boucle.
//
//  Seuils reels CDC : I warn/crit = 15/20 A ; T warn/crit = 60/75 degC.
//  -> 80 degC et 25 A depassent les seuils CRITIQUES.
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
static const float R_SHUNT = 0.0005f;
static const float I_MAX   = 30.0f;
static const float I_LSB   = I_MAX / 32768.0f;
static const uint16_t SHUNT_CAL = (uint16_t)(819.2e6f * I_LSB * R_SHUNT); // 375

// --- Seuils reels CDC ---
static const float I_WARN = 15.0f, I_CRIT = 20.0f;
static const float T_WARN = 60.0f, T_CRIT = 75.0f;

// --- Scenario d'injection ---
static const float FORCE_T_C   = 80.0f;   // temperature forcee (> 75 = critique)
static const float FORCE_I_A   = 25.0f;   // courant force (> 20 = critique)
static const int   CYCLE_S     = 15;      // duree du cycle complet
static const int   FORCE_DEB_S = 10;      // injection de t=10 a t=14 s (5 s)

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
//  CTN : table AVX NB12K00103JBB
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

  Serial.println("===== MONITORING COMPLET v3 - INJECTION DE DEFAUT =====");
  uint16_t idIna = inaRead16(0x3E), idTmp = tmpRead(0x0C);
  Serial.printf("INA237 MANUF_ID  = 0x%04X %s\n", idIna, idIna == 0x5449 ? "[OK]" : "[KO!]");
  Serial.printf("TMP126 DEVICE_ID = 0x%04X %s\n", idTmp, idTmp == 0x2126 ? "[OK]" : "[KO!]");
  inaWrite16(0x01, 0xFB68);
  inaWrite16(0x02, SHUNT_CAL);
  Serial.printf("Cycle %d s : normal 0-%d s, INJECTION (T=%.0f degC, I=%.0f A) %d-%d s\n",
                CYCLE_S, FORCE_DEB_S - 1, FORCE_T_C, FORCE_I_A, FORCE_DEB_S, CYCLE_S - 1);
  Serial.printf("Seuils CDC : I %g/%g A, T %g/%g degC\n", I_WARN, I_CRIT, T_WARN, T_CRIT);
}

void loop() {
  // --- Mesures REELLES (toujours effectuees et affichees) ---
  float vbus   = inaRead16(0x05) * 3.125e-3f;
  float vshunt = (int16_t)inaRead16(0x04) * 5.0e-6f;
  float iReel  = (int16_t)inaRead16(0x07) * I_LSB;
  float tDie   = ((int16_t)inaRead16(0x06) >> 4) * 0.125f;
  float tReel  = ((int16_t)tmpRead(0x00) >> 2) * 0.03125f;
  float tN1    = ntcCelsius(analogRead(PIN_NTC_1));
  float tN2    = ntcCelsius(analogRead(PIN_NTC_2));

  // --- Scenario : faut-il forcer les valeurs ? ---
  int tCycle = (int)((millis() / 1000UL) % CYCLE_S);
  bool force = (tCycle >= FORCE_DEB_S);

  // Valeurs UTILISEES par la surveillance (forcees ou reelles)
  float tSurv = force ? FORCE_T_C : max(tReel, max(isnan(tN1) ? -999.0f : tN1,
                                                   isnan(tN2) ? -999.0f : tN2));
  float iSurv = force ? FORCE_I_A : fabsf(iReel);

  if (force) {
    Serial.printf("[FORCE t=%2ds] T_surv=%.1f degC, I_surv=%.1f A  (reel : T=%.2f, I=%+.3f A)\n",
                  tCycle, tSurv, iSurv, tReel, iReel);
  } else {
    Serial.printf("[normal t=%2ds] %.3f V | %+.4f mV | %+.3f A | Tdie %.2f | TMP %.2f | CTN %.1f / %.1f\n",
                  tCycle, vbus, vshunt * 1000.0f, iReel, tDie, tReel, tN1, tN2);
  }

  // --- Decision (meme logique que la production, seuils CDC reels) ---
  int niveau = 0;
  if (iSurv >= I_CRIT || tSurv >= T_CRIT) niveau = 2;
  else if (iSurv >= I_WARN || tSurv >= T_WARN) niveau = 1;

  if (niveau == 2) {
    Serial.println("  !! CRITIQUE -> LED rouge + bip long");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(400);
    delay(600);
  } else if (niveau == 1) {
    Serial.println("  ! avertissement -> LED rouge + bip court");
    digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); beep(80);
    delay(920);
  } else {
    digitalWrite(PIN_LED_RED, LOW); digitalWrite(PIN_LED_GREEN, HIGH);
    delay(1000);
  }
}
