// Pilotage direct de la sonde LSU ADV, sans circuit d'évaluation Bosch (CJ135, LT4 ADV).
//
// Le microcontrôleur fait lui-même ce que fait le circuit Bosch :
//  1. il injecte un courant alternatif dans la cellule de Nernst (sortie logique 0/3,3 V
//     à travers une résistance) et en déduit sa résistance interne Ri, donc sa température ;
//  2. il règle le courant de pompe Ip pour tenir la tension de Nernst à 450 mV ;
//     ce courant, mesuré sur un shunt, donne lambda par la table de la fiche Bosch ;
//  3. il chauffe la sonde selon la stratégie de la fiche (heater.h) en régulant Ri.
//
// Principe et valeurs de référence : contrôleur open source rusEFI
// (github.com/mck1117/wideband, licence MIT), qui pilote les LSU 4.2, 4.9 et ADV.
// Les valeurs du circuit sont à confirmer quand l'électronique sera dessinée.
//
// Code C++ pur (aucune dépendance Arduino), testé sur PC dans tests/.
#pragma once

#include <stdint.h>

#include "heater.h"
#include "lsu_adv.h"

// Circuit analogique entre la carte et la sonde (valeurs de la carte rusEFI f0_module).
struct LsuCircuit {
  float vcc_V = 3.3f;  // pleine échelle de l'ADC et niveau haut des sorties PWM

  // Cellule de Nernst : tension UN - VM amplifiée vers l'ADC (rusEFI : NERNST_INPUT_GAIN 2.7).
  float nernstGain = 2.7f;

  // Mesure de Ri : créneau 0 / 3,3 V à travers une résistance (rusEFI : 22 kΩ, 3,3 V crête à
  // crête), autour de la masse virtuelle. Ri = R / (Vcc / Vac - 1) - Rserie.
  float esrDriveVpp_V = 3.3f;
  float esrDriveR_Ohm = 22000.0f;
  float esrSeriesR_Ohm = 0.0f;  // schéma : Nernst prise sur VM, R3 hors mesure (rusEFI : 10 Ω)

  // Mesure du courant de pompe : shunt de 61,9 Ω puis ampli x10 (rusEFI), autour d'une
  // tension de repos. Ip [mA] = (V - Vrepos) × 1000 / (gain × R).
  float pumpSenseR_Ohm = 61.9f;
  float pumpSenseGain = 10.0f;
  float pumpSenseZero_V = 1.65f;  // nominal ; recalé automatiquement tant que la pompe est coupée

  // Commande du courant de pompe : PWM filtrée (« DAC ») puis convertisseur tension-courant.
  // Schéma (docs/schema/) : source de Howland, Ip = (V - 1,65 V) / 316 Ω, soit +0,316 V/mA.
  // rusEFI : V = Vcc/2 - 0,000321162 V/µA × Ip, soit -0,321162 V/mA.
  float pumpDacZero_V = 1.65f;
  float pumpDacVoltsPerMa = 0.316f;
};

struct LsuControlConfig {
  float sampleRate_Hz = 2500.0f;     // rusEFI : 2,5 kHz ; injection Ri à la moitié
  float nernstTarget_V = 0.45f;      // rusEFI NERNST_TARGET, CJ125 : 450 mV

  // Boucle de pompe (rusEFI pump_control.cpp : kP 50, kI 10000, 500 Hz).
  float pumpRate_Hz = 500.0f;
  float pumpKp_mAperV = 50.0f;
  float pumpKi_mAperVs = 10000.0f;
  float pumpLimit_mA = 3.0f;         // ADV : -1,38 à +1,40 mA sur la plage 0,65-5 (fiche)

  // Pompe autorisée seulement à chaud : « Ip ne doit être activé qu'une fois l'élément
  // assez chaud ». rusEFI : à 200 °C sous la consigne, soit environ 2,3 kΩ pour l'ADV.
  float pumpEnableEsr_Ohm = 2300.0f;
  float pumpDisableEsr_Ohm = 3000.0f;

