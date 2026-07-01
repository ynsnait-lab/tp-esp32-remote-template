#pragma once
#include <cstdio>
#include <cstddef>
// =============================================================================
//  Formatage PUR d'une ligne de log au format CSV -> testable sur PC.
//  Colonnes : t_ms,temperature,vbus,courant,niveau
//  Renvoie le nombre de caracteres ecrits (comme snprintf).
// =============================================================================
inline int formatLogLine(char* out, size_t n, unsigned long t_ms,
                         float tempC, float vbus, float current, int level) {
  return snprintf(out, n, "%lu,%.2f,%.3f,%.3f,%d",
                  t_ms, tempC, vbus, current, level);
}
