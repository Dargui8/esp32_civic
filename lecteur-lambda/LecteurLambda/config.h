// =====================================================================================
//  Configuration du lecteur lambda LSU ADV : c'est le seul fichier à modifier.
//  Les réglages entourés de #ifndef peuvent aussi être imposés à la compilation
//  (build_flags de PlatformIO, voir platformio.ini).
// =====================================================================================
#pragma once

// -------------------------------------------------------------------------------------
// 1. Sonde simulée ou vraie sonde
// -------------------------------------------------------------------------------------
// La carte pilote elle-même la sonde (pas de circuit Bosch) : voir lsu_driver.h.
#define MESURE_SIMULATION 0  // sonde simulée : essais sur table, AUCUNE sonde branchée
#define MESURE_SONDE 1       // vraie sonde, à travers le circuit de pilotage (section 4)

#ifndef CFG_MESURE
#define CFG_MESURE MESURE_SIMULATION
#endif

// Pilotage de la vraie sonde : bloqué tant que les valeurs de la section 4 n'ont pas été
// vérifiées sur le circuit réalisé. Un sens ou un gain faux peut détruire la sonde
// (courant de pompe inversé, surchauffe). Une fois vérifiées, décommenter :
// #define CIRCUIT_VERIFIE

// -------------------------------------------------------------------------------------
// 2. Écran
// -------------------------------------------------------------------------------------
#define ECRAN_AUCUN 0
#define ECRAN_ST7735 1   // TFT couleur 160 x 128 en SPI (celui du croquis jan31a)
#define ECRAN_SSD1306 2  // OLED 128 x 64 en I2C
#define ECRAN_LCD_I2C 3  // LCD 16 x 2 caractères avec interface PCF8574 en I2C

#ifndef CFG_ECRAN
#define CFG_ECRAN ECRAN_ST7735
#endif

#define ST7735_MODELE INITR_BLACKTAB  // variante de l'écran, comme dans jan31a
#define SSD1306_ADRESSE 0x3C
#define LCD_ADRESSE 0x27              // 0x3F sur certains modules

// -------------------------------------------------------------------------------------
// 3. Sortie vers le Speeduino (entrée O2, broche A8 sur la carte v0.4)
// -------------------------------------------------------------------------------------
#define SORTIE_PWM 1      // PWM filtrée (filtre RC) puis ampli x1,52 : 3,3 V -> 5 V
#define SORTIE_MCP4725 2  // convertisseur 12 bits MCP4725 alimenté en 5 V : 0-5 V direct

#ifndef CFG_SORTIE
#define CFG_SORTIE SORTIE_PWM
#endif

// Relation linéaire envoyée au Speeduino, identique à celle du croquis jan31a :
// AFR = 10 + 2 × V, soit 0 V = AFR 10,0 et 5 V = AFR 20,0 en essence.
// Dans TunerStudio : Tools > Calibrate AFR Table > Custom Linear WB, 0 V = 10, 5 V = 20.
#define AFR_STOECHIO 14.7f  // essence ; E85 : 9.765
#define SORTIE_V_BAS 0.0f
#define SORTIE_LAMBDA_BAS (10.0f / AFR_STOECHIO)
#define SORTIE_V_HAUT 5.0f
#define SORTIE_LAMBDA_HAUT (20.0f / AFR_STOECHIO)

// Tension envoyée tant que la mesure n'est pas exploitable (chauffe, défaut).
// 5 V = AFR 20 : au-dessus de ego_max_afr (17 dans le tune), le Speeduino 202402
// n'applique alors aucune correction (corrections.cpp, ligne 618 : ego_min < O2 < ego_max).
#define SORTIE_V_NON_PRETE 5.0f

// Étage de sortie : tension obtenue sur l'entrée du Speeduino pour 100 % de PWM.
// 5,0 V avec l'ampli x1,52 prévu ; 3,3 V sans ampli (revoir alors la relation ci-dessus).
#define SORTIE_PLEINE_ECHELLE_V 5.0f
#define MCP4725_ADRESSE 0x60      // 0x62 sur le module Adafruit
#define MCP4725_REFERENCE_V 5.0f  // tension d'alimentation du MCP4725
// Correction mesurée au multimètre (commande série « sortie 1.00 » puis « sortie 4.00 »).
#define SORTIE_CORR_GAIN 1.0f
#define SORTIE_CORR_DECALAGE_V 0.0f

// -------------------------------------------------------------------------------------
// 4. Circuit de pilotage direct (À CONFIRMER AVEC L'ÉLECTRONIQUE)
// -------------------------------------------------------------------------------------
// Valeurs de référence : contrôleur open source rusEFI (carte f0_module), voir README.
// Cellule de Nernst : (UN - VM) amplifiée vers l'ADC.
#define CIRCUIT_GAIN_NERNST 2.7f
// Mesure de Ri : sortie logique 0 / 3,3 V à travers une résistance vers UN.
#define CIRCUIT_R_INJECTION_OHM 22000.0f
#define CIRCUIT_V_INJECTION_CC 3.3f   // amplitude crête à crête du créneau
#define CIRCUIT_R_SERIE_VM_OHM 10.0f
// Mesure du courant de pompe : shunt et ampli, autour d'une tension de repos.
#define CIRCUIT_R_SHUNT_OHM 61.9f
#define CIRCUIT_GAIN_SHUNT 10.0f
#define CIRCUIT_ZERO_MESURE_V 1.65f   // recalé automatiquement tant que la pompe est coupée
// Commande du courant de pompe : PWM filtrée -> convertisseur tension-courant.
#define CIRCUIT_ZERO_POMPE_V 1.65f    // tension de commande donnant 0 mA
#define CIRCUIT_V_PAR_MA_POMPE -0.321162f
// Tension batterie, par pont diviseur : 10 kΩ / 2 kΩ = 6.
#define UBAT_PONT_DIVISEUR 6.0f