  // Protection de la pompe (ajoutée) : commande en butée = câblage ou sens inversé.
  float pumpSaturation_s = 0.5f;
  float pumpFaultRetry_s = 10.0f;
  uint8_t pumpMaxRetries = 2;

  // Filtres (rusEFI : alpha 0,002 sur Ri et 0,02 sur Ip, à 2,5 kHz).
  float esrFilterTau_s = 0.2f;
  float pumpFilterTau_s = 0.02f;
  float zeroFilterTau_s = 0.5f;
  float zeroMaxOffset_V = 0.15f;     // écart de zéro toléré sur la mesure de Ip

  float heaterRate_Hz = 100.0f;      // chauffe et calcul de lambda
  float lockTolerance_V = 0.10f;     // boucle accrochée si |Vn - 0,45 V| reste sous ce seuil...
  float lockDelay_s = 0.2f;          // ...pendant ce temps
};

// Défauts de la pompe (champ de bits).
enum : uint8_t {
  PUMP_FAULT_NONE = 0,
  PUMP_FAULT_SATURATED = 1 << 0,  // commande en butée trop longtemps
  PUMP_FAULT_ZERO = 1 << 1,       // zéro de la mesure de Ip hors tolérance
  PUMP_FAULT_LATCHED = 1 << 7
};

// Mesures d'un échantillon (tensions sur les broches de l'ADC).
struct LsuInputs {
  float nernstAdc_V;
  float pumpSenseAdc_V;
  float ubat_V;  // tension batterie (déjà multipliée par le pont diviseur)
};

// Commandes à appliquer jusqu'à l'échantillon suivant.
struct LsuOutputs {
  bool esrDriveHigh;  // créneau d'injection pour Ri
  float pumpDuty;     // PWM du « DAC » de pompe, 0 à 1
  float heaterDuty;   // PWM de chauffe, 0 à 1
};

// État complet, copié vers la boucle principale (écran, journal, sortie Speeduino).
struct LsuStatus {
  float esr_Ohm;
  float nernst_V;
  float pumpMeasured_mA;
  float pumpCommand_mA;
  float senseZero_V;
  float ubat_V;
  float heater_V;
  float heaterDuty;
  float lambda;
  lsu::Range range;
  bool valid;        // lambda exploitable
  bool supplyOk;     // tension batterie dans la plage de la fiche
  bool pumpActive;
  bool locked;
  HeaterPhase phase;
  float phaseTime_s;
  uint8_t heaterFaults;
  uint8_t pumpFaults;
  uint32_t samples;
};

class LsuController {
 public:
  LsuController(const LsuCircuit& circuit, const LsuControlConfig& control, const HeaterConfig& heater);

  void reset();

  // Un échantillon : traite les mesures et renvoie les commandes de la période suivante.
  LsuOutputs step(const LsuInputs& in);

  const LsuStatus& status() const { return status_; }
  const LsuCircuit& circuit() const { return circuit_; }
  const LsuControlConfig& control() const { return control_; }

  // Ri à partir de l'amplitude crête à crête mesurée sur la cellule (V).
  float esrFromAc(float vac) const;
  // Tension de la PWM de pompe pour un courant voulu, et réciproque.
  float pumpDacVolts(float ip_mA) const;
  float pumpCurrentFromDac(float volts) const;

 private:
  void pumpUpdate(float dt);
  void slowUpdate(float dt, float ubat);

  LsuCircuit circuit_;
  LsuControlConfig control_;
  HeaterController heater_;
  LsuStatus status_;

  uint16_t pumpDivider_;
  uint16_t heaterDivider_;
  uint16_t pumpCounter_;
  uint16_t heaterCounter_;
  float dt_;
  float alphaEsr_;
  float alphaPump_;
  float alphaZero_;

  float r_[3];
  uint8_t rCount_;
  bool esrHigh_;
  float nernstSum_;
  uint16_t nernstCount_;
  float integral_;
  float saturationTimer_;
  float lockTimer_;
  float pumpFaultTimer_;
  uint8_t pumpRetries_;
  float inactiveTime_;
  float stableTime_;
};
