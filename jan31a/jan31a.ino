#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Broches ESP32 (tu peux les changer si besoin)
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18  // SCK (VSPI)
#define TFT_MOSI 23  // MOSI (VSPI)

// Instance de l'écran (SPI matériel)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  tft.initR(INITR_BLACKTAB);   // initialisation écran
  tft.setRotation(1);          // orientation paysage
  tft.fillScreen(ST77XX_BLACK);// fond noir

  tft.setTextColor(ST77XX_ORANGE);
  tft.setTextSize(2);          // taille du texte 1 2 3 4 

  tft.setCursor(5, 10);
  tft.print("LAMBDA");

  tft.setCursor(5, 30);
  tft.print("14.7");

}

void loop() {
  // vide → rien ne change, affichage figé
}
