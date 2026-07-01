# Référence registres — capteurs de la carte « Projet M1 2024 »

Extrait des datasheets TI. Sert de référence de validation (valeurs attendues,
formats, conversions). Issue #11.

---

## INA237 — capteur courant/tension (I²C)

- **Adresse I²C : `0x40`** (broches A0 = A1 = GND).
- **Contrôle de présence : lire `0x3E` (MANUFACTURER_ID) → doit valoir `0x5449` ("TI").**
- Accès : registres 16 bits, MSB en premier (POWER sur 24 bits). C2 = complément à 2.

| Adr. | Registre | Accès | Reset | Format | Valeur attendue / conversion |
|---|---|---|---|---|---|
| `0x00` | CONFIG | R/W | `0x0000` | 16 b | bit15 RST, **bit4 ADCRANGE** (0 = ±163,84 mV, 1 = ±40,96 mV) |
| `0x01` | ADC_CONFIG | R/W | `0xFB68` | 16 b | MODE[15:12], VBUSCT[11:9], VSHCT[8:6], VTCT[5:3], AVG[2:0] |
| `0x02` | SHUNT_CAL | R/W | `0x1000` | 15 b | `819,2e6 × CURRENT_LSB × R_shunt` (× 4 si ADCRANGE = 1) |
| `0x04` | VSHUNT | R | `0x0000` | 16 b, C2 | **5 µV/LSB** (ADCRANGE = 0) ; 1,25 µV/LSB (ADCRANGE = 1) |
| `0x05` | VBUS | R | `0x0000` | 16 b | **3,125 mV/LSB** |
| `0x06` | DIETEMP | R | `0x0000` | [15:4], C2 | **125 m°C/LSB** → `°C = (reg >> 4) × 0,125` |
| `0x07` | CURRENT | R | `0x0000` | 16 b, C2 | `I[A] = CURRENT_LSB × reg` |
| `0x08` | POWER | R | `0x000000` | 24 b | `P[W] = 0,2 × CURRENT_LSB × reg` |
| `0x0B` | DIAG_ALRT | R/W | `0x0001` | 16 b | flags : MATHOF, TMPOL, SHNTOL/UL, BUSOL/UL, POL, CNVRF |
| `0x0C` | SOVL | R/W | `0x7FFF` | 16 b | seuil surtension shunt |
| `0x0D` | SUVL | R/W | `0x8000` | 16 b | seuil sous-tension shunt |
| `0x0E` | BOVL | R/W | `0x7FFF` | 16 b | seuil surtension bus (3,125 mV/LSB) |
| `0x0F` | BUVL | R/W | `0x0000` | 16 b | seuil sous-tension bus |
| `0x10` | TEMP_LIMIT | R/W | `0x7FFF` | [15:4] | seuil température die |
| `0x11` | PWR_LIMIT | R/W | `0xFFFF` | 16 b | seuil puissance |
| `0x3E` | **MANUFACTURER_ID** | R | `0x5449` | 16 b | **`0x5449` = "TI"** ✅ présence |

**Équations courant / puissance :**
- `CURRENT_LSB = I_max_attendu / 2¹⁵`
- `Current[A] = CURRENT_LSB × CURRENT`
- `Power[W]   = 0,2 × CURRENT_LSB × POWER`
- Pleine échelle shunt : ±163,84 mV (ADCRANGE = 0) ou ±40,96 mV (ADCRANGE = 1)

---

## TMP126 — capteur température (SPI 3 fils)

- **Contrôle de présence : lire `0x0C` (Device_ID) → doit valoir `0x2126`.**
- Mot de commande 16 bits : `[8] = R/W` (1 = lecture, 0 = écriture), `[7:0] = sous-adresse`.
  - Lecture : `commande = 0x0100 | reg`, puis 16 bits de donnée.
  - Écriture : `commande = reg`, puis 16 bits de donnée.
- SPI **mode 0**. Câblage 3 fils : MOSI rejoint SIO via R501 (10 kΩ).

| Adr. | Registre | Accès | Reset | Format / valeur attendue |
|---|---|---|---|---|
| `0x00` | Temp_Result | R | `0x0000` | 14 b [15:2], **0,03125 °C/LSB**, C2 |
| `0x01` | Slew_Result | R | `0x0000` | 0,03125 °C/s |
| `0x02` | Alert_Status | R/RC | `0x0000` | b7 CRC, b5 Slew, b4 THigh_St, b3 TLow_St, **b2 THigh_Flag, b1 TLow_Flag, b0 Data_Ready** (lecture efface les RC) |
| `0x03` | Configuration | R/W | `0x0006` | b8 Reset, b7 AVG, b5 Int/Comp, b4 One-Shot, b3 Mode, [2:0] période (110b = 1 Hz) |
| `0x04` | Alert_Enable | R/W | `0x0016` | b4 CRC, b3 Slew, **b2 THigh, b1 TLow**, b0 Data_Ready |
| `0x05` | TLow_Limit | R/W | `0xF380` | **= −25 °C** (format Temp) |
| `0x06` | THigh_Limit | R/W | `0x2A80` | **= +85 °C** |
| `0x07` | Hysteresis | R/W | `0x0A0A` | hystérésis |
| `0x08` | Slew_Limit | R/W | `0x0500` | seuil de pente |
| `0x09`–`0x0B` | Unique_ID1/2/3 | R | — | identifiant unique de la puce |
| `0x0C` | **Device_ID** | R | `0x2126` | **`0x2126`** ✅ présence |

**Conversion température / seuils :**
- `°C  = (reg >> 2) × 0,03125`
- `reg = round(°C / 0,03125) << 2`
- Exemples (datasheet) : 25 °C = `0x0C80` · 0 °C = `0x0000` · −25 °C = `0xF380` · 175 °C = `0x5780`

---

## Valeurs « attendues » critiques pour la validation

| Capteur | Registre à lire | Valeur attendue | Si différent |
|---|---|---|---|
| INA237 | `0x3E` (MANUFACTURER_ID) | **`0x5449`** | capteur absent / I²C KO |
| TMP126 | `0x0C` (Device_ID) | **`0x2126`** | capteur absent / SPI KO |

Ces deux lectures sont câblées dans le firmware (`INA237::begin()` et `TMP126::present()`).

---

## Sources

- INA237 — Texas Instruments, *SBOSA20A* : <https://www.ti.com/lit/ds/symlink/ina237.pdf>
- TMP126 — Texas Instruments, *SNIS227C* : <https://www.ti.com/product/TMP126>
