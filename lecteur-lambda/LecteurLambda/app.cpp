// Boucle principale : mesure, chauffe, sortie Speeduino, écran, journal série.
#include "app.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "frontend.h"
#include "frontend_analog.h"
#include "hal.h"
#include "heater.h"
#include "lsu_adv.h"
#include "sensor_sim.h"
#include "speeduino_out.h"

#if CFG_MESURE == MESURE_SIMULATION && SIMULATION_POTENTIOMETRE && !defined(PIN_POT_SIMULATION)
#error "Pas d'entrée analogique libre pour le potentiomètre de simulation sur cette carte."
#endif

namespace {

HeaterConfig makeHeaterConfig() {
  HeaterConfig c;  // valeurs de la fiche Bosch : voir heater.h
  c.condensationTime_s = CHAUFFE_CONDENSATION_S;
  c.startVoltage_V = CHAUFFE_DEMARRAGE_V;
  c.startDelay_s = CHAUFFE_DEMARRAGE_S;
  return c;
}

#if CFG_MESURE == MESURE_SIMULATION
SimParams makeSimParams() {
  SimParams p;
  p.gain = LSU_GAIN;
  p.shunt_Ohm = LSU_SHUNT_OHM;
  p.uaZero_V = LSU_UA_ZERO_V;
  return p;
}
SimFrontEnd frontEnd(makeSimParams());
const bool kSimulation = true;
#else
AnalogFrontEnd frontEnd;
const bool kSimulation = false;
#endif

HeaterController heater(makeHeaterConfig());
SpeeduinoOutput speeduino;
const lsu::LinearMap kOutputMap = {SORTIE_V_BAS, SORTIE_LAMBDA_BAS, SORTIE_V_HAUT, SORTIE_LAMBDA_HAUT};

struct Measure {
  FrontEndSample sample;
  float ip_mA;
  float lambdaRaw;
  float lambdaOut;      // filtré pour le Speeduino
  float lambdaDisplay;  // filtré pour l'écran
  lsu::Range range;
  bool valid;
  float out_V;
};
Measure m;

uint32_t g_lastControlMs = 0;
uint32_t g_lastDisplayMs = 0;
uint32_t g_lastLogMs = 0;
bool g_outputForced = false;
float g_forcedVolts = 0.0f;
char g_cmd[40];
uint8_t g_cmdLen = 0;

inline float lowPass(float y, float x, float dt, float tau) { return y + (x - y) * dt / (tau + dt); }

void controlStep(float dt) {
#if CFG_MESURE == MESURE_SIMULATION && SIMULATION_POTENTIOMETRE
  frontEnd.setLambdaOverride(0.70f + 0.60f * hal::readVoltage(PIN_POT_SIMULATION, 4) / ADC_REFERENCE_V);
#endif

  // 1. Mesure : UA -> Ip -> lambda (table Bosch).
  m.sample = frontEnd.read();
  m.ip_mA = lsu::uaToIp_mA(m.sample.ua_V, LSU_GAIN, LSU_SHUNT_OHM, LSU_UA_ZERO_V);
  m.lambdaRaw = lsu::ipToLambda(m.ip_mA, &m.range);

  // 2. Chauffe : stratégie de la fiche, puis régulation sur Ri.
  heater.update(dt, m.sample.ri_Ohm, m.sample.ubat_V);
  hal::heaterSetDuty(heater.duty());
  frontEnd.heaterApplied(heater.appliedVoltage_V(), dt);

  // 3. Lambda exploitable seulement au point de fonctionnement.
  const bool valid = heater.ready() && m.sample.ok && m.range != lsu::Range::Invalid;
  if (valid && m.valid) {
    m.lambdaOut = lowPass(m.lambdaOut, m.lambdaRaw, dt, FILTRE_SORTIE_S);
    m.lambdaDisplay = lowPass(m.lambdaDisplay, m.lambdaRaw, dt, FILTRE_ECRAN_S);
  } else {
    m.lambdaOut = m.lambdaRaw;
    m.lambdaDisplay = m.lambdaRaw;
  }
  m.valid = valid;

  // 4. Sortie Speeduino.
  if (g_outputForced) {
    m.out_V = g_forcedVolts;
  } else if (valid) {
    m.out_V = lsu::lambdaToVolts(kOutputMap, m.lambdaOut);
  } else {
    m.out_V = SORTIE_V_NON_PRETE;
  }
  speeduino.setVolts(m.out_V);
}

void refreshDisplay() {
  DisplayData d;
  d.lambda = m.lambdaDisplay;
  d.afr = m.lambdaDisplay * AFR_STOECHIO;
  d.valid = m.valid;
  d.range = m.range;
  d.phase = heater.phase();
  d.faults = heater.faults();
  d.phaseTime_s = heater.phaseTime_s();
  d.ri_Ohm = m.sample.ri_Ohm;
  d.heater_V = heater.appliedVoltage_V();
  d.ubat_V = m.sample.ubat_V;
  d.out_V = m.out_V;
  d.supplyOk = heater.supplyOk();
  d.simulation = kSimulation;
  d.outputForced = g_outputForced;
  displayUpdate(d);
}

void printHeader() {
  Serial.println(F("# Lecteur lambda Bosch LSU ADV"));
  Serial.print(F("# carte " CARTE_NOM ", mesure "));
  Serial.println(kSimulation ? F("SIMULEE (aucune sonde ne doit etre branchee)") : F("analogique"));
  Serial.println(F("# commandes : sortie <V> | sortie auto | lambda <valeur> | lambda auto | aide"));
  Serial.println(F("t_ms,phase,UA_V,Ip_mA,lambda,AFR,Ri_ohm,Ubat_V,Uh_V,PWM_chauffe,sortie_V,pret,defauts"));
}

void logLine() {
  Serial.print(millis());
  Serial.print(',');
  Serial.print(heaterPhaseShort(heater.phase()));
  Serial.print(',');
  Serial.print(m.sample.ua_V, 4);
  Serial.print(',');
  Serial.print(m.ip_mA, 4);
  Serial.print(',');
  Serial.print(m.lambdaRaw, 4);
  Serial.print(',');
  Serial.print(m.lambdaRaw * AFR_STOECHIO, 2);
  Serial.print(',');
  Serial.print(m.sample.ri_Ohm, 1);
  Serial.print(',');
  Serial.print(m.sample.ubat_V, 2);
  Serial.print(',');
  Serial.print(heater.appliedVoltage_V(), 2);
  Serial.print(',');
  Serial.print(heater.duty(), 3);
  Serial.print(',');
  Serial.print(m.out_V, 3);
  Serial.print(',');
  Serial.print(m.valid ? 1 : 0);
  Serial.print(',');
  Serial.println(heater.faults());
}

void printHelp() {
  Serial.println(F("# sortie 2.35  : impose 2,35 V au Speeduino (calibration au multimetre)"));
  Serial.println(F("# sortie auto  : revient a la mesure"));
  Serial.println(F("# lambda 0.90  : impose lambda en simulation ; lambda auto : profil automatique"));
}

void handleCommand(char* line) {
  for (char* p = line; *p; ++p) {
    if (*p >= 'A' && *p <= 'Z') *p = (char)(*p + ('a' - 'A'));
  }
  char* arg = strchr(line, ' ');
  if (arg) {
    *arg++ = '\0';
    while (*arg == ' ') ++arg;
  }

  if (strcmp(line, "sortie") == 0 && arg && *arg) {
    if (strcmp(arg, "auto") == 0) {
      g_outputForced = false;
      Serial.println(F("# sortie : mesure"));
    } else {
      float v = (float)atof(arg);
      if (v < 0.0f) v = 0.0f;
      if (v > 5.0f) v = 5.0f;
      g_forcedVolts = v;
      g_outputForced = true;
      Serial.print(F("# sortie imposee a "));
      Serial.print(v, 3);
      Serial.println(F(" V"));
    }
  } else if (strcmp(line, "lambda") == 0 && arg && *arg) {
#if CFG_MESURE == MESURE_SIMULATION
    if (strcmp(arg, "auto") == 0) {
      frontEnd.setLambdaOverride(NAN);
      Serial.println(F("# lambda simule : profil automatique"));
    } else {
      const float l = (float)atof(arg);
      frontEnd.setLambdaOverride(l);
      Serial.print(F("# lambda simule impose a "));
      Serial.println(l, 3);
    }
#else
    Serial.println(F("# commande reservee a la simulation"));
#endif
  } else {
    printHelp();
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (g_cmdLen > 0) {
        g_cmd[g_cmdLen] = '\0';
        handleCommand(g_cmd);
        g_cmdLen = 0;
      }
    } else if ((size_t)g_cmdLen + 1 < sizeof g_cmd) {
      g_cmd[g_cmdLen++] = c;
    }
  }
}

}  // namespace

