#include "sensor_sim.h"

#include <math.h>

#include "lsu_adv.h"

SimFrontEnd::SimFrontEnd() : p_() { begin(); }

SimFrontEnd::SimFrontEnd(const SimParams& p) : p_(p) { begin(); }

void SimFrontEnd::begin() {
  t_ = 0.0f;
  temp_ = p_.ambient_C;
  exhaustOffset_ = 0.0f;
  lambdaOverride_ = NAN;
  rng_ = 12345u;
  riBroken_ = false;
}

float SimFrontEnd::noise() {
  rng_ = rng_ * 1664525u + 1013904223u;  // générateur congruentiel : répétable
  return (float)(rng_ >> 8) / 8388608.0f - 1.0f;  // [-1, 1[
}

float SimFrontEnd::exhaustTemp_C() const {
  float tex = p_.ambient_C;
  if (t_ > p_.engineStart_s) {
    const float run = t_ - p_.engineStart_s;
    tex += (p_.exhaustHot_C - p_.ambient_C) * (1.0f - expf(-run / p_.exhaustTau_s));
  }
  return tex + exhaustOffset_;
}

float SimFrontEnd::trueLambda() const {
  if (lambdaOverride_ == lambdaOverride_) return lambdaOverride_;
  const float k = 6.2831853f;
  return 1.0f + 0.12f * sinf(k * t_ / 14.0f) + 0.03f * sinf(k * t_ / 3.1f);
}

float SimFrontEnd::trueRi_Ohm() const {
  const float tk = temp_ + 273.15f;
  const float topk = p_.operatingTemp_C + 273.15f;
  const float x = p_.riSlope_K * (1.0f / tk - 1.0f / topk);
  if (x > 15.0f) return 1.0e6f;  // sonde froide : résistance très élevée
  const float ri = p_.riAtOperating_Ohm * expf(x);
  return ri > 1.0e6f ? 1.0e6f : ri;
}

void SimFrontEnd::heaterApplied(float ueff_V, float dt_s) {
  if (!(dt_s > 0.0f)) return;
  const float power = ueff_V * ueff_V / p_.heaterR_Ohm;
  const float loss = p_.lossCoeff_WperK * (temp_ - exhaustTemp_C());
  temp_ += dt_s * (power - loss) / p_.heatCapacity_JperK;
  t_ += dt_s;
}

FrontEndSample SimFrontEnd::read() {
  FrontEndSample s;
  s.ubat_V = (t_ >= p_.engineStart_s) ? p_.ubatEngineOn_V : p_.ubatEngineOff_V;
  const float ri = trueRi_Ohm();
  s.ri_Ohm = riBroken_ ? NAN : ri * (1.0f + 0.005f * noise());

  // La cellule de pompe ne fonctionne qu'à chaud : à froid, Ip reste nul.
  float ip = 0.0f;
  if (ri < 3.0f * p_.riAtOperating_Ohm) ip = lsu::lambdaToIp_mA(trueLambda());
  s.ua_V = lsu::ipToUa_V(ip, p_.gain, p_.shunt_Ohm, p_.uaZero_V) + p_.uaNoise_V * noise();
  s.ok = true;
  return s;
}
