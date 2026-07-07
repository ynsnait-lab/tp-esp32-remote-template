#pragma once
#include <stdint.h>
// =============================================================================
//  Calcul PUR de la calibration courant INA237 (datasheet TI SBOSA20A, eq. 1-2).
//  AUCUNE dependance Arduino/materiel -> compilable et TESTABLE sur PC.
//
//    CURRENT_LSB = I_max_attendu / 2^15            [A/LSB]
//    SHUNT_CAL   = 819,2e6 * CURRENT_LSB * R_shunt (x4 si ADCRANGE = 1)
//
//  Carte "Projet M1 2024" : shunt X500 = PSR400ITQFF0L50, R = 0,5 mOhm.
//  Utilise par INA237::configureCurrent() ; separe du driver pour pouvoir
//  valider le calcul en test individuel, sans carte (exigence du process
//  de validation : logique testable isolement du materiel).
// =============================================================================

// Pas de quantification du courant (A/LSB) pour un courant max attendu donne.
inline float ina237CurrentLsb(float maxCurrentA) {
  return maxCurrentA / 32768.0f;                    // 2^15
}

// Valeur du registre SHUNT_CAL (0x02). 15 bits utiles.
// adcRange1 = true si CONFIG.ADCRANGE = 1 (pleine echelle shunt +/-40,96 mV) :
// le facteur x4 de la datasheet s'applique alors.
// Renvoie 0 si les parametres sont invalides ou si le resultat deborde des
// 15 bits (calibration impossible -> a traiter comme une erreur).
inline uint16_t ina237ShuntCal(float maxCurrentA, float rShuntOhm,
                               bool adcRange1 = false) {
  if (maxCurrentA <= 0.0f || rShuntOhm <= 0.0f) {
    return 0;
  }
  float cal = 819.2e6f * ina237CurrentLsb(maxCurrentA) * rShuntOhm;
  if (adcRange1) {
    cal *= 4.0f;
  }
  if (cal < 1.0f || cal > 32767.0f) {               // hors 15 bits
    return 0;
  }
  return (uint16_t)cal;
}

// Verification de coherence : la tension shunt au courant max doit tenir dans
// la pleine echelle de l'ADC (163,84 mV si ADCRANGE=0, 40,96 mV si ADCRANGE=1).
inline bool ina237ShuntInRange(float maxCurrentA, float rShuntOhm,
                               bool adcRange1 = false) {
  float vShuntMax = maxCurrentA * rShuntOhm;
  return vShuntMax <= (adcRange1 ? 40.96e-3f : 163.84e-3f);
}
