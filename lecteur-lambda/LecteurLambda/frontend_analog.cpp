#include "frontend_analog.h"

#include <Wire.h>

#include "config.h"
#include "hal.h"

#if CFG_MESURE == MESURE_ANALOGIQUE && !defined(RI_CALIBRATION_FAITE)
#error "Mesure analogique : renseigner UR_DECALAGE_V et RI_OHM_PAR_V (config.h, section 4) pour le circuit choisi, puis définir RI_CALIBRATION_FAITE."
#endif

namespace {

#if CFG_MESURE == MESURE_ANALOGIQUE && CFG_CONVERTISSEUR == CONVERTISSEUR_ADS1115
// --- ADS1115 : conversion unique, entrée simple, ±4,096 V, 860 échantillons/s ---
bool adsReadVolts(uint8_t channel, float* volts) {
  const uint16_t config = 0x8000                         // démarrer une conversion
                          | ((uint16_t)(4 + channel) << 12)  // AINx par rapport à GND
                          | (1u << 9)                    // ±4,096 V
                          | (1u << 8)                    // conversion unique
                          | (7u << 5)                    // 860 échantillons/s
                          | 0x0003;                      // comparateur désactivé
  Wire.beginTransmission(ADS1115_ADRESSE);
  Wire.write((uint8_t)0x01);
  Wire.write((uint8_t)(config >> 8));
  Wire.write((uint8_t)(config & 0xFF));
  if (Wire.endTransmission() != 0) return false;

  // Attente de la fin de conversion (bit 15 du registre de configuration), 5 ms au plus.
  const uint32_t start = micros();
  for (;;) {
    Wire.beginTransmission(ADS1115_ADRESSE);
    Wire.write((uint8_t)0x01);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom((uint8_t)ADS1115_ADRESSE, (uint8_t)2) != 2) return false;
    const uint8_t hi = Wire.read();
    Wire.read();
    if (hi & 0x80) break;
    if (micros() - start > 5000UL) return false;
  }

  Wire.beginTransmission(ADS1115_ADRESSE);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom((uint8_t)ADS1115_ADRESSE, (uint8_t)2) != 2) return false;
  const uint8_t hi = Wire.read();
  const uint8_t lo = Wire.read();
  const int16_t raw = (int16_t)(((uint16_t)hi << 8) | lo);
  *volts = (float)raw * 4.096f / 32768.0f;
  return true;
}
#endif

// Tension sur l'entrée UA, UR ou Ubat, selon le convertisseur choisi.
bool readInput(uint8_t pin, uint8_t adsChannel, uint8_t samples, float* volts) {
#if CFG_MESURE == MESURE_ANALOGIQUE && CFG_CONVERTISSEUR == CONVERTISSEUR_ADS1115
  (void)pin;
  (void)samples;
  return adsReadVolts(adsChannel, volts);
#else
  (void)adsChannel;
  *volts = hal::readVoltage(pin, samples);
  return true;
#endif
}

// Relation UR -> Ri du circuit d'évaluation. Linéaire ici ; si le circuit retenu
// donne une autre relation, c'est cette fonction qu'il faut remplacer.
float urToRi(float ur_V) {
  const float ri = (ur_V - UR_DECALAGE_V) * RI_OHM_PAR_V;
  return ri > 0.0f ? ri : NAN;
}

}  // namespace

void AnalogFrontEnd::begin() {}

FrontEndSample AnalogFrontEnd::read() {
  FrontEndSample s;
  float uaPin = NAN, urPin = NAN, ubatPin = NAN;
  const bool okUa = readInput(PIN_UA, ADS1115_VOIE_UA, ADC_SURECHANTILLONNAGE, &uaPin);
  const bool okUr = readInput(PIN_UR, ADS1115_VOIE_UR, 4, &urPin);
  const bool okUbat = readInput(PIN_UBAT, ADS1115_VOIE_UBAT, 4, &ubatPin);

  s.ua_V = (uaPin * UA_PONT_DIVISEUR) * UA_CORR_GAIN + UA_CORR_DECALAGE_V;
  s.ri_Ohm = okUr ? urToRi(urPin * UR_PONT_DIVISEUR) : NAN;
  s.ubat_V = okUbat ? ubatPin * UBAT_PONT_DIVISEUR : NAN;
  s.ok = okUa && uaPin > UA_BROCHE_MIN_V && uaPin < UA_BROCHE_MAX_V;
  return s;
}
