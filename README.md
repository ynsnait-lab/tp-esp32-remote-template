# Validation électronique — Firmware carte ESP32 « Projet M1 2024 »

Firmware de **bring-up et de validation** d'une carte custom à base d'ESP32-PICO-D4
(moniteur d'énergie : courant, tension, température). Objectif : vérifier que
chaque bloc matériel fonctionne, mesurer, et lever des alarmes.

Plateforme : **PlatformIO + Arduino**, cible `esp32dev`, liaison série **115200 bauds**.

## Démarrage rapide

```bash
# Compiler
pio run
# Flasher + moniteur série (carte branchée)
pio run -t upload && pio device monitor -b 115200
# Tests unitaires (sur PC, sans carte)
g++ -std=c++11 test/test_alarm_logic.cpp -o /tmp/t && /tmp/t
```

## Structure

```
src/
├── main.cpp          Orchestration : bring-up (setup) + boucle de surveillance (loop)
├── pins.h            Mapping des broches (issu du schéma)
├── hmi.*             LED bicolore + buzzer
├── ina237.*          Capteur courant/tension (I²C)
├── tmp126.*          Capteur température (SPI)
├── ntc.* / ntc_convert.h   2 thermistances CTN (ADC) + conversion pure
├── temperature.*     Agrégateur 3 sources + alarmes + validation croisée
├── mem.*             Capacité flash + détection/test PSRAM
├── alarm_logic.h     Décision d'alarme pure (OK / avertissement / critique)
├── ring_buffer.h     Historique de mesures (buffer circulaire pur)
└── log_format.h      Formatage de ligne de log CSV (pur)

test/                 Tests unitaires (fonctions pures, compilables sur PC)
examples/             Mini-sketches de validation par bloc (à copier dans main.cpp)
docs/                 Guide du code, sous-système température, registres
```

## Démarche de validation

- **Logique pure** (conversions, décision d'alarme, buffer, format de log) →
  isolée dans des en-têtes sans dépendance Arduino → **testée automatiquement sur PC**.
- **Matériel** (lecture réelle des capteurs) → **vérifié à la main sur la carte**
  via les mini-sketches `examples/` (step by step : un bloc = un sketch).
- Chaque capteur a une **valeur d'identité attendue** pour prouver la communication :
  INA237 `MANUFACTURER_ID = 0x5449` (I²C), TMP126 `DEVICE_ID = 0x2126` (SPI).

## Broches principales (schéma)

| Fonction | Broche | Fonction | Broche |
|---|---|---|---|
| I²C SDA / SCL | 21 / 22 | SPI SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 |
| LED verte / rouge | 14 / 15 | Buzzer | 13 |
| CTN 1 / CTN 2 | 26 / 25 (ADC2) | — | — |

## Documentation

- [`docs/guide-code.md`](docs/guide-code.md) — schéma d'architecture + rôle de chaque module (vulgarisé)
- [`docs/temperature.md`](docs/temperature.md) — sous-système température en détail
- [`docs/registres.md`](docs/registres.md) — registres INA237 / TMP126 + valeurs attendues

## Règles du projet (TP)

- Ne pas modifier `platformio.ini` ni supprimer `.gitignore`.
- Ne jamais committer le dossier `.pio/`.
- Le dépôt doit rester **public** (clonage par la plateforme du TP).
