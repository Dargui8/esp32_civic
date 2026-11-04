// Tests sur PC du cœur portable du lecteur lambda (aucune carte nécessaire).
//   cd lecteur-lambda/tests && make
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "format.h"
#include "heater.h"
#include "lsu_adv.h"
#include "lsu_driver.h"
#include "sim_hardware.h"

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


// --- Système complet : pilotage direct sur la sonde simulée --------------------------

struct Rig {
  LsuCircuit circuit;
  LsuControlConfig control;
  HeaterConfig heater;
  LsuController ctl;
  SimLsuHardware sim;
  LsuOutputs out;
  float dt;
  Rig() : ctl(circuit, control, heater), sim(circuit), dt(1.0f / control.sampleRate_Hz) {
    out.esrDriveHigh = false;
    out.pumpDuty = ctl.pumpDacVolts(0.0f) / circuit.vcc_V;
    out.heaterDuty = 0.0f;
  }
  const LsuStatus& step() {
    out = ctl.step(sim.step(out, dt));
    return ctl.status();
  }
  long samples(float seconds) const { return (long)(seconds / dt + 0.5f); }
};

static void testEsrFormula() {
  printf("Mesure de Ri par injection (diviseur 22 kohm, creneau 3,3 V)\n");
  Rig r;
  for (float esr = 100.0f; esr <= 5000.0f; esr *= 1.5f) {
    const float rs = r.circuit.esrSeriesR_Ohm;
    const float vac = 3.3f * (esr + rs) / (22000.0f + esr + rs);
    const float back = r.ctl.esrFromAc(vac);
    CHECK(fabsf(back - esr) < 0.01f * esr, "Ri %f -> %f", esr, back);
  }
  CHECK(r.ctl.esrFromAc(0.0f) == 0.0f, "Vac nulle");
  CHECK(r.ctl.esrFromAc(3.3f) >= 1.0e5f, "diviseur sature");
  // Courants dans la cellule de Nernst avec la masse virtuelle à 1,65 V et Vn = 0,45 V.
  const float iHigh = (3.3f - 1.65f - 0.45f) / 22310.0f * 1e6f;
  const float iLow = (0.0f - 1.65f - 0.45f) / 22310.0f * 1e6f;
  printf("  courant dans la cellule de Nernst : %+.0f / %+.0f uA, alternatif +-%.0f uA, moyen %+.0f uA\n", iHigh,
         iLow, (iHigh - iLow) / 2, (iHigh + iLow) / 2);
}