// -------------------------------------------------------------------------------------
// 5. Chauffe : réglages propres au montage (le reste suit la fiche, voir heater.h)
// -------------------------------------------------------------------------------------
#define CHAUFFE_CONDENSATION_S 10.0f  // durée de la phase à 2 V : « application specific »
#define CHAUFFE_DEMARRAGE_V 13.2f     // Ubat au-dessus : alternateur qui charge = moteur tournant
#define CHAUFFE_DEMARRAGE_S 2.0f

// -------------------------------------------------------------------------------------
// 6. Rythmes, filtres et journal
// -------------------------------------------------------------------------------------
#define PERIODE_SORTIE_MS 10      // sortie Speeduino à 100 Hz
#define PERIODE_ECRAN_MS 200      // écran à 5 Hz
#define PERIODE_JOURNAL_MS 100    // journal série à 10 Hz
#define FILTRE_SORTIE_S 0.02f     // filtre de la sortie Speeduino (en plus de celui de Ip)
#define FILTRE_ECRAN_S 0.25f      // filtre plus lent pour un affichage lisible
#ifndef JOURNAL_SERIE
#define JOURNAL_SERIE 1           // 1 : lignes CSV sur le port série (115200 bauds)
#endif
#ifndef CHIEN_DE_GARDE
#define CHIEN_DE_GARDE 1          // redémarre la carte si un des deux programmes se bloque
#endif
#define CHIEN_DE_GARDE_MS 2000

// -------------------------------------------------------------------------------------
// 7. Brochage par carte (détection automatique)
// -------------------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_RP2040)
// Raspberry Pi Pico, core « Raspberry Pi Pico/RP2040 » d'Earle Philhower.
// Le second cœur est réservé au pilotage de la sonde. CARTE CONSEILLÉE.
#if defined(ARDUINO_ARCH_MBED)
#error "Pico : installer le core « Raspberry Pi Pico/RP2040 » (Earle Philhower), pas le core Arduino Mbed."
#endif
#define CARTE_NOM "PICO"
#define PIN_NERNST 26               // ADC0 : sortie de l'ampli de Nernst
#define PIN_IP 27                   // ADC1 : sortie de l'ampli du shunt de pompe
#define PIN_UBAT 28                 // ADC2 : pont diviseur de la batterie
#define PIN_INJECTION 14            // créneau 0 / 3,3 V vers la résistance d'injection
#define PIN_POMPE 10                // PWM (tranche 5) vers le filtre et le convertisseur V-I
#define PIN_CHAUFFE 15              // PWM (tranche 7) vers le MOSFET de chauffe
#define PIN_SORTIE_PWM 13           // PWM (tranche 6) vers le filtre et l'ampli Speeduino
#define PIN_TFT_CS 17
#define PIN_TFT_DC 20
#define PIN_TFT_RST 21              // SPI0 : SCK 18, MOSI 19
#define ADC_REFERENCE_V 3.3f
#define ADC_PLEINE_ECHELLE 4096.0f
#define ADC_SURECHANTILLONNAGE 8
#define FREQ_ECHANTILLONNAGE_HZ 2500.0f  // comme rusEFI ; injection pour Ri à 1,25 kHz
#define CHAUFFE_PWM_HZ 200          // fiche : au moins 100 Hz
#define POMPE_PWM_HZ 30000
#define SORTIE_PWM_HZ 30000
#define PWM_RESOLUTION 4095         // 12 bits

#elif defined(ARDUINO_ARCH_ESP32)
// ESP32 DevKit : écran aux broches du croquis jan31a. ADC1 uniquement.
// La sonde est pilotée par une minuterie esp_timer à 2 kHz.
#define CARTE_NOM "ESP32"
#define PIN_NERNST 34
#define PIN_IP 35
#define PIN_UBAT 32
#define PIN_INJECTION 27
#define PIN_POMPE 33                // PWM LEDC, temporisateur 2
#define PIN_CHAUFFE 26              // PWM LEDC, temporisateur 0
#define PIN_SORTIE_PWM 25           // PWM LEDC, temporisateur 1
#define PIN_TFT_CS 5
#define PIN_TFT_DC 2
#define PIN_TFT_RST 4               // SPI matériel : SCK 18, MOSI 23
#define I2C_SDA 21
#define I2C_SCL 22
#define ADC_REFERENCE_V 3.3f        // non utilisé : l'ESP32 lit des millivolts calibrés
#define ADC_SURECHANTILLONNAGE 4
#define FREQ_ECHANTILLONNAGE_HZ 2000.0f  // injection pour Ri à 1 kHz
#define CHAUFFE_PWM_HZ 200
#define CHAUFFE_PWM_BITS 12
#define POMPE_PWM_HZ 19000
#define POMPE_PWM_BITS 12
#define SORTIE_PWM_HZ 15000
#define SORTIE_PWM_BITS 12

#elif defined(__AVR__)
#error "Arduino Mega : trop lent pour piloter la sonde sans circuit Bosch (ADC 10 bits à ~9 kHz, 16 MHz sans calcul flottant). Utiliser un Pico ou un ESP32."

#else
#error "Carte non prise en charge : Raspberry Pi Pico (conseillé) ou ESP32."
#endif

// I2C nécessaire ?
#define I2C_UTILISE (CFG_ECRAN == ECRAN_SSD1306 || CFG_ECRAN == ECRAN_LCD_I2C || CFG_SORTIE == SORTIE_MCP4725)
// Le PCF8574 des LCD est limité à 100 kHz.
#define I2C_FREQUENCE ((CFG_ECRAN == ECRAN_LCD_I2C) ? 100000UL : 400000UL)
