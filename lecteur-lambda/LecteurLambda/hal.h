// Couche matérielle : tout ce qui change d'une carte à l'autre (PWM, convertisseur,
// chien de garde). Implémentation dans hal.cpp, une section par carte.
#pragma once

#include <Arduino.h>

namespace hal {

void begin();  // convertisseur et bus I2C

// Sortie de chauffe : PWM vers le MOSFET. heaterBegin() la force à 0 : à appeler en premier.
void heaterBegin();
void heaterSetDuty(float duty);  // 0 à 1

// Sortie PWM vers le Speeduino (si CFG_SORTIE = SORTIE_PWM).
void outPwmBegin();
void outPwmSetDuty(float duty);  // 0 à 1

// Tension moyenne sur une broche analogique, en volts, sur 'samples' lectures.
float readVoltage(uint8_t pin, uint8_t samples);

void watchdogBegin();
void watchdogFeed();

}  // namespace hal
