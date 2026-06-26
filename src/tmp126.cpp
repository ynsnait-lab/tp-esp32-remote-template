#include <SPI.h>
#include "tmp126.h"
#include "pins.h"

namespace {
  // 1 MHz, MSB first, mode 0 (entree echantillonnee sur front montant de SCLK).
  const SPISettings TMP126_SPI(1000000, MSBFIRST, SPI_MODE0);
}

void TMP126::begin() {
  pinMode(PIN_SPI_CS, OUTPUT);
  digitalWrite(PIN_SPI_CS, HIGH);     // CS au repos = haut
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS);
}

// --- Acces registres -------------------------------------------------------
uint16_t TMP126::readReg(uint8_t reg) {
  uint16_t cmd = 0x0100 | reg;        // R/W = 1 (lecture)
  SPI.beginTransaction(TMP126_SPI);
  digitalWrite(PIN_SPI_CS, LOW);
  SPI.transfer16(cmd);
  uint16_t data = SPI.transfer16(0x0000);
  digitalWrite(PIN_SPI_CS, HIGH);
  SPI.endTransaction();
  return data;
}

void TMP126::writeReg(uint8_t reg, uint16_t value) {
  uint16_t cmd = 0x0000 | reg;        // R/W = 0 (ecriture)
  SPI.beginTransaction(TMP126_SPI);
  digitalWrite(PIN_SPI_CS, LOW);
  SPI.transfer16(cmd);
  SPI.transfer16(value);
  digitalWrite(PIN_SPI_CS, HIGH);
  SPI.endTransaction();
}

// --- Identite / mesure -----------------------------------------------------
uint16_t TMP126::deviceId() { return readReg(REG_DEVICE_ID); }
bool     TMP126::present()  { return deviceId() == DEVICE_ID; }

float TMP126::temperature() {
  return countsToCelsius((int16_t)readReg(REG_TEMP));
}

// --- Configuration ---------------------------------------------------------
uint16_t TMP126::config() { return readReg(REG_CONFIG); }

void TMP126::setComparatorMode(bool comparator) {
  uint16_t c = config();
  if (comparator) c |= CFG_INT_COMP; else c &= ~CFG_INT_COMP;
  writeReg(REG_CONFIG, c);
}

// --- Alarmes ---------------------------------------------------------------
// Les seuils ont le meme format que Temp_Result : counts(14 bits) << 2.
void TMP126::setHighLimit(float celsius) {
  writeReg(REG_THIGH, (uint16_t)(celsiusToCounts(celsius) << 2));
}
void TMP126::setLowLimit(float celsius) {
  writeReg(REG_TLOW, (uint16_t)(celsiusToCounts(celsius) << 2));
}
float TMP126::highLimit() { return countsToCelsius((int16_t)readReg(REG_THIGH)); }
float TMP126::lowLimit()  { return countsToCelsius((int16_t)readReg(REG_TLOW)); }

void TMP126::enableAlerts(bool high, bool low) {
  uint16_t en = readReg(REG_ALERT_ENABLE);
  if (high) en |= EN_THIGH; else en &= ~EN_THIGH;
  if (low)  en |= EN_TLOW;  else en &= ~EN_TLOW;
  writeReg(REG_ALERT_ENABLE, en);
}

uint16_t TMP126::alertStatus() { return readReg(REG_ALERT_STATUS); }