void appSetup() {
  hal::heaterBegin();  // chauffe coupée avant toute autre chose
  Serial.begin(115200);
  hal::begin();
  speeduino.begin();   // « mesure non prête » dès la mise sous tension
  frontEnd.begin();
  displayBegin();

  m.sample.ua_V = NAN;
  m.sample.ri_Ohm = NAN;
  m.sample.ubat_V = NAN;
  m.sample.ok = false;
  m.ip_mA = NAN;
  m.lambdaRaw = NAN;
  m.lambdaOut = NAN;
  m.lambdaDisplay = NAN;
  m.range = lsu::Range::Invalid;
  m.valid = false;
  m.out_V = SORTIE_V_NON_PRETE;

  printHeader();
  hal::watchdogBegin();
  g_lastControlMs = g_lastDisplayMs = g_lastLogMs = millis();
}

void appLoop() {
  hal::watchdogFeed();
  readSerial();

  const uint32_t now = millis();
  if (now - g_lastControlMs >= PERIODE_MESURE_MS) {
    const float dt = (float)(now - g_lastControlMs) / 1000.0f;
    g_lastControlMs = now;
    controlStep(dt);
  }
  if (now - g_lastDisplayMs >= PERIODE_ECRAN_MS) {
    g_lastDisplayMs = now;
    refreshDisplay();
  }
#if JOURNAL_SERIE
  if (now - g_lastLogMs >= PERIODE_JOURNAL_MS) {
    g_lastLogMs = now;
    logLine();
  }
#endif
}
