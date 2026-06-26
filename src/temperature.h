#pragma once
#include <Arduino.h>
#include "tmp126.h"
// =============================================================================
//  Sous-systeme TEMPERATURE de la carte "Projet M1 2024".
//  Regroupe les 3 sources de temperature et permet leur validation croisee :
//    1. TMP126   (capteur principal, SPI) + ses alarmes (seuils + ALERT)
//    2. CTN 1/2  (thermistances NB12K00103, ADC)
//    3. T_die    (temperature interne de l'INA237, fournie par l'appelant)
// =============================================================================
namespace temperature {
  extern TMP126 tmp;

  // Seuils d'alarme par defaut du TMP126 (degC) - ajustables.
  const float DEFAULT_HIGH_C = 40.0f;
  const float DEFAULT_LOW_C  = 5.0f;

  void begin();                       // init TMP126 (mode comparateur + alarmes) + NTC
  void report(float inaDieC = NAN);   // lit + affiche toutes les sources + recoupe
}
