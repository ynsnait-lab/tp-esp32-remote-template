#include <Arduino.h>

#define LED_GREEN 2   // CMD_LED_GREEN
#define LED_RED   16  // CMD_LED_RED

void setup() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  Serial.begin(115200);
  Serial.println("BOOT OK");
}

void loop() {
  digitalWrite(LED_GREEN, HIGH);
  Serial.println("LED GREEN ON");
  delay(500);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  Serial.println("LED RED ON");
  delay(500);

  digitalWrite(LED_RED, LOW);
}