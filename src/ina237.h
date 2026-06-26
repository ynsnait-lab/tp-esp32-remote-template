#pragma once
#include <Arduino.h>
// =============================================================================
//  INA237 - moniteur de courant/tension/puissance, interface I2C.
//  Registres et conversions : datasheet TI SBOSA20A.
//  Adresse I2C de la carte : A0=A1=GND -> 0x40.
// =============================================================================

class INA237 {
public:
  // Registres (datasheet INA237, table 7-x)
  static const uint8_t REG_CONFIG     = 0x00;
  static const uint8_t REG_ADC_CONFIG = 0x01;
  static const uint8_t REG_SHUNT_CAL  = 0x02;
  static const uint8_t REG_VSHUNT     = 0x04;
  static const uint8_t REG_VBUS       = 0x05;
  static const uint8_t REG_DIETEMP    = 0x06;
  static const uint8_t REG_CURRENT    = 0x07;
  static const uint8_t REG_POWER      = 0x08;
  static const uint8_t REG_MANUF_ID   = 0x3E;   // doit valoir 0x5449 ("TI")
  static const uint16_t MANUF_ID_TI   = 0x5449;

  bool begin(uint8_t i2cAddr = 0x40);  // true si MANUFACTURER_ID == 0x5449
  uint16_t manufacturerId();

  // Mesures brutes (sans calibration, ADCRANGE = 0 par defaut)
  float busVoltage();      // V    (3,125 mV/LSB)
  float shuntVoltage();    // V    (5 uV/LSB, ADCRANGE = 0)
  float dieTemperature();  // degC (125 m degC/LSB, 12 bits utiles)

  // Calibration courant : a activer une fois R_shunt connu (datasheet du shunt).
  //   maxCurrentA : courant max attendu ; rShuntOhm : valeur du shunt (X500).
  void  configureCurrent(float maxCurrentA, float rShuntOhm);
  float current();         // A (= CURRENT_LSB * registre CURRENT)

private:
  uint8_t  _addr      = 0x40;
  float    _currentLsb = 0.0f;   // 0 tant que non calibre
  uint16_t read16(uint8_t reg);
  bool     write16(uint8_t reg, uint16_t value);
};
