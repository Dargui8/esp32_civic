#include "lsu_driver.h"

#include <math.h>

namespace {

inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Coefficient d'un filtre du premier ordre de constante de temps tau, au pas dt.
inline float alphaFor(float tau, float dt) { return dt / (tau + dt); }

inline uint16_t dividerFor(float fast_Hz, float slow_Hz) {
  float d = fast_Hz / slow_Hz;
  if (d < 1.0f) d = 1.0f;
  return (uint16_t)(d + 0.5f);
}

inline bool heating(HeaterPhase p) {
  return p == HeaterPhase::Boost || p == HeaterPhase::Hold || p == HeaterPhase::Ramp ||
         p == HeaterPhase::ClosedLoop;
}

const uint8_t kPumpBlocking = PUMP_FAULT_SATURATED | PUMP_FAULT_ZERO | PUMP_FAULT_LATCHED;

}  // namespace

LsuController::LsuController(const LsuCircuit& circuit, const LsuControlConfig& control,
                             const HeaterConfig& heater)
    : circuit_(circuit), control_(control), heater_(heater) {
  reset();
}

void LsuController::reset() {
  heater_.reset();
  dt_ = 1.0f / control_.sampleRate_Hz;
  pumpDivider_ = dividerFor(control_.sampleRate_Hz, control_.pumpRate_Hz);
  heaterDivider_ = dividerFor(control_.sampleRate_Hz, control_.heaterRate_Hz);
  alphaEsr_ = alphaFor(control_.esrFilterTau_s, dt_);
  alphaPump_ = alphaFor(control_.pumpFilterTau_s, dt_);
  alphaZero_ = alphaFor(control_.zeroFilterTau_s, dt_);
  pumpCounter_ = 0;
  heaterCounter_ = 0;
  r_[0] = r_[1] = r_[2] = 0.0f;
  rCount_ = 0;
  esrHigh_ = false;
  nernstSum_ = 0.0f;
  nernstCount_ = 0;
  integral_ = 0.0f;
  saturationTimer_ = 0.0f;
  lockTimer_ = 0.0f;
  pumpFaultTimer_ = 0.0f;
  pumpRetries_ = 0;
  inactiveTime_ = 0.0f;
  stableTime_ = 0.0f;

  status_.esr_Ohm = esrFromAc(circuit_.esrDriveVpp_V);  // sonde froide jusqu'à preuve du contraire
  status_.nernst_V = 0.0f;
  status_.pumpMeasured_mA = 0.0f;
  status_.pumpCommand_mA = 0.0f;
  status_.senseZero_V = circuit_.pumpSenseZero_V;
  status_.ubat_V = NAN;
  status_.heater_V = 0.0f;
  status_.heaterDuty = 0.0f;
  status_.lambda = NAN;
  status_.range = lsu::Range::Invalid;
  status_.valid = false;
  status_.supplyOk = false;
  status_.pumpActive = false;
  status_.locked = false;
  status_.phase = heater_.phase();
  status_.phaseTime_s = 0.0f;
  status_.heaterFaults = HEATER_FAULT_NONE;
  status_.pumpFaults = PUMP_FAULT_NONE;
  status_.samples = 0;
}

float LsuController::esrFromAc(float vac) const {
  const float vpp = circuit_.esrDriveVpp_V;
  if (!(vac > 0.0f)) return 0.0f;
  if (vac >= 0.999f * vpp) return 1.0e5f;  // au-delà de ce que le diviseur peut mesurer
  const float esr = circuit_.esrDriveR_Ohm / (vpp / vac - 1.0f) - circuit_.esrSeriesR_Ohm;
  return esr > 0.0f ? esr : 0.0f;
}

float LsuController::pumpDacVolts(float ip_mA) const {
  return circuit_.pumpDacZero_V + ip_mA * circuit_.pumpDacVoltsPerMa;
}

float LsuController::pumpCurrentFromDac(float volts) const {
  return (volts - circuit_.pumpDacZero_V) / circuit_.pumpDacVoltsPerMa;
}

LsuOutputs LsuController::step(const LsuInputs& in) {
  ++status_.samples;

  // 1. Cellule de Nernst. Le créneau d'injection change de sens à chaque échantillon :
  //    sur trois échantillons, l'écart entre phases donne l'amplitude alternative (donc Ri)
  //    et leur moyenne la tension de Nernst (méthode de rusEFI, sampling.cpp).
  r_[0] = r_[1];
  r_[1] = r_[2];
  r_[2] = in.nernstAdc_V / circuit_.nernstGain;
  if (rCount_ < 3) ++rCount_;
  if (rCount_ == 3) {
    const float opposite = 0.5f * (r_[0] + r_[2]);
    const float ac = fabsf(opposite - r_[1]);
    status_.esr_Ohm += alphaEsr_ * (esrFromAc(ac) - status_.esr_Ohm);
    nernstSum_ += 0.5f * (opposite + r_[1]);
    ++nernstCount_;
  }
  esrHigh_ = !esrHigh_;

  // 2. Courant de pompe mesuré sur le shunt.
  const float ip = (in.pumpSenseAdc_V - status_.senseZero_V) * 1000.0f /
                   (circuit_.pumpSenseGain * circuit_.pumpSenseR_Ohm);
  status_.pumpMeasured_mA += alphaPump_ * (ip - status_.pumpMeasured_mA);

  // Zéro de la mesure recalé tant que la pompe est coupée (après 0,5 s de repos).
  if (status_.pumpActive) {
    inactiveTime_ = 0.0f;
  } else {
    inactiveTime_ += dt_;
    if (inactiveTime_ > 0.5f) status_.senseZero_V += alphaZero_ * (in.pumpSenseAdc_V - status_.senseZero_V);
  }

  // 3. Boucle de pompe, 4. chauffe et lambda, à leurs rythmes propres.
  if (++pumpCounter_ >= pumpDivider_) {
    pumpCounter_ = 0;
    pumpUpdate(dt_ * pumpDivider_);
  }
  if (++heaterCounter_ >= heaterDivider_) {
    heaterCounter_ = 0;
    slowUpdate(dt_ * heaterDivider_, in.ubat_V);
  }

  LsuOutputs out;
  out.esrDriveHigh = esrHigh_;
  out.pumpDuty = clampf(pumpDacVolts(status_.pumpCommand_mA) / circuit_.vcc_V, 0.0f, 1.0f);
  out.heaterDuty = heater_.duty();
  return out;
}

