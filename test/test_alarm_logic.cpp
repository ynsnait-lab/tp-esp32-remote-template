// =============================================================================
//  Test unitaire de la logique d'alarme (OK / avertissement / critique).
//  Tourne SUR PC, sans carte. Compilation :
//     g++ -std=c++11 test/test_alarm_logic.cpp -o /tmp/test_alarm && /tmp/test_alarm
// =============================================================================
#include <cstdio>
#include "../src/alarm_logic.h"

static int fails = 0;

static void check(bool cond, const char* msg) {
  printf("[%s] %s\n", cond ? "OK  " : "FAIL", msg);
  if (!cond) { fails++; }
}

int main() {
  // Seuils exemple : avertissement 40, critique 60
  check(alarmLevelHigh(25.0f, 40.0f, 60.0f) == AlarmLevel::OK,       "25 < warn -> OK");
  check(alarmLevelHigh(45.0f, 40.0f, 60.0f) == AlarmLevel::WARNING,  "45 entre seuils -> WARNING");
  check(alarmLevelHigh(70.0f, 40.0f, 60.0f) == AlarmLevel::CRITICAL, "70 >= crit -> CRITICAL");
  check(alarmLevelHigh(40.0f, 40.0f, 60.0f) == AlarmLevel::WARNING,  "pile au seuil warn -> WARNING");
  check(alarmLevelHigh(60.0f, 40.0f, 60.0f) == AlarmLevel::CRITICAL, "pile au seuil crit -> CRITICAL");

  // Combinaison : on garde le plus grave
  check(alarmWorst(AlarmLevel::WARNING, AlarmLevel::CRITICAL) == AlarmLevel::CRITICAL, "worst(WARN,CRIT)=CRIT");
  check(alarmWorst(AlarmLevel::OK,      AlarmLevel::WARNING)  == AlarmLevel::WARNING,  "worst(OK,WARN)=WARN");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
