// Sonde LSU ADV et circuit de pilotage direct simulés, échantillon par échantillon.
//
// Le programme fait tourner exactement le même code de pilotage (lsu_driver.h) sur cette
// simulation que sur la vraie sonde : essais sans électronique, et tests sur PC.
//
// Modèle volontairement simple, NON issu de la fiche Bosch sauf mention :
//  - chauffage résistif de 6,5 Ω (8,7 W sous 7,5 V : fiche), un seul bloc thermique ;
//  - Ri en fonction de la température : table de rusEFI pour la LSU ADV (300 Ω à 785 °C,
//    cohérent avec les 300 Ω de la fiche) ;
//  - cavité de mesure du premier ordre : l'oxygène en excès suit l'écart entre le courant
//    de pompe nécessaire (table de la fiche) et le courant appliqué ;
//  - tension de Nernst en tangente hyperbolique autour de 450 mV, raide comme une vraie
//    cellule. La dynamique de la cavité est calée pour que les gains de pompe de rusEFI,
//    éprouvés sur de vraies sondes, y soient stables : la simulation ne prouve PAS la
//    stabilité de la boucle de pompe sur la vraie sonde.
#pragma once

#include <stdint.h>

#include "lsu_driver.h"

struct SimParams {
  // Thermique.
  float heaterR_Ohm = 6.5f;
  float heatCapacity_JperK = 0.25f;
  float lossCoeff_WperK = 0.02f;
  float ambient_C = 20.0f;
  float exhaustHot_C = 350.0f;
  float exhaustTau_s = 60.0f;
  // Cellule de pompe et cavité.
  // Calage : Nernst raide (0,1 mA d'écart = 76 % de l'excursion) et cavité « capacitive ».
  // Dans ce modèle, avec les gains rusEFI, la boucle est stable de 0,05 à 0,2 mA pour 0,5 s ;
  // une cellule plus raide ou une cavité plus rapide la font osciller.
  float cavityTau_s = 0.5f;
  float nernstSwing_V = 0.40f;
  float nernstScale_mA = 0.1f;
  float pumpDeadTemp_C = 400.0f;  // en dessous, la cellule de pompe ne conduit pas
  float pumpFullTemp_C = 600.0f;
  // Imperfections du circuit, pour éprouver le logiciel.
  float senseZeroError_V = 0.012f;
  float adcNoise_V = 0.0015f;
  // Moteur.
  float engineStart_s = 1.0f;
  float ubatEngineOff_V = 12.4f;
  float ubatEngineOn_V = 14.0f;
};

class SimLsuHardware {
 public:
  SimLsuHardware(const LsuCircuit& circuit, const SimParams& params);
  explicit SimLsuHardware(const LsuCircuit& circuit);

  void reset();

  // Applique les commandes pendant dt, puis renvoie les mesures en fin de période.
  LsuInputs step(const LsuOutputs& out, float dt);

  void setLambdaOverride(float lambda) { lambdaOverride_ = lambda; }  // NaN : profil automatique
  void setExhaustOffset_C(float d) { exhaustOffset_ = d; }
  void setSenseZeroError_V(float e) { p_.senseZeroError_V = e; }
  void setPumpWiringReversed(bool r) { pumpReversed_ = r; }  // pour éprouver la protection

  float time_s() const { return t_; }
  float sensorTemp_C() const { return temp_; }
  float trueLambda() const;
  float trueEsr_Ohm() const;
  float pumpActual_mA() const { return pumpActual_; }
  float nernstTrue_V() const { return nernst_; }

  // Ri de la LSU ADV en fonction de la température (table rusEFI inversée).
  static float esrAtTemp(float temp_C);

 private:
  float exhaustTemp_C() const;
  float noise();

  LsuCircuit c_;
  SimParams p_;
  float t_;
  float temp_;
  float cavity_mA_;
  float nernst_;
  float pumpActual_;
  float exhaustOffset_;
  float lambdaOverride_;
  uint32_t rng_;
  bool pumpReversed_;
};
