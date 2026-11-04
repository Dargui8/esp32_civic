// Lecteur de sonde lambda large bande Bosch LSU ADV
//  - affichage précis de la richesse (lambda à 3 décimales, AFR) ;
//  - sortie analogique 0-5 V pour l'entrée O2 du Speeduino ;
//  - pilotage direct de la sonde, sans circuit Bosch : mesure de Ri par injection,
//    boucle de courant de pompe, chauffe selon la stratégie de la fiche Bosch.
// Cartes : Raspberry Pi Pico (conseillée) ou ESP32.
// Tous les réglages sont dans config.h ; documentation dans ../README.md.
#include "app.h"

void setup() { appSetup(); }

void loop() { appLoop(); }
