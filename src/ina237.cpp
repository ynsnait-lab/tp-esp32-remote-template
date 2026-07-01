#include <Wire.h>
#include "ina237.h"

// --- Acces registres (16 bits, MSB en premier) ------------------------------
uint16_t INA237::read16(uint8_t reg) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;   // repeated start
  if (Wire.requestFrom((int)_addr, 2) != 2) return 0;
  uint16_t hi = Wire.read();
  uint16_t lo = Wire.read();
  return (hi << 8) | lo;
}

bool INA237::write16(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  return Wire.endTransmission() == 0;
}

bool INA237::begin(uint8_t i2cAddr) {
  _addr = i2cAddr;
  return manufacturerId() == MANUF_ID_TI;
}

uint16_t INA237::manufacturerId() { return read16(REG_MANUF_ID); }

// VBUS : valeur non signee, 3,125 mV/LSB
float INA237::busVoltage() {
  return read16(REG_VBUS) * 3.125e-3f;
}

// VSHUNT : complement a 2, 5 uV/LSB (ADCRANGE = 0)
float INA237::shuntVoltage() {
  int16_t raw = (int16_t)read16(REG_VSHUNT);
  return raw * 5.0e-6f;
}

// DIETEMP : bits [15:4], complement a 2, 125 m degC/LSB
float INA237::dieTemperature() {
  int16_t raw = (int16_t)read16(REG_DIETEMP);
  return (raw >> 4) * 0.125f;     // decalage signe -> 12 bits utiles
}

// SHUNT_CAL = 819,2e6 * CURRENT_LSB * R_shunt   (ADCRANGE = 0)
void INA237::configureCurrent(float maxCurrentA, float rShuntOhm) {
  _currentLsb = maxCurrentA / 32768.0f;            // 2^15
  uint16_t cal = (uint16_t)(819.2e6f * _currentLsb * rShuntOhm);
  write16(REG_SHUNT_CAL, cal & 0x7FFF);            // 15 bits
}

float INA237::current() {
  if (_currentLsb <= 0.0f) return NAN;             // non calibre
  int16_t raw = (int16_t)read16(REG_CURRENT);
  return raw * _currentLsb;
}

// ADC_CONFIG (0x01) : MODE = Fh (continu bus + shunt + temperature),
// temps de conversion 1052 us et moyennage x1 (valeur de reset 0xFB68,
// ecrite explicitement pour un etat connu apres init).
bool INA237::initAdc() {
  return write16(REG_ADC_CONFIG, 0xFB68);
}

// SOVL : complement a 2, 5 uV/LSB (ADCRANGE = 0)
void INA237::setShuntOverLimit(float volts) {
  write16(REG_SOVL, (uint16_t)(int16_t)(volts / 5.0e-6f));
}

// BOVL / BUVL : non signes, 15 bits utiles, 3,125 mV/LSB
void INA237::setBusOverLimit(float volts) {
  write16(REG_BOVL, (uint16_t)(volts / 3.125e-3f) & 0x7FFF);
}
void INA237::setBusUnderLimit(float volts) {
  write16(REG_BUVL, (uint16_t)(volts / 3.125e-3f) & 0x7FFF);
}

// TEMP_LIMIT : 125 m degC/LSB, champ sur les bits [15:4]
void INA237::setTempLimit(float celsius) {
  int16_t counts = (int16_t)(celsius / 0.125f);
  write16(REG_TEMP_LIMIT, (uint16_t)(counts << 4));
}

// DIAG_ALRT : flags d'etat (mode transparent par defaut : un flag retombe
// quand la condition disparait ; lecture par polling, ALERT non cablee).
uint16_t INA237::diagAlert() {
  return read16(REG_DIAG_ALRT);
}
