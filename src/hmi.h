#pragma once
// HMI : LED bicolore (verte/rouge) + buzzer Murata PKLCS (PWM ~4 kHz).
namespace hmi {
  void begin();                 // configure GPIO + canal PWM
  void ledGreen(bool on);
  void ledRed(bool on);
  void beep(uint16_t ms = 200); // bip bloquant
  void selfTest();              // clignote chaque LED + un bip (controle visuel/sonore)
}
