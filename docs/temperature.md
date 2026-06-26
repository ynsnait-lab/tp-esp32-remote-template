# Sous-système Température — référence

Carte « Projet M1 2024 » (ESP32-PICO-D4). Trois sources de température, validées
et recoupées par le firmware (`temperature.cpp`).

## Sources

| Source | Composant | Interface | Broches | Module |
|---|---|---|---|---|
| Principale | TMP126 | SPI 3 fils | SCK=18, MISO=19, MOSI=23, CS=5 | `tmp126` |
| CTN 1 | NB12K00103 (10 kΩ) | ADC | GPIO26 (ADC2) | `ntc` |
| CTN 2 | NB12K00103 (10 kΩ) | ADC | GPIO25 (ADC2) | `ntc` |
| Puce | INA237 (T_die) | I²C | SDA=21, SCL=22 | `ina237` |

> ⚠️ GPIO25/26 sont sur **ADC2** : indisponible si le WiFi/BT est actif.

## TMP126 — registres utilisés (datasheet TI SNIS227C)

| Adr. | Registre | Accès | Format / note |
|---|---|---|---|
| 0x00 | Temp_Result | R | 14 bits [15:2], 0,03125 °C/LSB, complément à 2 |
| 0x02 | Alert_Status | R/RC | bit2=THigh, bit1=TLow, bit0=Data_Ready (lecture efface) |
| 0x03 | Configuration | R/W | bit5=Int/Comp, bit4=One-Shot, bit3=Mode, [2:0]=période |
| 0x04 | Alert_Enable | R/W | bit2=THigh_En, bit1=TLow_En |
| 0x05 | TLow_Limit | R/W | même format que Temp_Result (reset −25 °C) |
| 0x06 | THigh_Limit | R/W | même format que Temp_Result (reset +85 °C) |
| 0x07 | Hysteresis | R/W | — |
| 0x0C | Device_ID | R | **0x2126** (contrôle de présence) |

**Trame SPI** : mot de commande 16 bits = `[8]=R/W` (1=lecture, 0=écriture),
`[7:0]=sous-adresse`. Lecture : `0x0100 | reg`. Écriture : `reg`, puis 16 bits
de donnée. SPI **mode 0**. Câblage 3 fils : MOSI rejoint SIO via R501 (10 kΩ).

**Seuil ↔ °C** : `registre = (round(°C / 0,03125) << 2)` ; relecture :
`°C = (registre >> 2) × 0,03125`.

## CTN NB12K00103 — conversion ADC → °C

- R25 = 10 kΩ, **Beta (25/85) = 3630 K**, boîtier 0805 (AVX/Kyocera).
- Montage : `3V3 — R604(10k) — nœud — NTC — GND`, filtre R605(5,1k)+2×47nF.
- Conversion **ratiométrique** (Vref s'annule) :

```
R_ntc = 10000 × raw / (4095 − raw)
1/T   = 1/298,15 + (1/3630) × ln(R_ntc / 10000)
°C    = (1/T) − 273,15
```

> L'ADC de l'ESP32 est non linéaire : valeur indicative (calibrer si besoin de précision).

## INA237 — température de puce

- Registre **DIETEMP (0x06)**, bits [15:4], 125 m°C/LSB, complément à 2.
- `°C = (registre >> 4) × 0,125`.

## Validation croisée

À température ambiante stabilisée, les sources valides doivent concorder.
`temperature::report()` calcule l'écart max entre toutes les sources lues et
signale `[OK]` si ≤ 5 °C, sinon `[A VERIFIER]`.
