// =============================================================================
//  STEP "lecture CTN" — le programme minimal pour une premiere victoire.
//  Objectif : lire une thermistance et voir la valeur bouger sur le port serie.
//
//  UTILISATION : copier ce contenu dans src/main.cpp, puis Build (✓) + Upload (→)
//  dans PlatformIO, et ouvrir le moniteur serie (115200 bauds).
//  Chauffe la CTN avec le doigt -> la valeur doit changer.
// =============================================================================
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);          // resolution ADC : 0..4095
  Serial.println("Lecture CTN1 (GPIO26)");
}

void loop() {
  int raw = analogRead(26);          // CTN1 = GPIO26 (ADC2)
  Serial.print("CTN1 brut = ");
  Serial.println(raw);
  delay(500);
}
