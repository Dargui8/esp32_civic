// Stratégie de chauffe de la sonde LSU ADV.
//
// Reprend le graphe « Heater Strategy » de la fiche Bosch (page 2) :
//
//   U_H,eff
//    12 V |      ┌──────────┐ max. 8 s
//         |      │          │            rampe ≤ 0,3 V/s    point de fonctionnement
//     9 V |      │          └──┐ 1,5 s  ________/‾‾‾‾‾‾‾‾‾‾‾  puis régulation sur Ri
//         |      │             └───────/
//  1,8-2 V|──────┘  phase d'eau de condensation (durée propre à l'application)
//         └──────┴───────────────────────────────────────────── t
//               t=0
//
// La tension pilotée est la tension EFFICACE du chauffage. Avec une commande PWM
// sous la tension batterie Ubat : U_eff = Ubat × √(rapport cyclique).
//
// Code C++ pur (aucune dépendance Arduino), testé sur PC dans tests/.
#pragma once

#include <stdint.h>

enum class HeaterPhase : uint8_t {
  WaitStart = 0,  // chauffe coupée : on attend que le moteur tourne
  Condensation,   // 1,8 à 2 V : eau de condensation dans l'échappement
  Boost,          // 12 V pendant 8 s au maximum
  Hold,           // palier à 9 V pendant 1,5 s
  Ramp,           // montée d'au plus 0,3 V/s jusqu'au point de fonctionnement
  ClosedLoop,     // régulation de la température par la résistance interne Ri
  Fault           // chauffe coupée sur défaut
};

// Défauts (champ de bits), lisibles avec HeaterController::faults().
enum : uint8_t {
  HEATER_FAULT_NONE = 0,
  HEATER_FAULT_RI_MISSING = 1 << 0,      // Ri non mesurée : pas de chauffe à l'aveugle
  HEATER_FAULT_WARMUP_TIMEOUT = 1 << 1,  // point de fonctionnement jamais atteint
  HEATER_FAULT_RI_HIGH = 1 << 2,         // Ri trop haute en régulation (sonde froide, débranchée)
  HEATER_FAULT_RI_LOW = 1 << 3,          // Ri trop basse en régulation (surchauffe, court-circuit)
  HEATER_FAULT_LATCHED = 1 << 7          // trop de tentatives : bloqué jusqu'au redémarrage
};

struct HeaterConfig {
  // --- Valeurs de la fiche Bosch LSU ADV ---
  float condensationVoltage_V = 2.0f;  // « 1,8 ... 2 V »
  float boostVoltage_V = 12.0f;        // « 12 V »
  float boostTimeMax_s = 8.0f;         // « max. 8 s »
  float holdVoltage_V = 9.0f;          // « 9 V »
  float holdTime_s = 1.5f;             // « 1,5 s »
  float rampRate_Vps = 0.3f;           // « max. ramp rate = 0,3 V/s »
  float riTarget_Ohm = 300.0f;         // résistance nominale de la cellule de Nernst
  float closedLoopStart_V = 7.5f;      // alimentation H+ nominale
  float supplyMin_V = 10.8f;           // tension d'alimentation système
  float supplyMax_V = 16.5f;

  // --- Propres à l'application (la fiche les laisse au choix) ---
  float condensationTime_s = 10.0f;    // « application specific » : à adapter au montage
  float startVoltage_V = 13.2f;        // moteur considéré tournant (alternateur qui charge)
  float startDelay_s = 2.0f;

  // --- Sécurités et régulation ajoutées (absentes de la fiche) ---
  float rampVoltageMax_V = 12.0f;      // plafond de la rampe = tension du boost
  float rampTimeout_s = 20.0f;         // au-delà : défaut, la sonde ne chauffe pas
  float closedLoopMin_V = 0.0f;
  float closedLoopMax_V = 12.0f;
  // Gains réglés sur la sonde simulée (sensor_sim.h) : À REVÉRIFIER sur la vraie sonde.
  float kp_V = 4.0f;                   // gain proportionnel, en V par unité de ln(Ri/Rcible)
  float ki_Vps = 2.0f;                 // gain intégral, en V/s par unité de ln(Ri/Rcible)
  float readyTolerance = 0.15f;        // mesure déclarée exploitable à ±15 % de la Ri cible...
  float readyLossTolerance = 0.30f;    // ...et perdue au-delà de ±30 %
  float readyDelay_s = 0.5f;
  float riHighFault_Ohm = 3000.0f;
  float riLowFault_Ohm = 100.0f;
  float riFaultDelay_s = 3.0f;
  float faultRetryDelay_s = 10.0f;
  uint8_t maxRetries = 2;
  float retryResetTime_s = 60.0f;      // régulation stable : on oublie les tentatives
};

class HeaterController {
 public:
  explicit HeaterController(const HeaterConfig& cfg = HeaterConfig());

  void reset();

  // Appelée périodiquement. ri_Ohm : NaN si non mesurée. ubat_V : tension du chauffage.
  void update(float dt_s, float ri_Ohm, float ubat_V);

  HeaterPhase phase() const { return phase_; }
  float phaseTime_s() const { return phaseTime_; }
  float targetVoltage_V() const { return target_; }    // consigne U_H,eff
  float duty() const { return duty_; }                 // rapport cyclique PWM, 0 à 1
  float appliedVoltage_V() const { return applied_; }  // U_H,eff réellement appliquée
  bool ready() const { return ready_; }                // sonde au point de fonctionnement
  bool supplyOk() const { return supplyOk_; }
  uint8_t faults() const { return faults_; }
  const HeaterConfig& config() const { return cfg_; }

  // Rapport cyclique donnant la tension efficace voulue : (U_eff / Ubat)², borné à [0, 1].
  static float dutyForVoltage(float ueff_V, float ubat_V);

 private:
  void enter(HeaterPhase p);
  void fault(uint8_t code);
  void regulate(float dt_s, float ri_Ohm);

  HeaterConfig cfg_;
  HeaterPhase phase_;
  float phaseTime_;
  float startTimer_;
  float rampVoltage_;
  float integral_;
  float closedLoopOutput_;
  float readyTimer_;
  float riHighTimer_;
  float riLowTimer_;
  float target_;
  float duty_;
  float applied_;
  float lnReadyIn_;
  float lnReadyOut_;
  uint8_t faults_;
  uint8_t retries_;
  bool ready_;
  bool supplyOk_;
};

// Libellés en ASCII (les écrans n'ont pas d'accents).
const char* heaterPhaseName(HeaterPhase p);   // ex. « REGULATION »
const char* heaterPhaseShort(HeaterPhase p);  // 6 caractères au plus
const char* heaterFaultName(uint8_t faults);  // défaut principal
