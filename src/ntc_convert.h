#pragma once
#include <math.h>
#include <stdint.h>
// =============================================================================
//  Conversion PURE ADC -> degC pour la CTN NB12K00103 (10k a 25 degC, Beta 3630).
//  AUCUNE dependance Arduino/materiel -> compilable et TESTABLE sur PC.
//  Montage : 3V3 -- R_pullup(10k) -- noeud -- NTC -- GND (ratiometrique).
//
//  Deux conversions disponibles :
//   1. TABLE (recommandee) : table de correspondance ADC -> degC fournie par
//      l'encadrant (J. Clement) pour la NB12K00103JBB, recherche binaire +
//      interpolation lineaire, arithmetique entiere. La table integre le
//      comportement reel de l'ADC ESP32 (attenuation 11 dB, non-linearite),
//      donc plus fidele que le modele theorique.
//   2. BETA (recoupe) : modele theorique 1/T = 1/T0 + ln(R/R0)/Beta. Sert de
//      verification croisee et de solution de repli.
// =============================================================================

// ---------------------------------------------------------------------------
// 1. Conversion par TABLE (source : aide de cours, corrigee - cf. notes)
// ---------------------------------------------------------------------------
constexpr int NTC_TABLE_SIZE = 33;
// Constantes NON fournies avec la table : deduites (points 25 degC et haute
// temperature coherents avec l'ADC ESP32 en 11 dB). A CONFIRMER avec l'encadrant.
constexpr int NTC_MIN_TEMP   = -40;   // degC au premier point de la table
constexpr int NTC_TABLE_STEP = 5;     // degC entre deux points -> -40..+120 degC

// Valeurs ADC brutes attendues, de la plus froide (-40 degC) a la plus chaude.
static const uint16_t kNtcTable[NTC_TABLE_SIZE] = {
    3222, 3195, 3161, 3118, 3064, 2998, 2919, 2825, 2716, 2592,
    2455, 2306, 2147, 1983, 1816, 1650, 1489, 1335, 1190, 1056,
     934,  824,  725,  637,  560,  492,  432,  380,  335,  295,
     260,  230,  204
};

// Conversion table -> DIXIEMES de degC (entier, sans virgule flottante).
// Hors table : borne a la temperature min/max (saturation, pas d'extrapolation).
// Corrections par rapport a l'aide de cours :
//  - intermediaires d'interpolation en int32_t (au lieu d'int8_t, risque de
//    troncature/debordement) ;
//  - commentaires bornes remis dans le bon sens (raw grand = froid).
inline int16_t ntcRawToTenths(uint16_t raw) {
  // raw au-dessus du premier point = plus froid que le debut de table
  if (raw > kNtcTable[0]) {
    return (int16_t)(NTC_MIN_TEMP * 10);
  }
  // raw en dessous du dernier point = plus chaud que la fin de table
  if (raw < kNtcTable[NTC_TABLE_SIZE - 1]) {
    return (int16_t)((NTC_MIN_TEMP + (NTC_TABLE_SIZE - 1) * NTC_TABLE_STEP) * 10);
  }

  // Recherche binaire (table decroissante) : trouve left tel que
  // table[left] >= raw > table[left+1]
  int left = 0;
  int right = NTC_TABLE_SIZE - 1;
  while ((left + 1) != right) {
    int middle = (left + right) >> 1;
    if (kNtcTable[middle] <= raw) {
      right = middle;
    } else {
      left = middle;
    }
  }

  // Interpolation lineaire entre les deux points encadrants
  int32_t diff = (int32_t)kNtcTable[left] - kNtcTable[left + 1];
  int32_t dVal = (int32_t)kNtcTable[left] - raw;
  int32_t degTenths = (diff != 0) ? (dVal * NTC_TABLE_STEP * 10) / diff : 0;
  return (int16_t)((NTC_MIN_TEMP + left * NTC_TABLE_STEP) * 10 + degTenths);
}

// Enrobage float : degC, ou NAN si le raw indique un defaut de cablage
// (0 = noeud a la masse / court-circuit CTN ; pleine echelle = circuit ouvert).
// NB : l'aide de cours saturait aussi ces cas -> un circuit ouvert serait lu
// "-40 degC" et masquerait une surchauffe ; ici on remonte NAN (capteur HS).
inline float ntcRawToCelsius(int raw) {
  if (raw <= 0 || raw >= 4095) {
    return NAN;
  }
  return ntcRawToTenths((uint16_t)raw) / 10.0f;
}

// ---------------------------------------------------------------------------
// 2. Modele BETA theorique (verification croisee)
// ---------------------------------------------------------------------------
constexpr float NTC_ADC_MAX  = 4095.0f;   // ADC 12 bits
constexpr float NTC_R_PULLUP = 10000.0f;  // R604 / R606
constexpr float NTC_R0       = 10000.0f;  // R25 (10k a 25 degC)
constexpr float NTC_T0_K     = 298.15f;   // 25 degC en kelvin
constexpr float NTC_BETA     = 3630.0f;   // B(25/85)

// Renvoie la temperature en degC, ou NAN si la valeur brute est hors plage.
// Suppose un ADC ideal (0..4095 lineaire sur 0..3,3 V) -> indicatif seulement.
inline float ntcRawToCelsiusBeta(int raw) {
  if (raw <= 0 || raw >= (int)NTC_ADC_MAX) {
    return NAN;
  }
  float rNtc = NTC_R_PULLUP * raw / (NTC_ADC_MAX - raw);          // Vref annulee
  float invT = 1.0f / NTC_T0_K + (1.0f / NTC_BETA) * logf(rNtc / NTC_R0);
  return (1.0f / invT) - 273.15f;
}
