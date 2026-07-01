// =============================================================================
//  Test unitaire de la conversion CTN (ADC -> degC).
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
  // Point milieu : R_ntc ~= 10k (raw ~ moitie) -> environ 25 degC
  float c25 = ntcRawToCelsius(2048);
  check(c25 > 24.0f && c25 < 26.0f, "raw=2048 -> ~25 degC (point milieu)");

  // Monotonie : un raw plus faible correspond a une temperature plus elevee
  check(ntcRawToCelsius(1000) > ntcRawToCelsius(3000),
        "raw plus faible => plus chaud (sens de variation)");

  // Bornes : circuit ouvert / court-circuit -> NAN
  check(std::isnan(ntcRawToCelsius(0)),    "raw=0 -> NAN (hors plage)");
  check(std::isnan(ntcRawToCelsius(4095)), "raw=4095 -> NAN (hors plage)");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
