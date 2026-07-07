// =============================================================================
//  STEP "ALARME" — valider la chaine IHM (LED + buzzer) et la logique d'alarme
//  SANS banc d'injection : on abaisse les seuils de temperature SOUS l'ambiante
//  pour declencher reellement avertissement puis critique (visible webcam).
//
//  Sequence au boot : self-test IHM (vert 0,5s -> rouge 0,5s -> bip).
//  Boucle : lit le TMP126 (SPI) et applique les seuils de TEST :
//     < 26 degC  -> OK        : LED verte fixe
//     26-29 degC -> WARNING   : LED rouge + bip court chaque seconde
//     > 29 degC  -> CRITICAL  : LED rouge + bip long chaque seconde
//  (l'ambiante ~28-31 degC du banc declenche naturellement l'alarme)
//
//  ATTENTION : seuils volontairement FAUX (test IHM). Les seuils reels du CDC
//  sont 20 A / 75 degC (EXF-25).
// =============================================================================
#include <Arduino.h>
#include <SPI.h>

static const int PIN_SCK = 18, PIN_MISO = 19, PIN_MOSI = 23, PIN_CS = 5;
static const int PIN_LED_GREEN = 14, PIN_LED_RED = 15, PIN_BUZZER = 13;

static const float SEUIL_WARN_C = 26.0f;   // seuils de TEST (pas CDC !)
static const float SEUIL_CRIT_C = 29.0f;

// --- TMP126 : lecture registre (cmd 16 bits, bit8 = lecture) ---
uint16_t tmp126Read(uint8_t reg) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer16(0x0100 | reg);
  uint16_t v = SPI.transfer16(0x0000);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  return v;
}
float tmp126Celsius() { return ((int16_t)tmp126Read(0x00) >> 2) * 0.03125f; }

// --- Buzzer via PWM (LEDC canal 0, ~4 kHz comme le Murata) ---
void beep(int ms) {
  ledcWriteTone(0, 4000);
  delay(ms);
  ledcWriteTone(0, 0);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  ledcSetup(0, 4000, 10);
  ledcAttachPin(PIN_BUZZER, 0);

  Serial.println("=== STEP ALARME : self-test IHM ===");
  digitalWrite(PIN_LED_GREEN, HIGH); delay(500);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);   delay(500);
  digitalWrite(PIN_LED_RED, LOW);
  beep(200);
  Serial.printf("Seuils de TEST : warn=%.0f crit=%.0f degC (CDC reel : 75 degC)\n",
                SEUIL_WARN_C, SEUIL_CRIT_C);
}

void loop() {
  float t = tmp126Celsius();
  if (t >= SEUIL_CRIT_C) {
    Serial.printf("T=%.2f degC -> CRITIQUE (LED rouge + bip long)\n", t);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
    beep(400);
  } else if (t >= SEUIL_WARN_C) {
    Serial.printf("T=%.2f degC -> AVERTISSEMENT (LED rouge + bip court)\n", t);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
    beep(80);
  } else {
    Serial.printf("T=%.2f degC -> OK (LED verte)\n", t);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, HIGH);
  }
  delay(1000);
}
