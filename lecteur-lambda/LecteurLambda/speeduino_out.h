// Sortie analogique vers l'entrée O2 du Speeduino (A8 sur la carte v0.4 : résistance
// série de 470 Ω, condensateurs et diodes d'écrêtage, sans pont diviseur).
#pragma once

class SpeeduinoOutput {
 public:
  void begin();
  // Tension voulue sur l'entrée du Speeduino, de 0 à 5 V.
  void setVolts(float volts);
  float volts() const { return volts_; }

 private:
  float volts_ = 0.0f;
};
