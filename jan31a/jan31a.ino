#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Broches ESP32 (tu peux les changer si besoin)
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18  // SCK (VSPI)
#define TFT_MOSI 23  // MOSI (VSPI)

//intergation des recepteurs
const int pinA0 = 32;
const int pinA1 = 33; // température

// Variables globales pour affichage
float lambdaValue = 0.0;
float tempValue = 0.0;

// Calibration de la sonde de température
// À adapter en fonction de votre sonde !
// Exemple pour LM35: V0=0.5V@0°C, V100=1.5V@100°C
float calibVoltage_Low = 0.50;   // Tension lue à 0°C (ou temp connue)
float calibTemp_Low = 0.0;       // Température réelle à ce voltage
float calibVoltage_High = 1.50;  // Tension lue à 100°C (ou temp connue)
float calibTemp_High = 100.0;    // Température réelle à ce voltage

// Instance de l'écran (SPI matériel)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);   // initialisation écran
  tft.setRotation(1);          // orientation paysage
  tft.fillScreen(ST77XX_BLACK);// fond noir

  tft.setTextColor(ST77XX_ORANGE);
  tft.setTextSize(2);          // taille du texte 1 2 3 4 

  tft.setCursor(5, 10);
  tft.print("LAMBDA");

  tft.setCursor(5, 30);
  tft.print(lambdaValue,2); // lambda variable

  tft.setCursor(5, 50);
  tft.print("TEMP EXT");

  tft.setCursor(5, 70);
  tft.print(tempValue,2); // temp variable    
}

void loop() {
    int widebandreading = analogRead(pinA0); // lecture du signal du capteur large bande
    int tempreading = analogRead(pinA1); // lecture du signal du capteur de température
    // ESP32 ADC: 0..4095, Vref = 3.3V
    float widebandvoltage = widebandreading * (3.3 / 4095.0); // conversion en tension
    float tempvoltage = tempreading * (3.3 / 4095.0); // conversion en tension

    lambdaValue = 10 + (widebandvoltage * 2);
    
    // Calibration linéaire de la température
    float tempRange = calibTemp_High - calibTemp_Low;
    float voltageRange = calibVoltage_High - calibVoltage_Low;
    tempValue = calibTemp_Low + ((tempvoltage - calibVoltage_Low) * (tempRange / voltageRange));

    // Mettre à jour l'affichage: effacer ancienne zone puis réécrire
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_ORANGE);
    tft.fillRect(5, 30, 120, 20, ST77XX_BLACK); // efface l'ancien lambda
    tft.setCursor(5, 30);
    tft.print(lambdaValue,2);

    tft.fillRect(5, 70, 120, 20, ST77XX_BLACK); // efface l'ancienne temp
    tft.setCursor(5, 70);
    tft.print(tempValue,2);
    tft.print(" °C");  // Ajout du °C

    delay(200);
}
