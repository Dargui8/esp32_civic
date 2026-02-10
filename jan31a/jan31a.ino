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
    tempValue = (tempvoltage - 0.5) * 100; // conversion en degrés Celsius (exemple)

    // Log pour debug
    Serial.print("A0="); Serial.print(widebandreading);
    Serial.print(" v="); Serial.print(widebandvoltage,3);
    Serial.print("  |  A1="); Serial.print(tempreading);
    Serial.print(" v="); Serial.println(tempvoltage,3);

    // Mettre à jour l'affichage: effacer ancienne zone puis réécrire
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_ORANGE);
    tft.fillRect(5, 30, 120, 20, ST77XX_BLACK); // efface l'ancien lambda
    tft.setCursor(5, 30);
    tft.print(lambdaValue,2);

    tft.fillRect(5, 70, 120, 20, ST77XX_BLACK); // efface l'ancienne temp
    tft.setCursor(5, 70);
    tft.print(tempValue,2);

    delay(200);
}
