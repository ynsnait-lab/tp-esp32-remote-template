#pragma once
// =============================================================================
//  Mesure des 2 thermistances CTN (NTC) via l'ADC interne.
//  Composant : AVX/Kyocera NB12K00103JBB - 10 kOhm a 25 degC, Beta(25/85)=3630 K.
//  Montage (schema hmi.SchDoc) : 3V3 -- R604(10k) -- noeud -- NTC -- GND,
//  avec R605(5,1k)+2x47nF en filtre RC vers l'ADC.
//
//  Conversion : table de correspondance ADC -> degC de l'aide de cours
//  (integre le comportement reel de l'ADC ESP32) + interpolation lineaire.
//  Le modele Beta theorique reste disponible en recoupe (cf. ntc_convert.h).
//
//  ATTENTION : GPIO25/26 sont sur ADC2 -> indisponible si WiFi/BT actif.
// =============================================================================
namespace ntc {
  void  begin();
  int   readRaw1();        // CTN 1 (GPIO26) : ADC brut 0..4095
  int   readRaw2();        // CTN 2 (GPIO25) : ADC brut 0..4095
  float readCelsius1();    // CTN 1 en degC (NAN si hors plage)
  float readCelsius2();    // CTN 2 en degC (NAN si hors plage)
  float rawToCelsius(int raw);   // conversion brute -> degC
}
