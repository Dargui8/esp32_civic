// Ce que le programme attend du circuit d'évaluation de la sonde, quel qu'il soit.
//
// Deux sources fournissent ces mesures, choisies dans config.h (CFG_MESURE) :
//  - SimFrontEnd (sensor_sim.h) : sonde simulée, pour essayer sans électronique ;
//  - AnalogFrontEnd (frontend_analog.h) : tensions UA et UR lues sur le vrai circuit.
#pragma once

struct FrontEndSample {
  float ua_V;    // sortie UA du circuit d'évaluation, image du courant de pompe Ip
  float ri_Ohm;  // résistance interne de la cellule de Nernst (NaN si inconnue)
  float ubat_V;  // tension d'alimentation du chauffage
  bool ok;       // lecture cohérente (pas de saturation du convertisseur)
};
