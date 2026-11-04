#include "speeduino_out.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "hal.h"

namespace {

#if CFG_SORTIE == SORTIE_MCP4725
void mcp4725Write(uint16_t code) {
  // Écriture rapide : 4 bits hauts (mode normal) puis 8 bits bas.
  Wire.beginTransmission(MCP4725_ADRESSE);
  Wire.write((uint8_t)((code >> 8) & 0x0F));
  Wire.write((uint8_t)(code & 0xFF));
  Wire.endTransmission();
}
#endif

}  // namespace

void SpeeduinoOutput::begin() {
#if CFG_SORTIE == SORTIE_PWM
  hal::outPwmBegin();
#endif
  setVolts(SORTIE_V_NON_PRETE);
}

void SpeeduinoOutput::setVolts(float volts) {
  if (!(volts >= 0.0f)) volts = 0.0f;
  if (volts > 5.0f) volts = 5.0f;
  volts_ = volts;
  // Correction de la chaîne de sortie mesurée au multimètre.
  const float corrected = volts * SORTIE_CORR_GAIN + SORTIE_CORR_DECALAGE_V;
#if CFG_SORTIE == SORTIE_PWM
  hal::outPwmSetDuty(corrected / SORTIE_PLEINE_ECHELLE_V);
#elif CFG_SORTIE == SORTIE_MCP4725
  float code = corrected / MCP4725_REFERENCE_V * 4095.0f + 0.5f;
  if (code < 0.0f) code = 0.0f;
  if (code > 4095.0f) code = 4095.0f;
  mcp4725Write((uint16_t)code);
#else
#error "CFG_SORTIE inconnue"
#endif
}
