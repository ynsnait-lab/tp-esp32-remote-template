# Guide du code — firmware de validation (vulgarisation)

Un module = un bloc de la carte. Pour chaque fichier `src/` : **en clair** (à quoi
ça sert) + **données clés** (les chiffres importants). Lecture rapide avant de
plonger dans le code.

---

### 🎬 `main.cpp` — le chef d'orchestre
**En clair :** au démarrage, lance la séquence de test de tous les blocs, l'un
après l'autre, et écrit les résultats sur le câble série (qu'on lit sur le PC).
**Données clés :**
- Liaison série **115200 bauds**
- 4 étapes : [1] HMI · [2] I²C/INA237 · [3] Température · [4] Mémoire

---

### 🗺️ `pins.h` — le plan de câblage
**En clair :** la liste « quel composant est branché sur quelle broche de l'ESP32 ».
Toutes les valeurs viennent du schéma de la carte.
**Données clés :**
- I²C : SDA **21**, SCL **22** · SPI : SCK **18**, MISO **19**, MOSI **23**, CS **5**
- LED verte **14**, rouge **15**, buzzer **13** · CTN **26** et **25**

---

### 💡 `hmi.cpp` / `hmi.h` — les voyants et le bip
**En clair :** allume les LED (verte/rouge) et fait sonner le buzzer. Sert à
montrer visuellement/auditivement que la carte réagit.
**Données clés :**
- Buzzer piloté en **PWM à 4 kHz** (fréquence de résonance du buzzer Murata)
- `selfTest()` = clignote chaque LED + un bip

---

### 🌡️ `ntc.cpp` / `ntc.h` — le thermomètre analogique (×2)
**En clair :** lit les 2 sondes CTN (thermistances) sur l'entrée analogique et
convertit la valeur brute en degrés. Plus il fait chaud, plus la résistance baisse.
**Données clés :**
- Sonde **NB12K00103** : 10 kΩ à 25 °C, **Beta = 3630**
- Conversion ratiométrique : `R = 10000 × raw / (4095 − raw)`, puis formule Beta
- ⚠️ Broches sur **ADC2** → indisponibles si Bluetooth actif

---

### ⚡ `ina237.cpp` / `ina237.h` — le compteur électrique
**En clair :** mesure le courant, la tension et la température interne via le bus
I²C. C'est le cœur du « moniteur d'énergie ».
**Données clés :**
- Adresse I²C **0x40** · présence : registre `0x3E` doit valoir **0x5449**
- Résolutions : tension bus **3,125 mV**, shunt **5 µV**, température **125 m°C**
- Courant : nécessite la valeur du shunt X500 (à confirmer)

---

### 🌡️ `tmp126.cpp` / `tmp126.h` — le thermomètre numérique précis
**En clair :** capteur de température sur bus SPI, avec des alarmes (seuils haut/bas)
qu'on programme et qu'on relit.
**Données clés :**
- Présence : registre `0x0C` doit valoir **0x2126**
- Résolution **0,03125 °C** · seuils par défaut carte : bas 5 °C / haut 40 °C
- Câblage **3 fils** (SIO via résistance de 10 kΩ)

---

### 🧭 `temperature.cpp` / `temperature.h` — le superviseur température
**En clair :** rassemble les **3 thermomètres** (TMP126 + 2 CTN + puce INA237),
les affiche et vérifie qu'ils sont d'accord entre eux (validation croisée).
**Données clés :**
- 3 sources comparées · alerte si **écart > 5 °C**
- Configure aussi les alarmes du TMP126

---

### 🧠 `mem.cpp` / `mem.h` — l'inventaire mémoire
**En clair :** vérifie la quantité de mémoire disponible et teste que la PSRAM
fonctionne (écrit puis relit un bloc).
**Données clés :**
- Flash **4 Mo** (garde les données, mais ~**100 000** écritures max/secteur)
- PSRAM **2 Mo** (écriture illimitée, mais **perd tout** à la coupure)
- Test : écriture/relecture de **64 Ko** en PSRAM

---

> 📎 Références techniques détaillées : [registres.md](registres.md) (valeurs attendues
> des capteurs) et [temperature.md](temperature.md) (sous-système température).
