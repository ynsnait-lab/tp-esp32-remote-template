#include <Arduino.h>
#include "esp_heap_caps.h"
#include "mem.h"

namespace mem {

void report() {
  Serial.println("[5] Memoire :");

  // --- Flash interne (ESP32-PICO-D4, 4 Mo attendus) ---
  uint32_t flash = ESP.getFlashChipSize();
  Serial.printf("    Flash : %.1f Mo (%u octets) @ %u MHz  [non volatile, ~100k cycles]\n",
                flash / (1024.0 * 1024.0), flash, ESP.getFlashChipSpeed() / 1000000);

  // --- PSRAM externe (APS1604M, 2 Mo attendus, volatile) ---
  psramInit();
  if (psramFound()) {
    Serial.printf("    PSRAM : detectee, %.2f Mo (%u octets), libre=%u  [volatile]\n",
                  ESP.getPsramSize() / (1024.0 * 1024.0),
                  (unsigned)ESP.getPsramSize(), (unsigned)ESP.getFreePsram());
  } else {
    Serial.println("    PSRAM : NON detectee -> verifier alim 3V3 de U102, cablage SDIO,");
    Serial.println("            ou l'activation PSRAM cote build (BOARD_HAS_PSRAM).");
  }
}

// Ecrit un motif dans un buffer alloue en PSRAM puis le relit (integrite).
bool psramSelfTest() {
  if (!psramFound()) {
    Serial.println("    PSRAM test : ignore (PSRAM absente)");
    return false;
  }
  const size_t N = 64 * 1024;   // 64 Ko
  uint8_t* buf = (uint8_t*)heap_caps_malloc(N, MALLOC_CAP_SPIRAM);
  if (!buf) {
    Serial.println("    PSRAM test : allocation echouee");
    return false;
  }
  for (size_t i = 0; i < N; i++) buf[i] = (uint8_t)(i * 31 + 7);
  bool good = true;
  for (size_t i = 0; i < N; i++) {
    if (buf[i] != (uint8_t)(i * 31 + 7)) { good = false; break; }
  }
  heap_caps_free(buf);
  Serial.printf("    PSRAM test : %s (64 Ko ecrits puis relus)\n", good ? "OK" : "ECHEC");
  return good;
}

} // namespace mem
