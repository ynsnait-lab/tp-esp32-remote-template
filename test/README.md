# Tests unitaires (sur PC, sans carte)

On teste la **logique pure** (conversions, décisions), pas la lecture réelle des
capteurs — ça, ça se vérifie à la main sur la carte.

## Lancer le test de conversion CTN

Depuis la racine du projet :

```bash
g++ -std=c++11 test/test_ntc_convert.cpp -o /tmp/test_ntc && /tmp/test_ntc
```

Sortie attendue : `== TOUS LES TESTS PASSENT ==` (code de retour 0).

## Ce qui est testé
- `raw = 2048` (point milieu) → ≈ 25 °C
- sens de variation (raw plus faible → plus chaud)
- bornes 0 / 4095 → `NAN` (circuit ouvert / court-circuit)

Le test inclut directement `src/ntc_convert.h` (fonction pure, sans Arduino),
c'est le **même code** que celui utilisé sur la carte dans `ntc.cpp`.
