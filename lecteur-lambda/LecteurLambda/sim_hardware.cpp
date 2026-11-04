#include "sim_hardware.h"

#include <math.h>

namespace {

// rusEFI, firmware/sampling.cpp : Ri (Ω) -> température (°C) de la LSU ADV.
const float kAdvEsr[] = {53,  96,  130, 162,  184,  206,  239,  278,  300,  330,  390,
                         462, 573, 730, 950, 1200, 1500, 1900, 2500, 3500, 5000, 6000};
const float kAdvTemp[] = {1198, 982, 914, 875, 855, 838, 816, 794, 785, 771, 751,
                          732,  711, 691, 671, 653, 635, 614, 588, 562, 537, 528};
const uint8_t kAdvCount = sizeof(kAdvEsr) / sizeof(kAdvEsr[0]);

inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

}  // namespace

SimLsuHardware::SimLsuHardware(const LsuCircuit& circuit, const SimParams& params) : c_(circuit), p_(params) {
  reset();
}

SimLsuHardware::SimLsuHardware(const LsuCircuit& circuit) : c_(circuit), p_() { reset(); }

void SimLsuHardware::reset() {
  t_ = 0.0f;
  temp_ = p_.ambient_C;
  cavity_mA_ = 0.0f;
  nernst_ = 0.45f;
  pumpActual_ = 0.0f;
  exhaustOffset_ = 0.0f;
  lambdaOverride_ = NAN;
  rng_ = 12345u;
  pumpReversed_ = false;
}

float SimLsuHardware::esrAtTemp(float temp) {
  // Interpolation de ln(Ri) en fonction de la température ; prolongée sous 528 °C.
  if (temp >= kAdvTemp[0]) return kAdvEsr[0];
  for (uint8_t i = 1; i < kAdvCount; ++i) {
    if (temp >= kAdvTemp[i]) {
      const float f = (kAdvTemp[i - 1] - temp) / (kAdvTemp[i - 1] - kAdvTemp[i]);
      return expf(logf(kAdvEsr[i - 1]) + f * (logf(kAdvEsr[i]) - logf(kAdvEsr[i - 1])));
    }
  }
  const uint8_t n = kAdvCount;
  const float slope = (logf(kAdvEsr[n - 1]) - logf(kAdvEsr[n - 2])) / (kAdvTemp[n - 2] - kAdvTemp[n - 1]);
  const float x = slope * (kAdvTemp[n - 1] - temp);
  if (x > 9.0f) return 1.0e6f;
  const float esr = kAdvEsr[n - 1] * expf(x);
  return esr > 1.0e6f ? 1.0e6f : esr;
}

float SimLsuHardware::trueEsr_Ohm() const { return esrAtTemp(temp_); }

float SimLsuHardware::noise() {
  rng_ = rng_ * 1664525u + 1013904223u;  // générateur congruentiel : essais répétables
  return (float)(rng_ >> 8) / 8388608.0f - 1.0f;
}

float SimLsuHardware::exhaustTemp_C() const {
  float tex = p_.ambient_C;
  if (t_ > p_.engineStart_s) {
    tex += (p_.exhaustHot_C - p_.ambient_C) * (1.0f - expf(-(t_ - p_.engineStart_s) / p_.exhaustTau_s));
  }
  return tex + exhaustOffset_;
}

float SimLsuHardware::trueLambda() const {
  if (lambdaOverride_ == lambdaOverride_) return lambdaOverride_;
  const float k = 6.2831853f;
  return 1.0f + 0.12f * sinf(k * t_ / 14.0f) + 0.03f * sinf(k * t_ / 3.1f);
}

LsuInputs SimLsuHardware::step(const LsuOutputs& out, float dt) {
  const float ubat = (t_ >= p_.engineStart_s) ? p_.ubatEngineOn_V : p_.ubatEngineOff_V;

  // Thermique : chauffage sous Ubat × √(rapport cyclique).
  const float ueff = ubat * sqrtf(clampf(out.heaterDuty, 0.0f, 1.0f));
  temp_ += dt * (ueff * ueff / p_.heaterR_Ohm - p_.lossCoeff_WperK * (temp_ - exhaustTemp_C())) /
           p_.heatCapacity_JperK;

  // Cellule de pompe : ne conduit qu'à chaud.
  const float eta = clampf((temp_ - p_.pumpDeadTemp_C) / (p_.pumpFullTemp_C - p_.pumpDeadTemp_C), 0.0f, 1.0f);
  float commanded = (clampf(out.pumpDuty, 0.0f, 1.0f) * c_.vcc_V - c_.pumpDacZero_V) / c_.pumpDacVoltsPerMa;
  if (pumpReversed_) commanded = -commanded;
  pumpActual_ = (eta > 0.0f) ? commanded : 0.0f;

  // Cavité : l'oxygène en excès suit l'écart entre courant nécessaire et courant appliqué.
  const float required = eta * lsu::lambdaToIp_mA(trueLambda());
  cavity_mA_ += dt / p_.cavityTau_s * (required - eta * pumpActual_ - cavity_mA_);
  nernst_ = 0.45f - p_.nernstSwing_V * tanhf(cavity_mA_ / p_.nernstScale_mA);

  // Injection pour Ri : créneau autour de la masse virtuelle (Vcc / 2).
  const float esr = trueEsr_Ohm() + c_.esrSeriesR_Ohm;
  const float drive = (out.esrDriveHigh ? c_.esrDriveVpp_V : 0.0f) - 0.5f * c_.esrDriveVpp_V;
  const float current = (drive - nernst_) / (c_.esrDriveR_Ohm + esr);
  const float measured = nernst_ + current * esr;

  LsuInputs in;
  in.nernstAdc_V = clampf(measured * c_.nernstGain + p_.adcNoise_V * noise(), 0.0f, c_.vcc_V);
  in.pumpSenseAdc_V = clampf(c_.pumpSenseZero_V + p_.senseZeroError_V +
                                 c_.pumpSenseGain * c_.pumpSenseR_Ohm * pumpActual_ / 1000.0f +
                                 p_.adcNoise_V * noise(),
                             0.0f, c_.vcc_V);
  in.ubat_V = ubat;
  t_ += dt;
  return in;
}
