// Affichage de la richesse : ST7735 (TFT), SSD1306 (OLED) ou LCD 16 x 2, au choix dans config.h.
#pragma once

#include <stdint.h>

#include "lsu_driver.h"

struct DisplayData {
  LsuStatus s;         // état de la sonde (chauffe, pompe, Ri, défauts)
  float lambda;        // filtré pour l'affichage
  float afr;
  float out_V;         // tension envoyée au Speeduino
  bool simulation;
  bool outputForced;   // sortie imposée par la commande série « sortie »
};

void displayBegin();
void displayUpdate(const DisplayData& d);
