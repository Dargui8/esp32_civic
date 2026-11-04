// Lecteur de sonde lambda large bande Bosch LSU ADV
//  - affichage précis de la richesse (lambda à 3 décimales, AFR) ;
//  - sortie analogique 0-5 V pour l'entrée O2 du Speeduino ;
//  - commande PWM du chauffage selon la stratégie de la fiche Bosch.
// Cartes : ESP32, Arduino Mega 2560, Raspberry Pi Pico.
// Tous les réglages sont dans config.h ; documentation dans ../README.md.
#include "app.h"

void setup() { appSetup(); }

void loop() { appLoop(); }
