// =====================================================================================
//  Configuration du lecteur lambda LSU ADV : c'est le seul fichier à modifier.
//  Les réglages entourés de #ifndef peuvent aussi être imposés à la compilation
//  (build_flags de PlatformIO, voir platformio.ini).
// =====================================================================================
#pragma once

// -------------------------------------------------------------------------------------
// 1. Source des mesures de la sonde
// -------------------------------------------------------------------------------------
#define MESURE_SIMULATION 0   // sonde simulée : essais sur table, SANS sonde branchée
#define MESURE_ANALOGIQUE 1   // tensions UA et UR du vrai circuit d'évaluation

#ifndef CFG_MESURE
#define CFG_MESURE MESURE_SIMULATION
#endif

// Convertisseur utilisé en mesure analogique.
#define CONVERTISSEUR_INTERNE 0  // ADC de la carte (ESP32 12 bits, Mega 10 bits, Pico 12 bits)
#define CONVERTISSEUR_ADS1115 1  // ADS1115 en I2C, 16 bits : recommandé pour la précision

#ifndef CFG_CONVERTISSEUR
#define CFG_CONVERTISSEUR CONVERTISSEUR_INTERNE
#endif

// -------------------------------------------------------------------------------------
// 2. Écran
// -------------------------------------------------------------------------------------
#define ECRAN_AUCUN 0
#define ECRAN_ST7735 1    // TFT couleur 160 x 128 en SPI (celui du croquis jan31a)
#define ECRAN_SSD1306 2   // OLED 128 x 64 en I2C
#define ECRAN_LCD_I2C 3   // LCD 16 x 2 caractères avec interface PCF8574 en I2C

#ifndef CFG_ECRAN
#define CFG_ECRAN ECRAN_ST7735
#endif

#define ST7735_MODELE INITR_BLACKTAB  // variante de l'écran, comme dans jan31a
#define SSD1306_ADRESSE 0x3C
#define LCD_ADRESSE 0x27              // 0x3F sur certains modules

// -------------------------------------------------------------------------------------
// 3. Sortie vers le Speeduino (entrée O2, broche A8 sur la carte v0.4)
// -------------------------------------------------------------------------------------
#define SORTIE_PWM 1      // PWM filtrée (filtre RC, plus un ampli x1,52 sur ESP32 et Pico)
#define SORTIE_MCP4725 2  // convertisseur 12 bits MCP4725 alimenté en 5 V : 0-5 V direct

#ifndef CFG_SORTIE
#define CFG_SORTIE SORTIE_PWM
#endif

// Relation linéaire envoyée au Speeduino, identique à celle du croquis jan31a :
// AFR = 10 + 2 × V, soit 0 V = AFR 10,0 et 5 V = AFR 20,0 en essence.
// Dans TunerStudio : Tools > Calibrate AFR Table > Custom Linear WB, 0 V = 10, 5 V = 20.
#define AFR_STOECHIO 14.7f                       // essence ; E85 : 9.765
#define SORTIE_V_BAS 0.0f
#define SORTIE_LAMBDA_BAS (10.0f / AFR_STOECHIO)
#define SORTIE_V_HAUT 5.0f
#define SORTIE_LAMBDA_HAUT (20.0f / AFR_STOECHIO)

// Tension envoyée tant que la mesure n'est pas exploitable (chauffe, défaut).
// 5 V = AFR 20 : au-dessus de ego_max_afr (17 dans le tune), le Speeduino 202402
// n'applique alors aucune correction (corrections.cpp, ligne 618 : ego_min < O2 < ego_max).
#define SORTIE_V_NON_PRETE 5.0f

// Étage de sortie : tension obtenue sur l'entrée du Speeduino pour 100 % de PWM.
// Mega : 5,0 V direct. ESP32 et Pico : 3,3 V, ou 5,0 V avec l'ampli x1,52 prévu.
#define SORTIE_PLEINE_ECHELLE_V 5.0f
#define MCP4725_ADRESSE 0x60       // 0x62 sur le module Adafruit
#define MCP4725_REFERENCE_V 5.0f   // tension d'alimentation du MCP4725
// Correction mesurée au multimètre (commande série « sortie 1.00 » puis « sortie 4.00 »).
#define SORTIE_CORR_GAIN 1.0f
#define SORTIE_CORR_DECALAGE_V 0.0f

