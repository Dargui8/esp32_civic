// Affichage de la richesse : ST7735 (TFT), SSD1306 (OLED) ou LCD 16 x 2, au choix dans config.h.
#pragma once

#include <stdint.h>

#include "heater.h"
#include "lsu_adv.h"

struct DisplayData {
  float lambda;        // filtré pour l'affichage
  float afr;
  bool valid;          // sonde au point de fonctionnement et lecture correcte
  lsu::Range range;
  HeaterPhase phase;
  uint8_t faults;
  float phaseTime_s;
  float ri_Ohm;
  float heater_V;      // tension efficace de chauffe appliquée
  float ubat_V;
  float out_V;         // tension envoyée au Speeduino
  bool supplyOk;
  bool simulation;
  bool outputForced;   // sortie imposée par la commande série « sortie »
};

void displayBegin();
void displayUpdate(const DisplayData& d);
