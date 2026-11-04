// Caractéristique de la sonde Bosch LSU ADV et conversions associées.
//
// Source : fiche Bosch Motorsport « Lambda Sensor LSU ADV/ADV pre Turbo »,
// 51946891, V, 06/05/2021, page 2 (fiches-techniques/lambda/adv_pre_turbo.pdf).
//
// Code C++ pur (aucune dépendance Arduino) : il est compilé tel quel sur la carte
// et dans les tests sur PC (dossier tests/).
#pragma once

#include <stdint.h>

namespace lsu {

// Un point de la caractéristique : courant de pompe Ip (mA) -> lambda.
struct IpPoint {
  float ip_mA;
  float lambda;
};

// Les 23 points du tableau Bosch, Ip croissant.
extern const IpPoint kBoschTable[];
extern const uint8_t kBoschTableSize;

enum class Range : uint8_t {
  InTable,    // 0,65 <= lambda <= 5,0 : dans la caractéristique Bosch
  RichLimit,  // Ip sous la table : lambda < 0,65, hors plage de la sonde
  LeanLimit,  // Ip au-dessus de la table : lambda > 5 (air libre, coupure d'injection)
  Invalid     // mesure inexploitable (NaN)
};

// Circuit d'évaluation (colonnes UA de la fiche) : UA = UA0 + v × Rshunt × Ip.
// Avec UA0 = 1,5 V et Rshunt = 61,9 Ω, cette relation redonne les 23 lignes du tableau
// à 0,5 mV près, pour v = 17 comme pour v = 8 (vérifié dans tests/test_core.cpp).
// La fiche précise que UA est la sortie du circuit d'évaluation, pas de la sonde.
float uaToIp_mA(float ua_V, float gain, float shunt_Ohm, float uaZero_V);
float ipToUa_V(float ip_mA, float gain, float shunt_Ohm, float uaZero_V);

// Ip -> lambda. Entre deux points, l'interpolation est linéaire en 1/lambda :
// Ip suit la teneur en oxygène (ou en gaz réducteurs), qui varie comme 1 - 1/lambda.
// Hors table, la valeur est bornée à 0,65 ou 5,0 et 'range' l'indique.
float interpolateLambda(const IpPoint* table, uint8_t n, float ip_mA, Range* range);
float ipToLambda(float ip_mA, Range* range);

// Lambda -> Ip (fonction réciproque, utilisée par la simulation et les tests).
float lambdaToIp_mA(float lambda);

// Relation linéaire entre lambda et une tension (sortie vers le Speeduino).
struct LinearMap {
  float v0;       // tension basse (V)
  float lambda0;  // lambda à la tension basse
  float v1;       // tension haute (V)
  float lambda1;  // lambda à la tension haute
};

// Bornée à [v0, v1]. Un lambda NaN donne v1 (hors fenêtre de correction du Speeduino).
float lambdaToVolts(const LinearMap& map, float lambda);
float voltsToLambda(const LinearMap& map, float volts);

}  // namespace lsu
