#pragma once
// =============================================================================
//  Logique d'alarme PURE (sans Arduino/materiel) -> testable sur PC.
//  Pour une grandeur qui ne doit pas depasser un seuil (temperature, courant).
//  Deux seuils : avertissement puis critique. Correspond aux etats de la FSM
//  (OK / avertissement / critique).
// =============================================================================

enum class AlarmLevel { OK = 0, WARNING = 1, CRITICAL = 2 };

// Renvoie le niveau d'alarme selon la valeur mesuree et les 2 seuils.
inline AlarmLevel alarmLevelHigh(float value, float warnThreshold, float critThreshold) {
  if (value >= critThreshold) { return AlarmLevel::CRITICAL; }
  if (value >= warnThreshold) { return AlarmLevel::WARNING; }
  return AlarmLevel::OK;
}

// Combine deux niveaux : on conserve le plus grave.
inline AlarmLevel alarmWorst(AlarmLevel a, AlarmLevel b) {
  return (static_cast<int>(a) >= static_cast<int>(b)) ? a : b;
}
