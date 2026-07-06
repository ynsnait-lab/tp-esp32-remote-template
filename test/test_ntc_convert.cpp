// =============================================================================
//  Test unitaire de la conversion CTN (ADC -> degC) : table + modele Beta.
//  Tourne SUR PC, sans carte ESP32. Compilation :
//     g++ -std=c++11 test/test_ntc_convert.cpp -o /tmp/test_ntc && /tmp/test_ntc
//  (voir test/README.md)
// =============================================================================
#include <cstdio>
#include <cmath>
#include "../src/ntc_convert.h"

static int fails = 0;

static void check(bool cond, const char* msg) {
  printf("[%s] %s\n", cond ? "OK  " : "FAIL", msg);
  if (!cond) { fails++; }
}

int main() {
  // --- Conversion par TABLE (dixiemes de degC, entier) ---

  // Points exacts de la table
  check(ntcRawToTenths(3222) == -400, "raw=3222 (table[0])  -> -40,0 degC");
  check(ntcRawToTenths(1983) ==  250, "raw=1983 (table[13]) -> +25,0 degC");
  check(ntcRawToTenths(204)  == 1200, "raw=204  (table[32]) -> +120,0 degC");

  // Interpolation entre 25 degC (1983) et 30 degC (1816)
  int16_t t = ntcRawToTenths(1900);
  check(t > 250 && t < 300, "raw=1900 -> interpole entre 25,0 et 30,0 degC");

  // Saturation hors table (pas d'extrapolation)
  check(ntcRawToTenths(3500) == -400, "raw>table[0] -> sature a -40,0 degC");
  check(ntcRawToTenths(150)  == 1200, "raw<table[32] -> sature a +120,0 degC");

  // Monotonie : un raw plus faible correspond a une temperature plus elevee
  check(ntcRawToTenths(1000) > ntcRawToTenths(3000),
        "raw plus faible => plus chaud (sens de variation)");

  // Defaut de cablage : circuit ouvert / court-circuit -> NAN (pas de saturation)
  check(std::isnan(ntcRawToCelsius(0)),    "raw=0 -> NAN (court-circuit)");
  check(std::isnan(ntcRawToCelsius(4095)), "raw=4095 -> NAN (circuit ouvert)");

  // --- Modele BETA (recoupe theorique) ---

  // Point milieu : R_ntc ~= 10k (raw ~ moitie) -> environ 25 degC
  float c25 = ntcRawToCelsiusBeta(2048);
  check(c25 > 24.0f && c25 < 26.0f, "Beta: raw=2048 -> ~25 degC (point milieu)");
  check(std::isnan(ntcRawToCelsiusBeta(0)), "Beta: raw=0 -> NAN (hors plage)");

  // --- Coherence table vs Beta en milieu de plage ---
  // Le modele Beta suppose un ADC ideal ; la table integre l'ADC reel de
  // l'ESP32 -> on tolere quelques degC d'ecart entre 10 et 40 degC.
  bool coherent = true;
  for (int i = 10; i <= 16; i++) {          // indices table 10..16 = 10..40 degC
    float tTable = ntcRawToCelsius(kNtcTable[i]);
    float tBeta  = ntcRawToCelsiusBeta(kNtcTable[i]);
    if (fabsf(tTable - tBeta) > 6.0f) { coherent = false; }
  }
  check(coherent, "table vs Beta : ecart < 6 degC entre 10 et 40 degC");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
