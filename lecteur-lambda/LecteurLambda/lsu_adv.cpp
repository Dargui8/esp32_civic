#include "lsu_adv.h"

namespace lsu {

// Fiche Bosch LSU ADV, page 2, colonnes « IP [mA] » et « lambda ».
const IpPoint kBoschTable[] = {
    {-1.38000f, 0.650f}, {-1.11000f, 0.700f}, {-0.88000f, 0.750f}, {-0.65000f, 0.800f},
    {-0.47500f, 0.850f}, {-0.37000f, 0.880f}, {-0.30000f, 0.900f}, {-0.16000f, 0.950f},
    {-0.07600f, 0.980f}, {-0.04800f, 0.990f}, {-0.02000f, 1.000f}, {0.01167f, 1.030f},
    {0.03278f, 1.050f},  {0.06444f, 1.080f},  {0.08556f, 1.100f},  {0.17000f, 1.180f},
    {0.23080f, 1.260f},  {0.36000f, 1.430f},  {0.40148f, 1.500f},  {0.52000f, 1.700f},
    {0.54740f, 1.780f},  {0.77000f, 2.430f},  {1.40000f, 5.000f},
};
const uint8_t kBoschTableSize = sizeof(kBoschTable) / sizeof(kBoschTable[0]);

float uaToIp_mA(float ua_V, float gain, float shunt_Ohm, float uaZero_V) {
  return (ua_V - uaZero_V) * 1000.0f / (gain * shunt_Ohm);
}

float ipToUa_V(float ip_mA, float gain, float shunt_Ohm, float uaZero_V) {
  return uaZero_V + gain * shunt_Ohm * ip_mA / 1000.0f;
}

float interpolateLambda(const IpPoint* t, uint8_t n, float ip, Range* range) {
  if (ip != ip) {  // NaN
    if (range) *range = Range::Invalid;
    return ip;
  }
  if (ip <= t[0].ip_mA) {
    if (range) *range = (ip < t[0].ip_mA) ? Range::RichLimit : Range::InTable;
    return t[0].lambda;
  }
  if (ip >= t[n - 1].ip_mA) {
    if (range) *range = (ip > t[n - 1].ip_mA) ? Range::LeanLimit : Range::InTable;
    return t[n - 1].lambda;
  }
  uint8_t i = 1;
  while (ip > t[i].ip_mA) ++i;  // t[i-1].ip < ip <= t[i].ip ; s'arrête au plus tard à n-1
  const float f = (ip - t[i - 1].ip_mA) / (t[i].ip_mA - t[i - 1].ip_mA);
  const float inv0 = 1.0f / t[i - 1].lambda;
  const float inv1 = 1.0f / t[i].lambda;
  if (range) *range = Range::InTable;
  return 1.0f / (inv0 + f * (inv1 - inv0));
}

float ipToLambda(float ip_mA, Range* range) {
  return interpolateLambda(kBoschTable, kBoschTableSize, ip_mA, range);
}

float lambdaToIp_mA(float lambda) {
  const IpPoint* t = kBoschTable;
  const uint8_t n = kBoschTableSize;
  if (lambda != lambda) return lambda;
  if (lambda <= t[0].lambda) return t[0].ip_mA;
  if (lambda >= t[n - 1].lambda) return t[n - 1].ip_mA;
  uint8_t i = 1;
  while (lambda > t[i].lambda) ++i;
  const float inv0 = 1.0f / t[i - 1].lambda;
  const float inv1 = 1.0f / t[i].lambda;
  const float f = (1.0f / lambda - inv0) / (inv1 - inv0);
  return t[i - 1].ip_mA + f * (t[i].ip_mA - t[i - 1].ip_mA);
}

float lambdaToVolts(const LinearMap& m, float lambda) {
  if (lambda != lambda) return m.v1;
  float v = m.v0 + (lambda - m.lambda0) * (m.v1 - m.v0) / (m.lambda1 - m.lambda0);
  const float lo = (m.v0 < m.v1) ? m.v0 : m.v1;
  const float hi = (m.v0 < m.v1) ? m.v1 : m.v0;
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return v;
}

float voltsToLambda(const LinearMap& m, float volts) {
  return m.lambda0 + (volts - m.v0) * (m.lambda1 - m.lambda0) / (m.v1 - m.v0);
}

}  // namespace lsu
