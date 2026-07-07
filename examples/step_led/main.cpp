// =============================================================================
//  STEP "LED" — caracteriser le cablage reel de la LED bicolore D600.
//  Schema (hmi.SchDoc) : CMD_LED_GREEN = IO14 -> anode verte (pin 1),
//                        CMD_LED_RED   = IO15 -> anode rouge (pin 3),
//                        cathodes via 150R -> GND (actif HAUT).
//
//  Boucle lente, narree au port serie : compare ce que tu VOIS a ce qui est
//  annonce.
//    Phase A : GPIO14 seul HAUT  -> attendu VERT
//    Phase B : GPIO15 seul HAUT  -> attendu ROUGE
//    Phase C : tout BAS          -> attendu ETEINT
//
//  DIAGNOSTIC :
//   - A=vert, B=rouge, C=eteint       -> cablage conforme, tout va bien.
//   - A et B = MEME couleur (orange/rouge) -> court-circuit entre les deux
//     lignes de commande (pont de soudure D600 pins 1-3 probable) : les deux
//     puces s'allument ensemble. Confirmer au multimetre (continuite 14<->15).
//   - A=eteint, B=rouge               -> puce verte HS (ou R601 ouverte).
//   - couleurs inversees              -> nets croises (contredirait le schema).
// =============================================================================
#include <Arduino.h>

static const int PIN_G = 14;   // CMD_LED_GREEN (schema)
static const int PIN_R = 15;   // CMD_LED_RED   (schema)

void setup() {
  Serial.begin(115200);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_R, OUTPUT);
  digitalWrite(PIN_G, LOW);
  digitalWrite(PIN_R, LOW);
  Serial.println("=== STEP LED : regarde la LED et compare avec l'annonce ===");
}

void loop() {
  Serial.println("[A] GPIO14 seul HAUT -> attendu : VERT");
  digitalWrite(PIN_R, LOW);
  digitalWrite(PIN_G, HIGH);
  delay(3000);

  Serial.println("[B] GPIO15 seul HAUT -> attendu : ROUGE");
  digitalWrite(PIN_G, LOW);
  digitalWrite(PIN_R, HIGH);
  delay(3000);

  Serial.println("[C] tout BAS -> attendu : ETEINT");
  digitalWrite(PIN_G, LOW);
  digitalWrite(PIN_R, LOW);
  delay(2000);
}
