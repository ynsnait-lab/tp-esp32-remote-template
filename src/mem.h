#pragma once
// =============================================================================
//  Validation memoire de la carte "Projet M1 2024" :
//    - Flash interne ESP32-PICO-D4 : 4 Mo, NON volatile, ~100 000 cycles/secteur
//    - PSRAM externe APS1604M (U102) : 2 Mo, VOLATILE, ecriture illimitee
//  But : verifier la CAPACITE (quantite) et le bon fonctionnement de la PSRAM.
//  NB : l'endurance (cycles) ne se teste PAS en usant la vraie flash -> elle se
//       valide par le calcul + la strategie d'ecriture (batching/wear-leveling).
// =============================================================================
namespace mem {
  void report();          // affiche capacite flash + etat/capacite PSRAM
  bool psramSelfTest();    // ecriture/relecture d'un buffer en PSRAM (si presente)
}
