#pragma once
#include <Arduino.h>
// =============================================================================
//  TMP126 - capteur de temperature SPI (datasheet TI SNIS227C).
//  Cablage carte : 3 fils (SIO unique). MOSI rejoint SIO via R501 (10k) en
//  serie, MISO lit le meme noeud -> le SPI materiel 4 fils fonctionne tel quel.
//
//  Mot de commande 16 bits : [15]=X [14]=CRC [13:10]=longueur CRC [9]=auto-inc
//                            [8]=R/W (1=lecture, 0=ecriture) [7:0]=sous-adresse.
//  -> lecture  : commande = 0x0100 | reg ; ecriture : commande = reg.
//  SPI MODE 0 (donnee echantillonnee sur front montant de SCLK).
//
//  Format temperature / seuils : 14 bits [15:2], complement a 2, 0,03125 degC/LSB.
// =============================================================================

class TMP126 {
public:
  // --- Registres (datasheet table 8-3) ---
  static const uint8_t REG_TEMP         = 0x00;  // Temp_Result (lecture)
  static const uint8_t REG_SLEW         = 0x01;  // Slew_Result
  static const uint8_t REG_ALERT_STATUS = 0x02;  // flags RC (lecture efface)
  static const uint8_t REG_CONFIG       = 0x03;  // Configuration (reset 0x0006)
  static const uint8_t REG_ALERT_ENABLE = 0x04;  // Alert_Enable (reset 0x0016)
  static const uint8_t REG_TLOW         = 0x05;  // TLow_Limit  (reset -25 degC)
  static const uint8_t REG_THIGH        = 0x06;  // THigh_Limit (reset +85 degC)
  static const uint8_t REG_HYST         = 0x07;  // Hysteresis
  static const uint8_t REG_DEVICE_ID    = 0x0C;  // doit valoir 0x2126
  static const uint16_t DEVICE_ID       = 0x2126;

  // --- Bits Alert_Status (0x02) ---
  static const uint16_t ST_THIGH      = (1u << 2);
  static const uint16_t ST_TLOW       = (1u << 1);
  static const uint16_t ST_DATA_READY = (1u << 0);

  // --- Bits Configuration (0x03) ---
  static const uint16_t CFG_INT_COMP  = (1u << 5);  // 1 = mode comparateur
  static const uint16_t CFG_ONE_SHOT  = (1u << 4);
  static const uint16_t CFG_MODE_SD   = (1u << 3);  // 1 = shutdown

  // --- Bits Alert_Enable (0x04) ---
  static const uint16_t EN_THIGH      = (1u << 2);
  static const uint16_t EN_TLOW       = (1u << 1);

  void begin();                 // SPI + broche CS
  bool present();               // DEVICE_ID == 0x2126
  uint16_t deviceId();
  float    temperature();       // degC

  // Configuration
  uint16_t config();
  void     setComparatorMode(bool comparator);  // ALERT en detecteur de seuil

  // Alarmes de temperature
  void  setHighLimit(float celsius);
  void  setLowLimit(float celsius);
  float highLimit();
  float lowLimit();
  void  enableAlerts(bool high, bool low);
  uint16_t alertStatus();       // lecture brute (efface les flags RC)

  // Acces registres generiques (issues #1 / #2)
  uint16_t readReg(uint8_t reg);
  void     writeReg(uint8_t reg, uint16_t value);

private:
  static int16_t  celsiusToCounts(float c) { return (int16_t)lroundf(c / 0.03125f); }
  static float    countsToCelsius(int16_t v) { return (v >> 2) * 0.03125f; }
};
