// Tests sur PC du cœur portable du lecteur lambda (aucune carte nécessaire).
//   cd lecteur-lambda/tests && make
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "format.h"
#include "heater.h"
#include "lsu_adv.h"
#include "sensor_sim.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, ...)                                     \
  do {                                                       \
    ++g_checks;                                              \
    if (!(cond)) {                                           \
      ++g_failures;                                          \
      printf("  ECHEC %s:%d : %s\n    ", __FILE__, __LINE__, #cond); \
      printf(__VA_ARGS__);                                   \
      printf("\n");                                          \
    }                                                        \
  } while (0)

// Tableau Bosch complet, recopié indépendamment du firmware (colonnes UA comprises).
struct BoschRow {
  float ip_mA, lambda, ua17, ua8;
};
static const BoschRow kRows[] = {
    {-1.38000f, 0.650f, 0.048f, 0.817f}, {-1.11000f, 0.700f, 0.332f, 0.950f},
    {-0.88000f, 0.750f, 0.574f, 1.064f}, {-0.65000f, 0.800f, 0.816f, 1.178f},
    {-0.47500f, 0.850f, 1.000f, 1.265f}, {-0.37000f, 0.880f, 1.111f, 1.317f},
    {-0.30000f, 0.900f, 1.184f, 1.351f}, {-0.16000f, 0.950f, 1.332f, 1.421f},
    {-0.07600f, 0.980f, 1.420f, 1.462f}, {-0.04800f, 0.990f, 1.449f, 1.476f},
    {-0.02000f, 1.000f, 1.479f, 1.490f}, {0.01167f, 1.030f, 1.512f, 1.506f},
    {0.03278f, 1.050f, 1.534f, 1.516f},  {0.06444f, 1.080f, 1.568f, 1.532f},
    {0.08556f, 1.100f, 1.590f, 1.542f},  {0.17000f, 1.180f, 1.679f, 1.584f},
    {0.23080f, 1.260f, 1.743f, 1.614f},  {0.36000f, 1.430f, 1.879f, 1.678f},
    {0.40148f, 1.500f, 1.922f, 1.699f},  {0.52000f, 1.700f, 2.047f, 1.758f},
    {0.54740f, 1.780f, 2.076f, 1.771f},  {0.77000f, 2.430f, 2.310f, 1.881f},
    {1.40000f, 5.000f, 2.973f, 2.193f},
};
static const int kRowCount = sizeof(kRows) / sizeof(kRows[0]);

static void testTableCopy() {
  printf("Table Bosch dans le firmware\n");
  CHECK(lsu::kBoschTableSize == kRowCount, "%d points au lieu de %d", lsu::kBoschTableSize, kRowCount);
  for (int i = 0; i < kRowCount && i < lsu::kBoschTableSize; ++i) {
    CHECK(lsu::kBoschTable[i].ip_mA == kRows[i].ip_mA && lsu::kBoschTable[i].lambda == kRows[i].lambda,
          "ligne %d differente", i);
  }
}

static void testEvaluationCircuit() {
  printf("Circuit d'evaluation : UA = 1,5 V + v x 61,9 ohm x Ip\n");
  float worst = 0.0f;
  for (int i = 0; i < kRowCount; ++i) {
    const float u17 = lsu::ipToUa_V(kRows[i].ip_mA, 17.0f, 61.9f, 1.5f);
    const float u8 = lsu::ipToUa_V(kRows[i].ip_mA, 8.0f, 61.9f, 1.5f);
    worst = fmaxf(worst, fmaxf(fabsf(u17 - kRows[i].ua17), fabsf(u8 - kRows[i].ua8)));
    // Aller-retour UA -> Ip.
    const float ip = lsu::uaToIp_mA(u17, 17.0f, 61.9f, 1.5f);
    CHECK(fabsf(ip - kRows[i].ip_mA) < 1e-5f, "Ip %f au lieu de %f", ip, kRows[i].ip_mA);
  }
  printf("  ecart maximal avec les colonnes UA de la fiche : %.2f mV\n", worst * 1000.0f);
  CHECK(worst <= 0.0006f, "ecart %.2f mV", worst * 1000.0f);
}

static void testLambdaConversion() {
  printf("Conversion Ip -> lambda\n");
  lsu::Range r;
  for (int i = 0; i < kRowCount; ++i) {
    const float l = lsu::ipToLambda(kRows[i].ip_mA, &r);
    CHECK(fabsf(l - kRows[i].lambda) < 1e-4f && r == lsu::Range::InTable, "noeud %d : %f", i, l);
    const float ip = lsu::lambdaToIp_mA(kRows[i].lambda);
    CHECK(fabsf(ip - kRows[i].ip_mA) < 1e-5f, "inverse noeud %d : %f", i, ip);
  }

  // Monotonie et aller-retour sur toute la plage.
  float prev = 0.0f;
  float worstRoundTrip = 0.0f;
  for (float ip = -1.38f; ip <= 1.40f; ip += 0.0005f) {
    const float l = lsu::ipToLambda(ip, &r);
    CHECK(l > prev, "non monotone a Ip=%f", ip);
    prev = l;
    worstRoundTrip = fmaxf(worstRoundTrip, fabsf(lsu::lambdaToIp_mA(l) - ip));
  }
  CHECK(worstRoundTrip < 2e-5f, "aller-retour %g mA", worstRoundTrip);

  // Qualité de l'interpolation en 1/lambda : on retire chaque point intérieur de la
  // zone dense (lambda 0,70 à 1,78) et on le recalcule à partir de ses voisins.
  // Comparaison avec une interpolation linéaire en lambda.
  float worstInv = 0.0f, worstLin = 0.0f;
  for (int skip = 1; skip < kRowCount - 2; ++skip) {
    lsu::IpPoint t[32];
    int n = 0;
    for (int i = 0; i < kRowCount; ++i)
      if (i != skip) t[n++] = lsu::IpPoint{kRows[i].ip_mA, kRows[i].lambda};
    const float l = lsu::interpolateLambda(t, (uint8_t)n, kRows[skip].ip_mA, &r);
    worstInv = fmaxf(worstInv, fabsf(l - kRows[skip].lambda) / kRows[skip].lambda);
    const BoschRow& a = kRows[skip - 1];
    const BoschRow& b = kRows[skip + 1];
    const float lin = a.lambda + (kRows[skip].ip_mA - a.ip_mA) / (b.ip_mA - a.ip_mA) * (b.lambda - a.lambda);
    worstLin = fmaxf(worstLin, fabsf(lin - kRows[skip].lambda) / kRows[skip].lambda);
  }
  printf("  point retire puis recalcule (lambda 0,70 a 1,78) : %.2f %% en 1/lambda, %.2f %% en lineaire\n",
         worstInv * 100.0f, worstLin * 100.0f);
  // Estimation pessimiste : sans le point retiré, l'écart entre voisins double.
  CHECK(worstInv < 0.015f, "interpolation trop grossiere : %.2f %%", worstInv * 100.0f);
  CHECK(worstInv < worstLin, "1/lambda moins bon que lineaire");

  // Hors table.
  float l = lsu::ipToLambda(-2.0f, &r);
  CHECK(r == lsu::Range::RichLimit && l == 0.65f, "riche : %f", l);
  l = lsu::ipToLambda(2.0f, &r);
  CHECK(r == lsu::Range::LeanLimit && l == 5.0f, "air : %f", l);
  l = lsu::ipToLambda(NAN, &r);
  CHECK(r == lsu::Range::Invalid && l != l, "NaN");

  // Sensibilité autour de la stoechiométrie avec v = 17.
  const float du = lsu::ipToUa_V(lsu::lambdaToIp_mA(1.001f), 17, 61.9f, 1.5f) -
                   lsu::ipToUa_V(lsu::lambdaToIp_mA(1.000f), 17, 61.9f, 1.5f);
  printf("  sensibilite a lambda 1 (v=17) : %.2f mV pour 0,001 de lambda\n", du * 1000.0f);
}

static void testOutputMap() {
  printf("Sortie Speeduino 0-5 V = AFR 10-20 (essence 14,7)\n");
  const lsu::LinearMap m = {0.0f, 10.0f / 14.7f, 5.0f, 20.0f / 14.7f};
  CHECK(fabsf(lsu::lambdaToVolts(m, 10.0f / 14.7f) - 0.0f) < 1e-5f, "0 V");
  CHECK(fabsf(lsu::lambdaToVolts(m, 20.0f / 14.7f) - 5.0f) < 1e-5f, "5 V");
  const float v1 = lsu::lambdaToVolts(m, 1.0f);
  CHECK(fabsf(v1 - 2.35f) < 1e-4f, "lambda 1 -> %f V", v1);
  // Lecture côté Speeduino : AFR = 10 + 2 × V (comme le croquis jan31a existant).
  for (float l = 0.70f; l < 1.35f; l += 0.01f) {
    const float v = lsu::lambdaToVolts(m, l);
    CHECK(fabsf((10.0f + 2.0f * v) / 14.7f - l) < 1e-5f, "lambda %f", l);
    CHECK(fabsf(lsu::voltsToLambda(m, v) - l) < 1e-5f, "inverse %f", l);
  }
  CHECK(lsu::lambdaToVolts(m, 0.5f) == 0.0f, "borne basse");
  CHECK(lsu::lambdaToVolts(m, 3.0f) == 5.0f, "borne haute");
  CHECK(lsu::lambdaToVolts(m, NAN) == 5.0f, "NaN -> tension haute");
}

static void testFormat() {
  printf("Mise en forme des nombres\n");
  char b[16];
  formatFixed(b, sizeof b, 0.998f, 3, 5);
  CHECK(strcmp(b, "0.998") == 0, "'%s'", b);
  formatFixed(b, sizeof b, 14.672f, 2, 6);
  CHECK(strcmp(b, " 14.67") == 0, "'%s'", b);
  formatFixed(b, sizeof b, 301.4f, 0, 5);
  CHECK(strcmp(b, "  301") == 0, "'%s'", b);
  formatFixed(b, sizeof b, -0.0004f, 3, 0);
  CHECK(strcmp(b, "0.000") == 0, "'%s'", b);
  formatFixed(b, sizeof b, -1.25f, 1, 0);
  CHECK(strcmp(b, "-1.3") == 0 || strcmp(b, "-1.2") == 0, "'%s'", b);
  formatFixed(b, sizeof b, NAN, 2, 5);
  CHECK(strcmp(b, "  ---") == 0, "'%s'", b);
  formatFixed(b, 4, 123.456f, 2, 0);
  CHECK(strlen(b) == 3, "tronque '%s'", b);
}

// Fait tourner la chauffe sur la sonde simulée et vérifie le cycle de la fiche.
struct RunStats {
  float tEnter[7];
  float maxBoostTime, maxHoldTime;
  float maxRampSlope;
  float maxVoltage;
  float maxDuty;
  float maxTemp;
  bool everNegativeDuty;
};

static void testHeaterCycle() {
  printf("Cycle de chauffe sur sonde simulee (fiche Bosch, page 2)\n");
  SimFrontEnd sim;
  HeaterController h;
  RunStats st;
  for (int i = 0; i < 7; ++i) st.tEnter[i] = -1.0f;
  st.maxBoostTime = st.maxHoldTime = st.maxRampSlope = st.maxVoltage = st.maxDuty = st.maxTemp = 0.0f;
  st.everNegativeDuty = false;

  const float dt = 0.01f;
  HeaterPhase prevPhase = h.phase();
  float prevTarget = 0.0f;
  float tReady = -1.0f;
  float worstLambdaErr = 0.0f;
  float riSumSq = 0.0f;
  int riCount = 0;
  float ueffSum = 0.0f;
  for (int k = 0; k < 9000; ++k) {  // 90 s
    const float t = k * dt;
    const FrontEndSample s = sim.read();
    h.update(dt, s.ri_Ohm, s.ubat_V);
    sim.heaterApplied(h.appliedVoltage_V(), dt);

    const HeaterPhase p = h.phase();
    if (p != prevPhase && st.tEnter[(int)p] < 0.0f) st.tEnter[(int)p] = t;
    if (p == HeaterPhase::Boost) st.maxBoostTime = fmaxf(st.maxBoostTime, h.phaseTime_s());
    if (p == HeaterPhase::Hold) st.maxHoldTime = fmaxf(st.maxHoldTime, h.phaseTime_s());
    if (p == HeaterPhase::Ramp && prevPhase == HeaterPhase::Ramp)
      st.maxRampSlope = fmaxf(st.maxRampSlope, (h.targetVoltage_V() - prevTarget) / dt);
    if (p == HeaterPhase::Condensation)
      CHECK(fabsf(h.targetVoltage_V() - 2.0f) < 1e-6f, "condensation a %f V", h.targetVoltage_V());
    if (p == HeaterPhase::Boost) CHECK(h.targetVoltage_V() == 12.0f, "boost a %f V", h.targetVoltage_V());
    if (p == HeaterPhase::Hold) CHECK(h.targetVoltage_V() == 9.0f, "palier a %f V", h.targetVoltage_V());
    st.maxVoltage = fmaxf(st.maxVoltage, h.appliedVoltage_V());
    st.maxDuty = fmaxf(st.maxDuty, h.duty());
    st.maxTemp = fmaxf(st.maxTemp, sim.sensorTemp_C());
    if (h.duty() < 0.0f) st.everNegativeDuty = true;
    if (h.ready() && tReady < 0.0f) tReady = t;

    // Après 20 s de régulation : Ri tenue et lambda mesuré juste.
    if (p == HeaterPhase::ClosedLoop && h.phaseTime_s() > 20.0f) {
      const float e = s.ri_Ohm / 300.0f - 1.0f;
      riSumSq += e * e;
      ++riCount;
      ueffSum += h.appliedVoltage_V();
      CHECK(h.ready(), "plus pret a t=%.2f", t);
      lsu::Range r;
      const float l = lsu::ipToLambda(lsu::uaToIp_mA(s.ua_V, 17, 61.9f, 1.5f), &r);
      worstLambdaErr = fmaxf(worstLambdaErr, fabsf(l - sim.trueLambda()));
    }
    prevPhase = p;
    prevTarget = h.targetVoltage_V();
  }

  printf("  attente moteur -> condensation : %.2f s\n", st.tEnter[(int)HeaterPhase::Condensation]);
  printf("  chauffe 12 V a %.2f s, duree %.2f s ; palier 9 V a %.2f s, duree %.2f s\n",
         st.tEnter[(int)HeaterPhase::Boost], st.maxBoostTime, st.tEnter[(int)HeaterPhase::Hold],
         st.maxHoldTime);
  printf("  rampe a %.2f s, pente maxi %.3f V/s ; regulation a %.2f s ; mesure prete a %.2f s\n",
         st.tEnter[(int)HeaterPhase::Ramp], st.maxRampSlope, st.tEnter[(int)HeaterPhase::ClosedLoop],
         tReady);
  const float riRms = riCount ? sqrtf(riSumSq / riCount) : 1.0f;
  printf("  regulation : ecart type de Ri %.2f %% ; tension moyenne %.2f V ; temperature maxi %.0f C\n",
         riRms * 100.0f, riCount ? ueffSum / riCount : 0.0f, st.maxTemp);
  printf("  erreur maximale de lambda en regulation : %.4f\n", worstLambdaErr);

  CHECK(st.tEnter[(int)HeaterPhase::Condensation] > 2.9f && st.tEnter[(int)HeaterPhase::Condensation] < 3.2f,
        "debut condensation %f", st.tEnter[(int)HeaterPhase::Condensation]);
  CHECK(st.maxBoostTime <= 8.0f + 1e-3f, "boost %f s", st.maxBoostTime);
  CHECK(st.maxHoldTime <= 1.5f + 1e-3f, "palier %f s", st.maxHoldTime);
  CHECK(st.tEnter[(int)HeaterPhase::Ramp] > 0.0f, "pas de rampe");
  CHECK(st.maxRampSlope <= 0.3f + 1e-3f, "pente %f V/s", st.maxRampSlope);
  CHECK(st.tEnter[(int)HeaterPhase::ClosedLoop] > 0.0f, "jamais en regulation");
  CHECK(st.tEnter[(int)HeaterPhase::Fault] < 0.0f, "defaut inattendu %s", heaterFaultName(h.faults()));
  CHECK(st.maxVoltage <= 12.0f + 1e-3f, "tension %f V", st.maxVoltage);
  CHECK(st.maxDuty <= 1.0f && !st.everNegativeDuty, "rapport cyclique");
  CHECK(riRms < 0.02f, "Ri mal tenue : %.2f %%", riRms * 100.0f);
  CHECK(worstLambdaErr < 0.003f, "lambda faux de %f", worstLambdaErr);
}

static void testHeaterDisturbance() {
  printf("Regulation : gaz d'echappement +150 C puis -200 C\n");
  SimFrontEnd sim;
  HeaterController h;
  const float dt = 0.01f;
  float worstAfter = 0.0f;
  for (int k = 0; k < 15000; ++k) {  // 150 s
    const float t = k * dt;
    if (k == 6000) sim.setExhaustOffset_C(150.0f);
    if (k == 10000) sim.setExhaustOffset_C(-200.0f);
    const FrontEndSample s = sim.read();
    h.update(dt, s.ri_Ohm, s.ubat_V);
    sim.heaterApplied(h.appliedVoltage_V(), dt);
    const bool settled = (t > 75.0f && t < 100.0f) || t > 125.0f;  // 15 s après chaque saut
    if (settled) worstAfter = fmaxf(worstAfter, fabsf(s.ri_Ohm / 300.0f - 1.0f));
  }
  printf("  ecart de Ri 15 s apres chaque saut : %.2f %% ; phase finale %s\n", worstAfter * 100.0f,
         heaterPhaseName(h.phase()));
  CHECK(h.phase() == HeaterPhase::ClosedLoop, "phase %s", heaterPhaseName(h.phase()));
  CHECK(worstAfter < 0.05f, "ecart %.2f %%", worstAfter * 100.0f);
}

static void testHeaterFaults() {
  printf("Defauts de chauffe\n");
  const float dt = 0.01f;

  // Ri non mesurée : aucune chauffe à l'aveugle au-delà de la condensation.
  {
    SimFrontEnd sim;
    HeaterController h;
    sim.setRiBroken(true);
    float maxAfterFault = 0.0f;
    for (int k = 0; k < 3000; ++k) {
      const FrontEndSample s = sim.read();
      h.update(dt, s.ri_Ohm, s.ubat_V);
      sim.heaterApplied(h.appliedVoltage_V(), dt);
      if (h.phase() == HeaterPhase::Fault) maxAfterFault = fmaxf(maxAfterFault, h.duty());
    }
    CHECK(h.faults() & HEATER_FAULT_RI_MISSING, "defaut Ri absente attendu (%u)", h.faults());
    CHECK(maxAfterFault == 0.0f, "chauffe pendant le defaut");
  }

  // Sonde qui ne chauffe jamais (chauffage coupé) : défaut de temps, puis blocage.
  {
    HeaterController h;
    int faultsSeen = 0;
    HeaterPhase prev = h.phase();
    for (int k = 0; k < 30000; ++k) {  // 300 s
      h.update(dt, 50000.0f, 14.0f);
      if (h.phase() == HeaterPhase::Fault && prev != HeaterPhase::Fault) ++faultsSeen;
      prev = h.phase();
    }
    printf("  sonde qui ne chauffe pas : %d defauts, %s\n", faultsSeen,
           (h.faults() & HEATER_FAULT_LATCHED) ? "chauffe bloquee" : "non bloquee");
    CHECK(h.faults() & HEATER_FAULT_WARMUP_TIMEOUT, "defaut de temps attendu");
    CHECK(h.faults() & HEATER_FAULT_LATCHED, "blocage attendu");
    CHECK(faultsSeen == 3, "%d tentatives au lieu de 3", faultsSeen);
    CHECK(h.duty() == 0.0f, "chauffe active apres blocage");
  }

  // Alimentation hors plage : chauffe coupée.
  {
    HeaterController h;
    for (int k = 0; k < 500; ++k) h.update(dt, 50000.0f, 14.0f);
    CHECK(h.phase() == HeaterPhase::Condensation && h.duty() > 0.0f, "condensation attendue");
    h.update(dt, 50000.0f, 9.0f);
    CHECK(h.duty() == 0.0f && !h.supplyOk(), "chauffe sous 9 V");
    h.update(dt, 50000.0f, 17.0f);
    CHECK(h.duty() == 0.0f && !h.supplyOk(), "chauffe sous 17 V");
  }

  // Rapport cyclique = (Ueff / Ubat)².
  CHECK(fabsf(HeaterController::dutyForVoltage(7.0f, 14.0f) - 0.25f) < 1e-6f, "7 V sous 14 V");
  CHECK(HeaterController::dutyForVoltage(12.0f, 11.0f) == 1.0f, "saturation");
  CHECK(HeaterController::dutyForVoltage(5.0f, NAN) == 0.0f, "Ubat NaN");
}

int main() {
  testTableCopy();
  testEvaluationCircuit();
  testLambdaConversion();
  testOutputMap();
  testFormat();
  testHeaterCycle();
  testHeaterDisturbance();
  testHeaterFaults();
  printf("\n%d verifications, %d echec(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
