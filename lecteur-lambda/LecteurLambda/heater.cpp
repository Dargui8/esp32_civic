#include "heater.h"

#include <math.h>

namespace {

inline bool validResistance(float r) { return r == r && r > 0.0f && r < 1.0e30f; }

inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

}  // namespace

HeaterController::HeaterController(const HeaterConfig& cfg) : cfg_(cfg) { reset(); }

void HeaterController::reset() {
  lnReadyIn_ = logf(1.0f + cfg_.readyTolerance);
  lnReadyOut_ = logf(1.0f + cfg_.readyLossTolerance);
  faults_ = HEATER_FAULT_NONE;
  retries_ = 0;
  supplyOk_ = false;
  rampVoltage_ = 0.0f;
  integral_ = 0.0f;
  closedLoopOutput_ = 0.0f;
  riHighTimer_ = 0.0f;
  riLowTimer_ = 0.0f;
  target_ = 0.0f;
  duty_ = 0.0f;
  applied_ = 0.0f;
  enter(HeaterPhase::WaitStart);
}

float HeaterController::dutyForVoltage(float ueff_V, float ubat_V) {
  if (!(ubat_V > 1.0f) || !(ueff_V > 0.0f)) return 0.0f;
  const float r = ueff_V / ubat_V;
  return clampf(r * r, 0.0f, 1.0f);
}

void HeaterController::enter(HeaterPhase p) {
  phase_ = p;
  phaseTime_ = 0.0f;
  ready_ = false;
  readyTimer_ = 0.0f;
  switch (p) {
    case HeaterPhase::WaitStart:
      startTimer_ = 0.0f;
      break;
    case HeaterPhase::Ramp:
      rampVoltage_ = cfg_.holdVoltage_V;
      break;
    case HeaterPhase::ClosedLoop:
      // Départ de la régulation à la tension nominale de la fiche (7,5 V).
      integral_ = cfg_.closedLoopStart_V;
      closedLoopOutput_ = cfg_.closedLoopStart_V;
      riHighTimer_ = 0.0f;
      riLowTimer_ = 0.0f;
      break;
    default:
      break;
  }
}

void HeaterController::fault(uint8_t code) {
  faults_ = code;
  if (retries_ >= cfg_.maxRetries) faults_ |= HEATER_FAULT_LATCHED;
  enter(HeaterPhase::Fault);
}

void HeaterController::update(float dt, float ri, float ubat) {
  if (!(dt > 0.0f)) return;
  if (dt > 0.5f) dt = 0.5f;  // boucle retardée : on limite le pas d'intégration

  phaseTime_ += dt;
  supplyOk_ = ubat >= cfg_.supplyMin_V && ubat <= cfg_.supplyMax_V;
  const bool riValid = validResistance(ri);
  const bool atOperatingPoint = riValid && ri <= cfg_.riTarget_Ohm;

  // 1. Transitions de la phase en cours.
  switch (phase_) {
    case HeaterPhase::WaitStart:
      if (supplyOk_ && ubat >= cfg_.startVoltage_V) {
        startTimer_ += dt;
      } else {
        startTimer_ = 0.0f;
      }
      if (startTimer_ >= cfg_.startDelay_s) enter(HeaterPhase::Condensation);
      break;

    case HeaterPhase::Condensation:
      if (phaseTime_ >= cfg_.condensationTime_s) enter(HeaterPhase::Boost);
      break;

    case HeaterPhase::Boost:
      if (!riValid) {
        fault(HEATER_FAULT_RI_MISSING);
      } else if (atOperatingPoint) {
        enter(HeaterPhase::ClosedLoop);
      } else if (phaseTime_ >= cfg_.boostTimeMax_s) {
        enter(HeaterPhase::Hold);
      }
      break;

    case HeaterPhase::Hold:
      if (!riValid) {
        fault(HEATER_FAULT_RI_MISSING);
      } else if (atOperatingPoint) {
        enter(HeaterPhase::ClosedLoop);
      } else if (phaseTime_ >= cfg_.holdTime_s) {
        enter(HeaterPhase::Ramp);
      }
      break;

    case HeaterPhase::Ramp:
      if (!riValid) {
        fault(HEATER_FAULT_RI_MISSING);
      } else if (atOperatingPoint) {
        enter(HeaterPhase::ClosedLoop);
      } else if (phaseTime_ >= cfg_.rampTimeout_s) {
        fault(HEATER_FAULT_WARMUP_TIMEOUT);
      } else if (supplyOk_) {  // pas de montée pendant que la chauffe est coupée
        rampVoltage_ += cfg_.rampRate_Vps * dt;
        if (rampVoltage_ > cfg_.rampVoltageMax_V) rampVoltage_ = cfg_.rampVoltageMax_V;
      }
      break;

    case HeaterPhase::ClosedLoop:
      if (!riValid) {
        fault(HEATER_FAULT_RI_MISSING);
      } else {
        regulate(dt, ri);
      }
      break;

    case HeaterPhase::Fault:
      if (!(faults_ & HEATER_FAULT_LATCHED) && phaseTime_ >= cfg_.faultRetryDelay_s) {
        ++retries_;
        enter(HeaterPhase::WaitStart);
      }
      break;
  }

  // 2. Consigne de la phase atteinte : une transition s'applique dès ce pas.
  switch (phase_) {
    case HeaterPhase::Condensation: target_ = cfg_.condensationVoltage_V; break;
    case HeaterPhase::Boost: target_ = cfg_.boostVoltage_V; break;
    case HeaterPhase::Hold: target_ = cfg_.holdVoltage_V; break;
    case HeaterPhase::Ramp: target_ = rampVoltage_; break;
    case HeaterPhase::ClosedLoop: target_ = closedLoopOutput_; break;
    case HeaterPhase::WaitStart:
    case HeaterPhase::Fault: target_ = 0.0f; break;
  }

  // 3. Rapport cyclique.
  if (supplyOk_) {
    duty_ = dutyForVoltage(target_, ubat);
    applied_ = ubat * sqrtf(duty_);
  } else {
    duty_ = 0.0f;  // alimentation hors plage (démarreur, surtension) : chauffe coupée
    applied_ = 0.0f;
  }
}

