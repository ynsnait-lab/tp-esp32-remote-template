# TP Validation Électronique — Template ESP32

Template de départ pour le TP à distance.  
---
## Comment démarrer

1. **Forke** ce dépôt sur ton compte GitHub (bouton "Fork" en haut à droite)
2. Vérifie que ton fork est bien **public** (Settings → Change visibility)
3. Modifie `src/main.cpp` avec ton code
4. Connecte-toi sur la plateforme du TP
5. Onglet **Dépôt Git** → colle l'URL de ton fork → **Cloner & Programmer**

---

## Structure du projet

```
tp-esp32-remote-template/
├── platformio.ini   ← config carte, ne pas modifier
├── .gitignore       ← exclut .pio/, ne pas supprimer
└── src/
    └── main.cpp     ← ton code va ici
```

---

## Règles importantes

- Ne modifie pas `platformio.ini`
- Ne supprime pas `.gitignore` — sans lui ton repo dépassera la taille limite
- Le repo doit rester **public**
- Ne committe jamais le dossier `.pio/`

---

## Ressources

(Test issue exemple)