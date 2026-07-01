// =============================================================================
//  Test unitaire du formatage de ligne de log (CSV).
//     g++ -std=c++11 test/test_log_format.cpp -o /tmp/test_log && /tmp/test_log
// =============================================================================
#include <cstdio>
#include <cstring>
#include "../src/log_format.h"

static int fails = 0;
static void check(bool cond, const char* msg) {
  printf("[%s] %s\n", cond ? "OK  " : "FAIL", msg);
  if (!cond) { fails++; }
}

int main() {
  char buf[64];
  int n = formatLogLine(buf, sizeof(buf), 1000UL, 25.0f, 3.300f, 0.500f, 1);

  check(strcmp(buf, "1000,25.00,3.300,0.500,1") == 0, "ligne CSV correcte");
  check(n == (int)strlen(buf), "valeur de retour = longueur ecrite");

  // temperature negative + niveau critique
  formatLogLine(buf, sizeof(buf), 42UL, -5.5f, 3.301f, -1.250f, 2);
  check(strcmp(buf, "42,-5.50,3.301,-1.250,2") == 0, "gere valeurs negatives");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
