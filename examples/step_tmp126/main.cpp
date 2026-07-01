// =============================================================================
//  STEP "TMP126" — valider la communication SPI avec le capteur de temperature.
//  1) lit le DEVICE_ID (doit valoir 0x2126) -> prouve que le SPI parle
//  2) affiche la temperature toutes les secondes
//
//  UTILISATION : copier ce contenu dans src/main.cpp, Build (✓) + Upload (→),
//  puis ouvrir le moniteur serie (115200 bauds).
//
//  Cablage (schema) : SCK=18, MISO=19, MOSI=23, CS=5. SPI mode 0.
//  Commande TMP126 sur 16 bits : bit8 = R/W (1=lecture), [7:0] = adresse.
// =============================================================================
#include <Arduino.h>
#include <SPI.h>

static const int PIN_SCK = 18, PIN_MISO = 19, PIN_MOSI = 23, PIN_CS = 5;

// Lecture d'un registre 16 bits du TMP126.
uint16_t tmp126Read(uint8_t reg) {
  uint16_t cmd = 0x0100 | reg;                 // R/W=1 (lecture) + adresse
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer16(cmd);                         // envoi commande
  uint16_t data = SPI.transfer16(0x0000);      // recuperation donnee
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  return data;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  delay(5);

  uint16_t id = tmp126Read(0x0C);              // registre DEVICE_ID
  Serial.printf("TMP126 DEVICE_ID = 0x%04X (attendu 0x2126)\n", id);
  if (id == 0x2126) { Serial.println("=> SPI OK : capteur present"); }
  else              { Serial.println("=> KO : verifier cablage SPI / CS"); }
}

void loop() {
  int16_t raw = (int16_t) tmp126Read(0x00);    // Temp_Result
  float t = (raw >> 2) * 0.03125f;             // 14 bits [15:2], 0,03125 degC/LSB
  Serial.printf("T = %.2f degC\n", t);
  delay(1000);
}