void HeaterController::regulate(float dt, float ri) {
  // Surveillance de la résistance interne.
  riHighTimer_ = (ri > cfg_.riHighFault_Ohm) ? riHighTimer_ + dt : 0.0f;
  riLowTimer_ = (ri < cfg_.riLowFault_Ohm) ? riLowTimer_ + dt : 0.0f;
  if (riHighTimer_ >= cfg_.riFaultDelay_s) {
    fault(HEATER_FAULT_RI_HIGH);
    return;
  }
  if (riLowTimer_ >= cfg_.riFaultDelay_s) {
    fault(HEATER_FAULT_RI_LOW);
    return;
  }

  // Ri décroît à peu près exponentiellement avec la température : l'écart en
  // logarithme est presque proportionnel à l'écart de température.
  // e > 0 : sonde trop froide, il faut chauffer davantage.
  const float e = logf(ri / cfg_.riTarget_Ohm);

  // Régulateur PI avec anti-emballement : on n'intègre pas dans le sens de la saturation,
  // ni quand la chauffe est coupée faute d'alimentation.
  const float u = integral_ + cfg_.kp_V * e;
  const bool saturatedHigh = u >= cfg_.closedLoopMax_V && e > 0.0f;
  const bool saturatedLow = u <= cfg_.closedLoopMin_V && e < 0.0f;
  if (supplyOk_ && !saturatedHigh && !saturatedLow) {
    integral_ += cfg_.ki_Vps * e * dt;
    integral_ = clampf(integral_, cfg_.closedLoopMin_V, cfg_.closedLoopMax_V);
  }
  closedLoopOutput_ = clampf(integral_ + cfg_.kp_V * e, cfg_.closedLoopMin_V, cfg_.closedLoopMax_V);

  // Mesure exploitable, avec hystérésis.
  const float ae = fabsf(e);
  if (ready_) {
    if (ae > lnReadyOut_) {
      ready_ = false;
      readyTimer_ = 0.0f;
    }
  } else if (ae <= lnReadyIn_) {
    readyTimer_ += dt;
    if (readyTimer_ >= cfg_.readyDelay_s) ready_ = true;
  } else {
    readyTimer_ = 0.0f;
  }

  if (phaseTime_ >= cfg_.retryResetTime_s) retries_ = 0;
}

const char* heaterPhaseName(HeaterPhase p) {
  switch (p) {
    case HeaterPhase::WaitStart: return "ATTENTE MOTEUR";
    case HeaterPhase::Condensation: return "CONDENSATION";
    case HeaterPhase::Boost: return "CHAUFFE 12V";
    case HeaterPhase::Hold: return "PALIER 9V";
    case HeaterPhase::Ramp: return "RAMPE";
    case HeaterPhase::ClosedLoop: return "REGULATION";
    case HeaterPhase::Fault: return "DEFAUT";
  }
  return "?";
}

const char* heaterPhaseShort(HeaterPhase p) {
  switch (p) {
    case HeaterPhase::WaitStart: return "ATTENT";
    case HeaterPhase::Condensation: return "CONDEN";
    case HeaterPhase::Boost: return "12V";
    case HeaterPhase::Hold: return "9V";
    case HeaterPhase::Ramp: return "RAMPE";
    case HeaterPhase::ClosedLoop: return "REGUL";
    case HeaterPhase::Fault: return "DEFAUT";
  }
  return "?";
}

const char* heaterFaultName(uint8_t f) {
  if (f & HEATER_FAULT_RI_MISSING) return "RI ABSENTE";
  if (f & HEATER_FAULT_WARMUP_TIMEOUT) return "CHAUFFE LENTE";
  if (f & HEATER_FAULT_RI_HIGH) return "SONDE FROIDE";
  if (f & HEATER_FAULT_RI_LOW) return "SURCHAUFFE";
  return "";
}