// -------------------------------------------------------------------------------------
// 4. Circuit d'évaluation de la sonde (À DÉFINIR AVEC L'ÉLECTRONIQUE)
// -------------------------------------------------------------------------------------
// UA = 1,5 V + v × 61,9 Ω × Ip : redonne les 23 lignes du tableau Bosch à 0,5 mV près.
#define LSU_UA_ZERO_V 1.5f
#define LSU_SHUNT_OHM 61.9f
#define LSU_GAIN 17.0f              // 17 : toute la plage 0,65 à 5 tient entre 0,05 et 2,97 V
#define UA_PONT_DIVISEUR 1.0f       // rapport du pont diviseur éventuel avant la broche
#define UA_CORR_GAIN 1.0f           // correction mesurée de la chaîne de mesure de UA
#define UA_CORR_DECALAGE_V 0.0f
#define UA_BROCHE_MIN_V 0.02f       // en dehors : convertisseur saturé, mesure refusée
#define UA_BROCHE_MAX_V 3.20f

// Mesure de Ri à partir de la tension UR du circuit : Ri = (UR - décalage) × pente.
// Ces deux valeurs dépendent du circuit choisi. Tant qu'elles ne sont pas mesurées,
// la compilation en MESURE_ANALOGIQUE est bloquée : une chauffe régulée sur une Ri
// fausse surchaufferait la sonde. Une fois mesurées, décommenter la ligne suivante.
// #define RI_CALIBRATION_FAITE
#define UR_PONT_DIVISEUR 1.0f
#define UR_DECALAGE_V 0.0f
#define RI_OHM_PAR_V 0.0f

// Tension batterie (alimentation du chauffage), par pont diviseur : 10 kΩ / 2 kΩ = 6.
#define UBAT_PONT_DIVISEUR 6.0f

// ADS1115 (si CFG_CONVERTISSEUR = CONVERTISSEUR_ADS1115) : entrées AIN0 à AIN3.
#define ADS1115_ADRESSE 0x48
#define ADS1115_VOIE_UA 0
#define ADS1115_VOIE_UR 1
#define ADS1115_VOIE_UBAT 2

// -------------------------------------------------------------------------------------
// 5. Chauffe : réglages propres au montage (le reste suit la fiche, voir heater.h)
// -------------------------------------------------------------------------------------
#define CHAUFFE_CONDENSATION_S 10.0f  // durée de la phase à 2 V : « application specific »
#define CHAUFFE_DEMARRAGE_V 13.2f     // Ubat au-dessus : alternateur qui charge = moteur tournant
#define CHAUFFE_DEMARRAGE_S 2.0f

// -------------------------------------------------------------------------------------
// 6. Rythmes, filtres et journal
// -------------------------------------------------------------------------------------
#define PERIODE_MESURE_MS 10      // mesure, chauffe et sortie à 100 Hz
#define PERIODE_ECRAN_MS 200      // écran à 5 Hz
#define PERIODE_JOURNAL_MS 100    // journal série à 10 Hz
#define FILTRE_SORTIE_S 0.02f     // constante de temps du filtre de la sortie Speeduino
#define FILTRE_ECRAN_S 0.25f      // filtre plus lent pour un affichage lisible
#ifndef JOURNAL_SERIE
#define JOURNAL_SERIE 1           // 1 : lignes CSV sur le port série (115200 bauds)
#endif
#ifndef CHIEN_DE_GARDE
#define CHIEN_DE_GARDE 1          // redémarre la carte si le programme se bloque
#endif
#define CHIEN_DE_GARDE_MS 2000

// Mode simulation : potentiomètre sur PIN_POT_SIMULATION pour faire varier lambda.
#ifndef SIMULATION_POTENTIOMETRE
#define SIMULATION_POTENTIOMETRE 0
#endif

