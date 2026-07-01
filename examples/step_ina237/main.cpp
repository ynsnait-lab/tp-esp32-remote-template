// =============================================================================
//  STEP "INA237" — valider la communication I2C avec le capteur de courant.
//  1) lit le MANUFACTURER_ID (doit valoir 0x5449 = "TI") -> prouve que l'I2C parle
//  2) affiche tension bus, tension shunt et courant toutes les secondes
//
//  UTILISATION : copier ce contenu dans src/main.cpp, Build (✓) + Upload (→),
//  puis ouvrir le moniteur serie (115200 bauds).
//
//  Cablage (schema) : SDA=21, SCL=22, adresse 0x40 (A0=A1=GND).
//  L'INA237 convertit en continu par defaut : pas de config necessaire pour lire.
// =============================================================================
#include <Arduino.h>
#include <Wire.h>

static const int     PIN_SDA = 21, PIN_SCL = 22;
static const uint8_t ADDR    = 0x40;
static const float   R_SHUNT = 0.0005f;          // shunt X500 : 0,5 mOhm (valide groupe)

// Lecture d'un registre 16 bits de l'INA237 (MSB en premier).
uint16_t ina237Read(uint8_t reg) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);                   // repeated start
  Wire.requestFrom((int)ADDR, 2);
  uint16_t hi = Wire.read();
  uint16_t lo = Wire.read();
  return (hi << 8) | lo;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  delay(5);

  uint16_t id = ina237Read(0x3E);                // registre MANUFACTURER_ID
  Serial.printf("INA237 MANUFACTURER_ID = 0x%04X (attendu 0x5449)\n", id);
  if (id == 0x5449) { Serial.println("=> I2C OK : capteur present"); }
  else              { Serial.println("=> KO : verifier cablage I2C / adresse"); }
}

void loop() {
  float   vbus   = ina237Read(0x05) * 3.125e-3f;       // VBUS : 3,125 mV/LSB
  int16_t shRaw  = (int16_t) ina237Read(0x04);         // VSHUNT (signe)
  float   vshunt = shRaw * 5.0e-6f;                    // 5 uV/LSB (ADCRANGE=0)
  float   courant = vshunt / R_SHUNT;                  // I = V / R

  Serial.printf("VBUS = %.3f V | VSHUNT = %.4f mV | I = %.3f A\n",
                vbus, vshunt * 1000.0f, courant);
  delay(1000);
}