static void testFullCycle() {
  printf("Cycle complet : chauffe de la fiche, Ri mesuree, pompe, lambda\n");
  Rig r;
  float tEnter[7];
  for (int i = 0; i < 7; ++i) tEnter[i] = -1.0f;
  HeaterPhase prev = r.ctl.status().phase;
  float tPump = -1, tLock = -1, tValid = -1;
  float worstEsrErr = 0, worstLambda = 0, nernstSq = 0, maxHeater = 0;
  float pumpWhileCold = 0;
  int n = 0;
  for (long k = 0; k < r.samples(90.0f); ++k) {
    const float t = k * r.dt;
    const LsuStatus& s = r.step();
    if (s.phase != prev && tEnter[(int)s.phase] < 0) tEnter[(int)s.phase] = t;
    prev = s.phase;
    if (s.pumpActive && tPump < 0) tPump = t;
    if (s.locked && tLock < 0) tLock = t;
    if (s.valid && tValid < 0) tValid = t;
    if (r.sim.trueEsr_Ohm() > 3000.0f) pumpWhileCold = fmaxf(pumpWhileCold, fabsf(r.sim.pumpActual_mA()));
    maxHeater = fmaxf(maxHeater, s.heater_V);
    if (t > 60.0f) {
      CHECK(s.valid, "mesure perdue a t=%.3f", t);
      worstEsrErr = fmaxf(worstEsrErr, fabsf(s.esr_Ohm / r.sim.trueEsr_Ohm() - 1.0f));
      worstLambda = fmaxf(worstLambda, fabsf(s.lambda - r.sim.trueLambda()));
      nernstSq += (s.nernst_V - 0.45f) * (s.nernst_V - 0.45f);
      ++n;
    }
  }
  const LsuStatus& s = r.ctl.status();
  printf("  condensation %.2f s, 12 V %.2f s, 9 V %.2f s, rampe %.2f s, regulation %.2f s\n",
         tEnter[(int)HeaterPhase::Condensation], tEnter[(int)HeaterPhase::Boost], tEnter[(int)HeaterPhase::Hold],
         tEnter[(int)HeaterPhase::Ramp], tEnter[(int)HeaterPhase::ClosedLoop]);
  printf("  pompe active %.2f s, accrochee %.2f s, mesure valide %.2f s\n", tPump, tLock, tValid);
  printf("  Ri mesuree / vraie : ecart max %.1f %% ; Nernst a %.1f mV rms de 450 mV ; zero de Ip %.4f V\n",
         worstEsrErr * 100, 1000 * sqrtf(nernstSq / n), s.senseZero_V);
  printf("  erreur max de lambda (profil variable) : %.4f ; tension de chauffe max %.2f V\n", worstLambda, maxHeater);
  CHECK(tEnter[(int)HeaterPhase::ClosedLoop] > 0 && tEnter[(int)HeaterPhase::Fault] < 0, "chauffe");
  CHECK(tPump > tEnter[(int)HeaterPhase::Boost], "pompe avant la chauffe");
  CHECK(pumpWhileCold == 0.0f, "courant de pompe sonde froide : %f mA", pumpWhileCold);
  CHECK(tValid > 0 && tValid < 40.0f, "mesure valide a %f s", tValid);
  CHECK(worstEsrErr < 0.05f, "Ri faussee de %.1f %%", worstEsrErr * 100);
  CHECK(fabsf(s.senseZero_V - (1.65f + 0.012f)) < 0.002f, "zero %f", s.senseZero_V);
  CHECK(worstLambda < 0.02f, "lambda faux de %f", worstLambda);
  CHECK(maxHeater <= 12.0f + 1e-3f, "chauffe %f V", maxHeater);
  CHECK(s.pumpFaults == PUMP_FAULT_NONE && s.heaterFaults == HEATER_FAULT_NONE, "defauts %u %u", s.pumpFaults,
        s.heaterFaults);
}

static void testLambdaAccuracy() {
  printf("Lambda fixe : ecart moyen apres 45 s\n");
  const float lambdas[] = {0.70f, 0.80f, 0.90f, 1.00f, 1.10f, 1.30f, 1.70f, 3.00f};
  for (float lam : lambdas) {
    Rig r;
    r.sim.setLambdaOverride(lam);
    double sum = 0;
    int n = 0;
    for (long k = 0; k < r.samples(50.0f); ++k) {
      const LsuStatus& s = r.step();
      if (k * r.dt > 45.0f) {
        sum += s.lambda;
        ++n;
      }
    }
    const float err = (float)(sum / n) - lam;
    printf("  lambda %.2f : mesure %.4f (%+.4f)\n", lam, sum / n, err);
    // Écart systématique attendu : les 20 µA moyens de l'injection décalent la tension de
    // Nernst mesurée d'environ 6 mV (rusEFI ne le corrige pas non plus).
    CHECK(fabsf(err) < 0.003f * lam + 0.0005f, "lambda %.2f : ecart %f", lam, err);
  }
}

static void testStepResponse() {
  printf("Reponse a un saut de lambda 0,90 -> 1,10\n");
  Rig r;
  r.sim.setLambdaOverride(0.90f);
  for (long k = 0; k < r.samples(45.0f); ++k) r.step();
  r.sim.setLambdaOverride(1.10f);
  float t90 = -1;
  for (long k = 0; k < r.samples(1.0f); ++k) {
    const LsuStatus& s = r.step();
    if (t90 < 0 && s.lambda >= 0.90f + 0.9f * 0.20f) t90 = k * r.dt;
  }
  printf("  90 %% du saut atteints en %.0f ms (filtre de Ip compris)\n", t90 * 1000);
  CHECK(t90 > 0 && t90 < 0.15f, "reponse %f s", t90);
  CHECK(r.ctl.status().valid, "mesure perdue pendant le saut");
}

