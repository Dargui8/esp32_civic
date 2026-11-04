// Sonde LSU ADV simulée, avec son circuit d'évaluation.
//
// Sert à faire tourner tout le programme (écran, sortie Speeduino, cycle de chauffe)
// sans aucune électronique de sonde, et à tester la chauffe sur PC.
//
// Modèle volontairement simple, NON issu de la fiche Bosch sauf mention :
//  - chauffage résistif de 6,5 Ω (8,7 W sous 7,5 V : valeurs de la fiche) ;
//  - un seul bloc thermique, refroidi vers la température des gaz ;
//  - Ri = 300 Ω (fiche) à 780 °C, décroissance exponentielle avec la température ;
//  - lambda qui oscille lentement autour de 1, ou imposée (potentiomètre, liaison série).
#pragma once

#include <stdint.h>

#include "frontend.h"

struct SimParams {
  float gain = 17.0f;                // circuit d'évaluation simulé (voir lsu_adv.h)
  float shunt_Ohm = 61.9f;
  float uaZero_V = 1.5f;
  float heaterR_Ohm = 6.5f;
  float heatCapacity_JperK = 0.25f;
  float lossCoeff_WperK = 0.02f;
  float ambient_C = 20.0f;
  float exhaustHot_C = 350.0f;       // température des gaz moteur chaud
  float exhaustTau_s = 60.0f;        // mise en température de l'échappement
  float riAtOperating_Ohm = 300.0f;
  float operatingTemp_C = 780.0f;
  float riSlope_K = 11000.0f;        // Ri × 3 pour 100 K de moins
  float engineStart_s = 1.0f;        // instant du démarrage moteur simulé
  float ubatEngineOff_V = 12.4f;
  float ubatEngineOn_V = 14.0f;
  float uaNoise_V = 0.001f;          // bruit de mesure sur UA
};

class SimFrontEnd {
 public:
  SimFrontEnd();
  explicit SimFrontEnd(const SimParams& p);

  void begin();
  FrontEndSample read();

  // Fait avancer la simulation thermique avec la tension de chauffe appliquée.
  void heaterApplied(float ueff_V, float dt_s);

  // Lambda imposé (NaN : retour au profil automatique).
  void setLambdaOverride(float lambda) { lambdaOverride_ = lambda; }
  // Décalage de température des gaz, pour éprouver la régulation.
  void setExhaustOffset_C(float d) { exhaustOffset_ = d; }
  // Coupe la mesure de Ri (NaN), pour éprouver les défauts.
  void setRiBroken(bool broken) { riBroken_ = broken; }

  float time_s() const { return t_; }
  float sensorTemp_C() const { return temp_; }
  float trueLambda() const;
  float trueRi_Ohm() const;

 private:
  float exhaustTemp_C() const;
  float noise();

  SimParams p_;
  float t_;
  float temp_;
  float exhaustOffset_;
  float lambdaOverride_;
  uint32_t rng_;
  bool riBroken_;
};