void LsuController::pumpUpdate(float dt) {
  if (nernstCount_ == 0) return;
  const float vn = nernstSum_ / (float)nernstCount_;
  nernstSum_ = 0.0f;
  nernstCount_ = 0;
  status_.nernst_V = vn;

  if (!status_.pumpActive) {
    integral_ = 0.0f;
    status_.pumpCommand_mA = 0.0f;
    status_.locked = false;
    lockTimer_ = 0.0f;
    saturationTimer_ = 0.0f;
    return;
  }

  // e > 0 : tension de Nernst basse, cavité trop riche en oxygène (mélange pauvre) :
  // il faut pomper davantage d'oxygène dehors, Ip augmente (Ip > 0 côté pauvre, fiche Bosch).
  const float e = control_.nernstTarget_V - vn;
  const float lim = control_.pumpLimit_mA;
  integral_ = clampf(integral_ + control_.pumpKi_mAperVs * e * dt, -lim, lim);
  const float u = control_.pumpKp_mAperV * e + integral_;
  status_.pumpCommand_mA = clampf(u, -lim, lim);

  saturationTimer_ = (fabsf(u) >= lim) ? saturationTimer_ + dt : 0.0f;

  if (fabsf(e) <= control_.lockTolerance_V) {
    lockTimer_ += dt;
    if (lockTimer_ >= control_.lockDelay_s) status_.locked = true;
  } else {
    lockTimer_ = 0.0f;
    status_.locked = false;
  }
}

void LsuController::slowUpdate(float dt, float ubat) {
  status_.ubat_V = ubat;
  heater_.update(dt, status_.esr_Ohm, ubat);

  // Défauts de la pompe.
  if (fabsf(status_.senseZero_V - circuit_.pumpSenseZero_V) > control_.zeroMaxOffset_V) {
    status_.pumpFaults |= PUMP_FAULT_ZERO;  // câblage de la mesure de Ip à vérifier
  }
  if (saturationTimer_ >= control_.pumpSaturation_s) {
    saturationTimer_ = 0.0f;
    status_.pumpFaults |= PUMP_FAULT_SATURATED;
    if (pumpRetries_ >= control_.pumpMaxRetries) status_.pumpFaults |= PUMP_FAULT_LATCHED;
    pumpFaultTimer_ = 0.0f;
  }
  if ((status_.pumpFaults & PUMP_FAULT_SATURATED) && !(status_.pumpFaults & PUMP_FAULT_LATCHED)) {
    pumpFaultTimer_ += dt;
    if (pumpFaultTimer_ >= control_.pumpFaultRetry_s) {
      status_.pumpFaults &= (uint8_t)~PUMP_FAULT_SATURATED;
      ++pumpRetries_;
    }
  }

  // Pompe autorisée seulement sonde chaude, chauffe en marche et sans défaut.
  const float threshold = status_.pumpActive ? control_.pumpDisableEsr_Ohm : control_.pumpEnableEsr_Ohm;
  const bool allowed = heating(heater_.phase()) && status_.esr_Ohm <= threshold &&
                       !(status_.pumpFaults & kPumpBlocking);
  if (allowed != status_.pumpActive) {
    status_.pumpActive = allowed;
    integral_ = 0.0f;
    status_.pumpCommand_mA = 0.0f;
    status_.locked = false;
    lockTimer_ = 0.0f;
    saturationTimer_ = 0.0f;
  }

  // Une minute de mesure stable efface le compte des tentatives.
  stableTime_ = (status_.pumpActive && status_.locked) ? stableTime_ + dt : 0.0f;
  if (stableTime_ >= 60.0f) pumpRetries_ = 0;

  status_.lambda = lsu::ipToLambda(status_.pumpMeasured_mA, &status_.range);
  status_.valid = heater_.ready() && status_.pumpActive && status_.locked &&
                  status_.range != lsu::Range::Invalid;
  status_.phase = heater_.phase();
  status_.phaseTime_s = heater_.phaseTime_s();
  status_.heaterFaults = heater_.faults();
  status_.heater_V = heater_.appliedVoltage_V();
  status_.heaterDuty = heater_.duty();
  status_.supplyOk = heater_.supplyOk();
}
