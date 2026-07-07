#pragma once
// =============================================================================
//  Mapping des broches — carte "Projet M1 2024" (ESP32-PICO-D4)
//  Source : projet_esp32_src/30_src/ESP32/Projet Outputs/SCHEMA/ESP32_Projet.pdf
// =============================================================================
//  CONFIRMÉ = label explicite et cohérent sur plusieurs feuilles du schéma.
//  À CONFIRMER = net identifié mais numéro de GPIO à revérifier (datasheet /
//                fichier Altium) avant utilisation. Ne pas piloter ces broches
//                tant que la valeur n'est pas validée.
// =============================================================================

// --- Bus I2C (mesure courant : INA237) ---------------------------- CONFIRMÉ
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

// --- Bus SPI (mesure température : TMP126) ------------------------- CONFIRMÉ
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_SPI_CS     5   // CS du TMP126

// --- UART2 (liaison série secondaire) ----------------------------- CONFIRMÉ
#define PIN_UART2_RX  16   // GPIO16-RX2

// --- Strapping ---------------------------------------------------- CONFIRMÉ
#define PIN_BOOT_MODE  0   // GPIO0 (bouton boot)

// --- HMI : LED bicolore / buzzer ---------------------------------- CONFIRMÉ
// Schema hmi.SchDoc : D600 (598-8610-207F) anodes pilotees par les GPIO,
// cathodes via 150R (R601/R602) -> actif a l'etat HAUT. Mapping verifie :
// CMD_LED_GREEN = IO14 (pin 17), CMD_LED_RED = IO15 (pin 21).
// NB bring-up 07/07 : LED percue ROUGE quel que soit le GPIO pilote (14 ou 15)
// -> suspicion de court-circuit entre les deux lignes de commande (anodes 1/3
// de D600 adjacentes) : les 2 puces s'allument ensemble (orange percu rouge).
// A verifier : continuite D600.1 <-> D600.3 au multimetre + examples/step_led.
#define PIN_LED_GREEN  14   // CMD_LED_GREEN (IO14)
#define PIN_LED_RED    15   // CMD_LED_RED   (IO15) - aussi strapping MTDO
#define PIN_BUZZER_PWM 13   // Buzzer-PWM    (IO13) - buzzer Murata PKLCS, ~4 kHz

// --- Mesure CTN (NTC) --------------------------------------------- CONFIRMÉ
// ATTENTION : GPIO25/26 = ADC2 -> indisponible si WiFi/BT actif.
#define PIN_NTC_1      26   // MES_NTC_1 (IO26, ADC2_CH9)
#define PIN_NTC_2      25   // MES_NTC_2 (IO25, ADC2_CH8)
