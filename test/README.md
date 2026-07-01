# Tests unitaires (sur PC, sans carte)

On teste la **logique pure** (conversions, décisions), pas la lecture réelle des
capteurs — ça, ça se vérifie à la main sur la carte.

## Lancer les tests (depuis la racine du projet)

```bash
g++ -std=c++11 test/test_ntc_convert.cpp -o /tmp/t && /tmp/t   # conversion CTN
g++ -std=c++11 test/test_alarm_logic.cpp -o /tmp/t && /tmp/t   # logique d'alarme
g++ -std=c++11 test/test_ring_buffer.cpp -o /tmp/t && /tmp/t   # buffer circulaire
g++ -std=c++11 test/test_log_format.cpp  -o /tmp/t && /tmp/t   # format de log CSV
```

Sortie attendue : `== TOUS LES TESTS PASSENT ==` (code de retour 0) pour chacun.

## Ce qui est testé
| Test | En-tête pur | Cas couverts |
|---|---|---|
| `test_ntc_convert` | `ntc_convert.h` | point milieu ≈ 25 °C, sens de variation, bornes → NAN |
| `test_alarm_logic` | `alarm_logic.h` | OK / avertissement / critique, seuils, combinaison |
| `test_ring_buffer` | `ring_buffer.h` | remplissage, plafond de capacité, écrasement du plus ancien |
| `test_log_format` | `log_format.h` | ligne CSV exacte, valeurs négatives |

Les tests incluent directement les en-têtes `src/*.h` (fonctions pures, sans
Arduino) : c'est le **même code** que celui exécuté sur la carte.
