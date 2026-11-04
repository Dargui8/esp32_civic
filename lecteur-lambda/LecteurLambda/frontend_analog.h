// Mesures du vrai circuit d'évaluation : UA (image de Ip), UR (image de Ri), Ubat.
// Les constantes de conversion sont dans config.h, section 4.
#pragma once

#include "frontend.h"

class AnalogFrontEnd {
 public:
  void begin();
  FrontEndSample read();
  void heaterApplied(float, float) {}
  void setLambdaOverride(float) {}
};