// -------------------------------------------------------------------------------------
// 7. Brochage par carte (détection automatique)
// -------------------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_ESP32)
// ESP32 DevKit : écran aux broches du croquis jan31a. ADC1 uniquement (ADC2 inutilisable avec le Wi-Fi).
#define CARTE_NOM "ESP32"
#define PIN_UA 34
#define PIN_UR 35
#define PIN_UBAT 32
#define PIN_POT_SIMULATION 33
#define PIN_CHAUFFE 26              // PWM LEDC, vers la grille du MOSFET de chauffe
#define PIN_SORTIE_PWM 25           // PWM LEDC, vers le filtre RC et l'ampli
#define PIN_TFT_CS 5
#define PIN_TFT_DC 2
#define PIN_TFT_RST 4               // SPI matériel : SCK 18, MOSI 23
#define I2C_SDA 21
#define I2C_SCL 22
#define ADC_REFERENCE_V 3.3f        // non utilisé : l'ESP32 lit des millivolts calibrés
#define ADC_PLEINE_ECHELLE 4096.0f
#define ADC_SURECHANTILLONNAGE 16
#define CHAUFFE_PWM_HZ 200          // fiche : au moins 100 Hz
#define CHAUFFE_PWM_BITS 12
#define SORTIE_PWM_HZ 15000
#define SORTIE_PWM_BITS 12

#elif defined(__AVR_ATmega2560__)
// Arduino Mega 2560 : tout en 5 V, la sortie PWM attaque directement le filtre RC.
#define CARTE_NOM "MEGA 2560"
#define PIN_UA A0
#define PIN_UR A1
#define PIN_UBAT A2
#define PIN_POT_SIMULATION A3
#define PIN_CHAUFFE 9               // Timer2, 490 Hz, 8 bits
#define PIN_SORTIE_PWM 11           // Timer1 (OC1A), 15,6 kHz, 10 bits : ne pas changer
#define PIN_TFT_CS 53
#define PIN_TFT_DC 49
#define PIN_TFT_RST 48              // SPI matériel : SCK 52, MOSI 51
#define ADC_REFERENCE_V 5.0f        // AVcc : à mesurer au multimètre pour la précision
#define ADC_PLEINE_ECHELLE 1024.0f
#define ADC_SURECHANTILLONNAGE 8

#elif defined(ARDUINO_ARCH_RP2040)
// Raspberry Pi Pico, core « Raspberry Pi Pico/RP2040 » d'Earle Philhower.
#if defined(ARDUINO_ARCH_MBED)
#error "Pico : installer le core « Raspberry Pi Pico/RP2040 » (Earle Philhower), pas le core Arduino Mbed."
#endif
#define CARTE_NOM "PICO"
#define PIN_UA 26
#define PIN_UR 27
#define PIN_UBAT 28
// Pas de PIN_POT_SIMULATION : les trois entrées analogiques sont prises.
#define PIN_CHAUFFE 15              // tranche PWM 7
#define PIN_SORTIE_PWM 13           // tranche PWM 6 : fréquence indépendante de la chauffe
#define PIN_TFT_CS 17
#define PIN_TFT_DC 20
#define PIN_TFT_RST 21              // SPI0 : SCK 18, MOSI 19
#define ADC_REFERENCE_V 3.3f
#define ADC_PLEINE_ECHELLE 4096.0f
#define ADC_SURECHANTILLONNAGE 16
#define CHAUFFE_PWM_HZ 200
#define SORTIE_PWM_HZ 30000
#define PWM_RESOLUTION 4095         // 12 bits

#else
#error "Carte non prise en charge : ESP32, Arduino Mega 2560 ou Raspberry Pi Pico."
#endif

// I2C nécessaire ?
#define I2C_UTILISE                                                                     \
  (CFG_ECRAN == ECRAN_SSD1306 || CFG_ECRAN == ECRAN_LCD_I2C || CFG_SORTIE == SORTIE_MCP4725 || \
   (CFG_MESURE == MESURE_ANALOGIQUE && CFG_CONVERTISSEUR == CONVERTISSEUR_ADS1115))
// Le PCF8574 des LCD est limité à 100 kHz.
#define I2C_FREQUENCE ((CFG_ECRAN == ECRAN_LCD_I2C) ? 100000UL : 400000UL)
