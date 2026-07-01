# Tests unitaires (sur PC, sans carte)

On teste la **logique pure** (conversions, décisions), pas la lecture réelle des
capteurs — ça, ça se vérifie à la main sur la carte.

## Lancer les tests (depuis la racine du projet)

```bash
# Conversion CTN (ADC -> degC)
g++ -std=c++11 test/test_ntc_convert.cpp -o /tmp/test_ntc && /tmp/test_ntc

# Logique d'alarme (OK / avertissement / critique)
g++ -std=c++11 test/test_alarm_logic.cpp -o /tmp/test_alarm && /tmp/test_alarm
```

Sortie attendue : `== TOUS LES TESTS PASSENT ==` (code de retour 0).

## Ce qui est testé
**Conversion CTN** (`ntc_convert.h`) :
- `raw = 2048` (point milieu) → ≈ 25 °C
- sens de variation (raw plus faible → plus chaud)
- bornes 0 / 4095 → `NAN` (circuit ouvert / court-circuit)

**Logique d'alarme** (`alarm_logic.h`) :
- valeur < seuil avertissement → `OK`
- entre les deux seuils → `WARNING`
- ≥ seuil critique → `CRITICAL`
- combinaison de deux niveaux → on garde le plus grave

Les tests incluent directement les en-têtes `src/*.h` (fonctions pures, sans
Arduino) : c'est le **même code** que celui exécuté sur la carte.
