// Deux boucles :
//  - boucle rapide (second cœur du Pico, minuterie sur ESP32), 2 à 2,5 kHz : mesures,
//    injection pour Ri, courant de pompe, chauffe (lsu_driver.h) ;
//  - boucle principale : sortie Speeduino, écran, journal série, commandes.
// Elles échangent un instantané de l'état (LsuStatus) sous verrou.
#include "app.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "hal.h"
#include "heater.h"
#include "lsu_adv.h"
#include "lsu_driver.h"
#include "sim_hardware.h"
#include "speeduino_out.h"

#if CFG_MESURE == MESURE_SONDE && !defined(CIRCUIT_VERIFIE)
#error "Vraie sonde : vérifier les valeurs du circuit (config.h, section 4) sur la carte réalisée, puis définir CIRCUIT_VERIFIE."
#endif

namespace {

LsuCircuit makeCircuit() {
  LsuCircuit c;
  c.vcc_V = ADC_REFERENCE_V;
  c.nernstGain = CIRCUIT_GAIN_NERNST;
  c.esrDriveVpp_V = CIRCUIT_V_INJECTION_CC;
  c.esrDriveR_Ohm = CIRCUIT_R_INJECTION_OHM;
  c.esrSeriesR_Ohm = CIRCUIT_R_SERIE_VM_OHM;
  c.pumpSenseR_Ohm = CIRCUIT_R_SHUNT_OHM;
  c.pumpSenseGain = CIRCUIT_GAIN_SHUNT;
  c.pumpSenseZero_V = CIRCUIT_ZERO_MESURE_V;
  c.pumpDacZero_V = CIRCUIT_ZERO_POMPE_V;
  c.pumpDacVoltsPerMa = CIRCUIT_V_PAR_MA_POMPE;
  return c;
}

LsuControlConfig makeControl() {
  LsuControlConfig k;  // valeurs de rusEFI et de la fiche : voir lsu_driver.h
  k.sampleRate_Hz = FREQ_ECHANTILLONNAGE_HZ;
  return k;
}

HeaterConfig makeHeaterConfig() {
  HeaterConfig h;  // valeurs de la fiche Bosch : voir heater.h
  h.condensationTime_s = CHAUFFE_CONDENSATION_S;
  h.startVoltage_V = CHAUFFE_DEMARRAGE_V;
  h.startDelay_s = CHAUFFE_DEMARRAGE_S;
  return h;
}

const float kSamplePeriod_s = 1.0f / FREQ_ECHANTILLONNAGE_HZ;
const uint16_t kPublishDivider = (uint16_t)(FREQ_ECHANTILLONNAGE_HZ / 200.0f);  // instantané à 200 Hz
const uint16_t kUbatDivider = (uint16_t)(FREQ_ECHANTILLONNAGE_HZ / 100.0f);     // Ubat à 100 Hz

LsuController controller(makeCircuit(), makeControl(), makeHeaterConfig());
#if CFG_MESURE == MESURE_SIMULATION
SimLsuHardware simulated(makeCircuit());
const bool kSimulation = true;
#else
const bool kSimulation = false;
#endif

// --- Boucle rapide : variables propres ---
LsuOutputs g_out;
float g_ubat = NAN;
uint16_t g_publishCounter = 0;
uint16_t g_ubatCounter = 0;

// --- Partagé entre les deux boucles (sous verrou, ou mot unique volatil) ---
LsuStatus g_shared;
volatile uint32_t g_ticks = 0;
volatile float g_lambdaOverride = NAN;  // commande « lambda » en simulation

// --- Boucle principale ---
LsuStatus m;  // dernier instantané
float g_lambdaOut = NAN;
float g_lambdaDisplay = NAN;
float g_outVolts = SORTIE_V_NON_PRETE;
SpeeduinoOutput speeduino;
uint32_t g_lastOutMs = 0;
uint32_t g_lastDisplayMs = 0;
uint32_t g_lastLogMs = 0;
uint32_t g_lastTicks = 0;
uint32_t g_lastTickChangeMs = 0;
bool g_outputForced = false;
float g_forcedVolts = 0.0f;
char g_cmd[40];
uint8_t g_cmdLen = 0;

// Une période d'échantillonnage : mesures -> pilotage -> sorties de puissance.
void fastTick() {
  LsuInputs in;
#if CFG_MESURE == MESURE_SIMULATION
  simulated.setLambdaOverride(g_lambdaOverride);
  in = simulated.step(g_out, kSamplePeriod_s);
#else
  // Les mesures précèdent le changement de sens de l'injection : chaque échantillon
  // est pris en fin de demi-période, une fois le créneau établi.
  in.nernstAdc_V = hal::readVoltage(PIN_NERNST, ADC_SURECHANTILLONNAGE);
  in.pumpSenseAdc_V = hal::readVoltage(PIN_IP, ADC_SURECHANTILLONNAGE);
  if (g_ubatCounter == 0) g_ubat = hal::readVoltage(PIN_UBAT, 4) * UBAT_PONT_DIVISEUR;
  if (++g_ubatCounter >= kUbatDivider) g_ubatCounter = 0;
  in.ubat_V = g_ubat;
#endif
  g_out = controller.step(in);
  hal::injectionWrite(g_out.esrDriveHigh);
  hal::pumpSetDuty(g_out.pumpDuty);
  hal::heaterSetDuty(g_out.heaterDuty);

  if (++g_publishCounter >= kPublishDivider) {
    g_publishCounter = 0;
    hal::lock();
    g_shared = controller.status();
    hal::unlock();
  }
  g_ticks = g_ticks + 1;
}

inline float lowPass(float y, float x, float dt, float tau) { return y + (x - y) * dt / (tau + dt); }

void outputStep(float dt) {
  hal::lock();
  m = g_shared;
  hal::unlock();

  if (m.valid && g_lambdaOut == g_lambdaOut) {
    g_lambdaOut = lowPass(g_lambdaOut, m.lambda, dt, FILTRE_SORTIE_S);
    g_lambdaDisplay = lowPass(g_lambdaDisplay, m.lambda, dt, FILTRE_ECRAN_S);
  } else {
    g_lambdaOut = m.valid ? m.lambda : NAN;
    g_lambdaDisplay = g_lambdaOut;
  }

  static const lsu::LinearMap kMap = {SORTIE_V_BAS, SORTIE_LAMBDA_BAS, SORTIE_V_HAUT, SORTIE_LAMBDA_HAUT};
  if (g_outputForced) {
    g_outVolts = g_forcedVolts;
  } else if (m.valid) {
    g_outVolts = lsu::lambdaToVolts(kMap, g_lambdaOut);
  } else {
    g_outVolts = SORTIE_V_NON_PRETE;
  }
  speeduino.setVolts(g_outVolts);
}

void refreshDisplay() {
  DisplayData d;
  d.s = m;
  d.lambda = g_lambdaDisplay;
  d.afr = g_lambdaDisplay * AFR_STOECHIO;
  d.out_V = g_outVolts;
  d.simulation = kSimulation;
  d.outputForced = g_outputForced;
  displayUpdate(d);
}

void printHeader() {
  Serial.println(F("# Lecteur lambda Bosch LSU ADV, pilotage direct (sans circuit Bosch)"));
  Serial.print(F("# carte " CARTE_NOM ", "));
  Serial.println(kSimulation ? F("sonde SIMULEE (aucune sonde ne doit etre branchee)") : F("vraie sonde"));
  Serial.println(F("# commandes : sortie <V> | sortie auto | lambda <valeur> | lambda auto | aide"));
  Serial.println(F("t_ms,phase,Nernst_V,Ri_ohm,Ip_cmd_mA,Ip_mA,lambda,AFR,Ubat_V,Uh_V,PWM_chauffe,"
                   "pompe,accroche,sortie_V,pret,def_chauffe,def_pompe"));
}

void logLine() {
  Serial.print(millis());
  Serial.print(',');
  Serial.print(heaterPhaseShort(m.phase));
  Serial.print(',');
  Serial.print(m.nernst_V, 4);
  Serial.print(',');
  Serial.print(m.esr_Ohm, 1);
  Serial.print(',');
  Serial.print(m.pumpCommand_mA, 4);
  Serial.print(',');
  Serial.print(m.pumpMeasured_mA, 4);
  Serial.print(',');
  Serial.print(m.lambda, 4);
  Serial.print(',');
  Serial.print(m.lambda * AFR_STOECHIO, 2);
  Serial.print(',');
  Serial.print(m.ubat_V, 2);
  Serial.print(',');
  Serial.print(m.heater_V, 2);
  Serial.print(',');
  Serial.print(m.heaterDuty, 3);
  Serial.print(',');
  Serial.print(m.pumpActive ? 1 : 0);
  Serial.print(',');
  Serial.print(m.locked ? 1 : 0);
  Serial.print(',');
  Serial.print(g_outVolts, 3);
  Serial.print(',');
  Serial.print(m.valid ? 1 : 0);
  Serial.print(',');
  Serial.print(m.heaterFaults);
  Serial.print(',');
  Serial.println(m.pumpFaults);
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
    if (!kSimulation) {
      Serial.println(F("# commande reservee a la simulation"));
    } else if (strcmp(arg, "auto") == 0) {
      g_lambdaOverride = NAN;
      Serial.println(F("# lambda simule : profil automatique"));
    } else {
      const float l = (float)atof(arg);
      g_lambdaOverride = l;
      Serial.print(F("# lambda simule impose a "));
      Serial.println(l, 3);
    }
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
  hal::safeOutputsBegin();  // chauffe coupée, pompe à 0 mA, avant toute autre chose
  Serial.begin(115200);
  hal::begin();
  speeduino.begin();        // « mesure non prête » dès la mise sous tension
  displayBegin();

  g_out.esrDriveHigh = false;
  g_out.pumpDuty = CIRCUIT_ZERO_POMPE_V / ADC_REFERENCE_V;
  g_out.heaterDuty = 0.0f;
  g_shared = controller.status();
  m = g_shared;

  printHeader();
  hal::fastLoopBegin(&fastTick, FREQ_ECHANTILLONNAGE_HZ);
  hal::watchdogBegin();
  g_lastOutMs = g_lastDisplayMs = g_lastLogMs = g_lastTickChangeMs = millis();
}

void appLoop() {
  const uint32_t now = millis();

  // Chien de garde : nourri seulement si la boucle rapide tourne aussi.
  const uint32_t ticks = g_ticks;
  if (ticks != g_lastTicks) {
    g_lastTicks = ticks;
    g_lastTickChangeMs = now;
  }
  if (now - g_lastTickChangeMs < 100) hal::watchdogFeed();

  readSerial();

  if (now - g_lastOutMs >= PERIODE_SORTIE_MS) {
    const float dt = (float)(now - g_lastOutMs) / 1000.0f;
    g_lastOutMs = now;
    outputStep(dt);
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