static void testDisturbance() {
  printf("Regulation de chauffe : gaz +150 C puis -200 C\n");
  Rig r;
  float worst = 0;
  for (long k = 0; k < r.samples(150.0f); ++k) {
    const float t = k * r.dt;
    if (k == r.samples(60.0f)) r.sim.setExhaustOffset_C(150.0f);
    if (k == r.samples(100.0f)) r.sim.setExhaustOffset_C(-200.0f);
    r.step();
    if ((t > 75.0f && t < 100.0f) || t > 125.0f) worst = fmaxf(worst, fabsf(r.sim.trueEsr_Ohm() / 300.0f - 1.0f));
  }
  printf("  ecart de Ri 15 s apres chaque saut : %.2f %% ; phase finale %s\n", worst * 100,
         heaterPhaseName(r.ctl.status().phase));
  CHECK(r.ctl.status().phase == HeaterPhase::ClosedLoop && r.ctl.status().valid, "regulation perdue");
  CHECK(worst < 0.05f, "ecart %.2f %%", worst * 100);
}

static void testPumpProtections() {
  printf("Protections de la pompe\n");
  // Convertisseur de pompe câblé à l'envers : la boucle part en butée.
  {
    Rig r;
    r.sim.setPumpWiringReversed(true);
    int faults = 0;
    bool prevSat = false;
    float worstPump = 0;
    for (long k = 0; k < r.samples(120.0f); ++k) {
      const LsuStatus& s = r.step();
      const bool sat = s.pumpFaults & PUMP_FAULT_SATURATED;
      if (sat && !prevSat) ++faults;
      // Un échantillon de latence : le courant est coupé dès l'échantillon suivant.
      if (sat && prevSat) worstPump = fmaxf(worstPump, fabsf(r.sim.pumpActual_mA()));
      prevSat = sat;
      CHECK(!s.valid, "mesure declaree valide avec la pompe inversee");
    }
    const LsuStatus& s = r.ctl.status();
    printf("  pompe inversee : %d mises en defaut, %s\n", faults,
           (s.pumpFaults & PUMP_FAULT_LATCHED) ? "pompe bloquee" : "non bloquee");
    CHECK(s.pumpFaults & PUMP_FAULT_LATCHED, "blocage attendu");
    CHECK(faults == 3, "%d mises en defaut au lieu de 3", faults);
    CHECK(worstPump == 0.0f, "courant de pompe pendant le defaut : %f", worstPump);
    CHECK(!s.pumpActive && s.pumpCommand_mA == 0.0f, "pompe encore commandee");
  }
  // Zéro de la mesure de Ip très faux : pompe jamais activée.
  {
    Rig r;
    r.sim.setSenseZeroError_V(0.3f);
    bool everActive = false;
    for (long k = 0; k < r.samples(60.0f); ++k) everActive |= r.step().pumpActive;
    printf("  zero de mesure decale de 0,3 V : defaut %u, pompe %s\n", r.ctl.status().pumpFaults,
           everActive ? "activee" : "jamais activee");
    CHECK(r.ctl.status().pumpFaults & PUMP_FAULT_ZERO, "defaut de zero attendu");
    CHECK(!everActive, "pompe activee malgre le zero faux");
  }
}

static void testHeaterOnly() {
  printf("Chauffe seule : defauts et alimentation\n");
  const float dt = 0.01f;
  // Ri non mesurée : aucune chauffe à l'aveugle au-delà de la condensation.
  {
    HeaterController h;
    float maxAfterFault = 0.0f;
    for (int k = 0; k < 3000; ++k) {
      h.update(dt, NAN, 14.0f);
      if (h.phase() == HeaterPhase::Fault) maxAfterFault = fmaxf(maxAfterFault, h.duty());
    }
    CHECK(h.faults() & HEATER_FAULT_RI_MISSING, "defaut Ri absente attendu (%u)", h.faults());
    CHECK(maxAfterFault == 0.0f, "chauffe pendant le defaut");
  }
  // Sonde qui ne chauffe jamais : défaut de temps, puis blocage après 3 essais.
  {
    HeaterController h;
    int faultsSeen = 0;
    HeaterPhase prev = h.phase();
    for (int k = 0; k < 30000; ++k) {
      h.update(dt, 50000.0f, 14.0f);
      if (h.phase() == HeaterPhase::Fault && prev != HeaterPhase::Fault) ++faultsSeen;
      prev = h.phase();
    }
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
  testEsrFormula();
  testFullCycle();
  testLambdaAccuracy();
  testStepResponse();
  testDisturbance();
  testPumpProtections();
  testHeaterOnly();
  printf("\n%d verifications, %d echec(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
