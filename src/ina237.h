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
  static const uint8_t REG_DIAG_ALRT  = 0x0B;   // flags d'alarme (lecture)
  static const uint8_t REG_SOVL       = 0x0C;   // seuil surtension shunt
  static const uint8_t REG_SUVL       = 0x0D;   // seuil sous-tension shunt
  static const uint8_t REG_BOVL       = 0x0E;   // seuil surtension bus
  static const uint8_t REG_BUVL       = 0x0F;   // seuil sous-tension bus
  static const uint8_t REG_TEMP_LIMIT = 0x10;   // seuil temperature die
  static const uint8_t REG_MANUF_ID   = 0x3E;   // doit valoir 0x5449 ("TI")
  static const uint16_t MANUF_ID_TI   = 0x5449;

  // Flags du registre DIAG_ALRT (datasheet, table 7-13)
  static const uint16_t DIAG_MATHOF = (1u << 9);  // debordement arithmetique
  static const uint16_t DIAG_TMPOL  = (1u << 7);  // sur-temperature die
  static const uint16_t DIAG_SHNTOL = (1u << 6);  // surtension shunt (sur-courant)
  static const uint16_t DIAG_SHNTUL = (1u << 5);  // sous-tension shunt
  static const uint16_t DIAG_BUSOL  = (1u << 4);  // surtension bus
  static const uint16_t DIAG_BUSUL  = (1u << 3);  // sous-tension bus
  static const uint16_t DIAG_POL    = (1u << 2);  // depassement puissance
  static const uint16_t DIAG_CNVRF  = (1u << 1);  // conversion terminee

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

  // Init ADC : mode continu bus + shunt + temperature (issue #3)
  bool initAdc();

  // Seuils d'alarme materiels (issue #5) - ADCRANGE = 0
  void setShuntOverLimit(float volts);   // SOVL : 5 uV/LSB (sur-courant)
  void setBusOverLimit(float volts);     // BOVL : 3,125 mV/LSB
  void setBusUnderLimit(float volts);    // BUVL : 3,125 mV/LSB
  void setTempLimit(float celsius);      // TEMP_LIMIT : 125 m degC/LSB, bits [15:4]

  // Remontee d'alarme : lecture des flags DIAG_ALRT (issue #6, par polling)
  uint16_t diagAlert();

private:
  uint8_t  _addr      = 0x40;
  float    _currentLsb = 0.0f;   // 0 tant que non calibre
  uint16_t read16(uint8_t reg);
  bool     write16(uint8_t reg, uint16_t value);
};
