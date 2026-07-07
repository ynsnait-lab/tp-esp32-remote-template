// =============================================================================
//  Test unitaire du calcul de calibration INA237 (SHUNT_CAL / CURRENT_LSB).
//  Tourne SUR PC, sans carte ESP32. Compilation :
//     g++ -std=c++11 test/test_ina237_cal.cpp -o /tmp/test_cal && /tmp/test_cal
//
//  Exemple type "test individuel sans carte" : le calcul est valide contre
//  les valeurs de la datasheet et de l'aide de cours AVANT tout flash.
// =============================================================================
#include <cstdio>
#include <cmath>
#include "../src/ina237_cal.h"

static int fails = 0;

static void check(bool cond, const char* msg) {
  printf("[%s] %s\n", cond ? "OK  " : "FAIL", msg);
  if (!cond) { fails++; }
}

int main() {
  const float R_SHUNT = 0.0005f;   // X500 (PSR400ITQFF0L50) : 0,5 mOhm

  // --- Cas de l'aide de cours : Imax = 200 A ---
  // CURRENT_LSB = 200/32768 ~= 6,1035 mA ; SHUNT_CAL ~= 2500 = 0x09C4
  float lsb200 = ina237CurrentLsb(200.0f);
  check(fabsf(lsb200 - 0.0061035f) < 1e-6f, "Imax=200A -> CURRENT_LSB ~= 6,1035 mA");
  check(ina237ShuntCal(200.0f, R_SHUNT) == 2500, "Imax=200A, 0,5 mOhm -> SHUNT_CAL = 2500 (0x09C4)");

  // --- Cas du projet : Imax = 10 A (seuil critique groupe) ---
  // CURRENT_LSB = 10/32768 ~= 305 uA ; SHUNT_CAL = 819,2e6*305e-6*5e-4 = 125
  check(ina237ShuntCal(10.0f, R_SHUNT) == 125, "Imax=10A, 0,5 mOhm -> SHUNT_CAL = 125");

  // --- Coherence ADCRANGE (piege de l'aide de cours) ---
  // 200 A x 0,5 mOhm = 100 mV : OK en ADCRANGE=0 (163,84 mV),
  // IMPOSSIBLE en ADCRANGE=1 (40,96 mV) -> l'ecriture de CONFIG=0x0010
  // proposee dans l'aide est incompatible avec Imax=200 A.
  check(ina237ShuntInRange(200.0f, R_SHUNT, false),  "200A/0,5mOhm : dans la plage ADCRANGE=0");
  check(!ina237ShuntInRange(200.0f, R_SHUNT, true),  "200A/0,5mOhm : HORS plage ADCRANGE=1");
  check(ina237ShuntInRange(10.0f, R_SHUNT, true),    "10A/0,5mOhm : dans la plage ADCRANGE=1");

  // --- Facteur x4 en ADCRANGE=1 ---
  check(ina237ShuntCal(10.0f, R_SHUNT, true) == 500, "ADCRANGE=1 -> SHUNT_CAL x4 (125 -> 500)");

  // --- Robustesse : parametres invalides / debordement 15 bits ---
  check(ina237ShuntCal(0.0f, R_SHUNT) == 0,      "Imax=0 -> erreur (0)");
  check(ina237ShuntCal(10.0f, -1.0f) == 0,       "Rshunt<0 -> erreur (0)");
  check(ina237ShuntCal(10.0f, 10.0f) == 0,       "SHUNT_CAL > 15 bits -> erreur (0)");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
