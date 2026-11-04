// Couche matérielle : tout ce qui change d'une carte à l'autre (PWM, ADC, cadencement de la
// boucle rapide, verrou, chien de garde). Implémentation dans hal.cpp, une section par carte.
#pragma once

#include <Arduino.h>

namespace hal {

// Sorties de puissance au repos : chauffe coupée, pompe à 0 mA, injection basse.
// À appeler en tout premier.
void safeOutputsBegin();

void begin();  // ADC, I2C, verrou

void heaterSetDuty(float duty);  // PWM de chauffe, 0 à 1
void pumpSetDuty(float duty);    // PWM du « DAC » de pompe, 0 à 1
void injectionWrite(bool high);  // créneau d'injection pour la mesure de Ri

void outPwmBegin();              // sortie PWM vers le Speeduino
void outPwmSetDuty(float duty);

// Tension moyenne sur une broche analogique, en volts, sur 'samples' lectures.
float readVoltage(uint8_t pin, uint8_t samples);

// Appelle tick() à rate_Hz : second cœur du Pico, minuterie esp_timer sur ESP32.
void fastLoopBegin(void (*tick)(), float rate_Hz);

// Verrou entre la boucle rapide et la boucle principale (sections très courtes).
void lock();
void unlock();

void watchdogBegin();
void watchdogFeed();

}  // namespace hal
