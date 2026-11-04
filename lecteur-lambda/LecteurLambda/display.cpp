#include "display.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "format.h"

#if CFG_ECRAN == ECRAN_ST7735
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#elif CFG_ECRAN == ECRAN_SSD1306
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#elif CFG_ECRAN == ECRAN_LCD_I2C
#include <Wire.h>
#endif

namespace {

// Bornes de la barre de richesse.
const float kBarLambdaMin = 0.70f;
const float kBarLambdaMax = 1.30f;

void copyText(char* dst, uint8_t size, const char* src) {
  strncpy(dst, src, size - 1);
  dst[size - 1] = '\0';
}

// Complète par des espaces jusqu'à 'width' caractères (efface l'ancien texte).
void padRight(char* b, uint8_t size, uint8_t width) {
  uint8_t n = (uint8_t)strlen(b);
  while (n < width && n + 1 < size) b[n++] = ' ';
  b[n] = '\0';
}

// Toujours 5 caractères.
void lambdaText(char* b, uint8_t size, const DisplayData& d) {
  if (!d.s.valid) {
    copyText(b, size, "-.---");
  } else if (d.s.range == lsu::Range::LeanLimit) {
    copyText(b, size, "  AIR");
  } else if (d.s.range == lsu::Range::RichLimit) {
    copyText(b, size, "<0.65");
  } else {
    formatFixed(b, size, d.lambda, 3, 5);
  }
}

// Toujours 5 caractères.
void afrText(char* b, uint8_t size, const DisplayData& d) {
  if (!d.s.valid || d.s.range != lsu::Range::InTable) {
    copyText(b, size, "--.--");
  } else {
    formatFixed(b, size, d.afr, 2, 5);
  }
}

// Toujours 5 caractères.
void riText(char* b, uint8_t size, float ri) {
  if (!(ri == ri)) {
    copyText(b, size, "  ---");
  } else if (ri > 99999.0f) {
    copyText(b, size, " >99k");
  } else {
    formatFixed(b, size, ri, 0, 5);
  }
}

// Message d'état, du plus important au moins important.
void appendLatched(char* b, uint8_t size, bool latched) {
  if (latched && strlen(b) + 8 < size) strcat(b, " BLOQUE");
}

void messageText(char* b, uint8_t size, const DisplayData& d) {
  const uint8_t pf = d.s.pumpFaults;
  if (d.s.phase == HeaterPhase::Fault) {
    copyText(b, size, heaterFaultName(d.s.heaterFaults));
    appendLatched(b, size, d.s.heaterFaults & HEATER_FAULT_LATCHED);
  } else if (pf & (PUMP_FAULT_SATURATED | PUMP_FAULT_LATCHED)) {
    copyText(b, size, "POMPE EN BUTEE");
    appendLatched(b, size, pf & PUMP_FAULT_LATCHED);
  } else if (pf & PUMP_FAULT_ZERO) {
    copyText(b, size, "ZERO IP HORS TOL.");
  } else if (!d.s.supplyOk) {
    copyText(b, size, "BATTERIE HORS PLAGE");
  } else if (d.outputForced) {
    copyText(b, size, "SORTIE IMPOSEE");
  } else if (d.simulation) {
    copyText(b, size, "SIMULATION SANS SONDE");
  } else if (d.s.valid) {
    copyText(b, size, "MESURE OK");
  } else if (d.s.pumpActive) {
    copyText(b, size, "ACCROCHAGE POMPE");
  } else {
    copyText(b, size, "CHAUFFE DE LA SONDE");
  }
}

float barFraction(float lambda) {
  float f = (lambda - kBarLambdaMin) / (kBarLambdaMax - kBarLambdaMin);
  if (f < 0.0f) f = 0.0f;
  if (f > 1.0f) f = 1.0f;
  return f;
}

}  // namespace

// =====================================================================================
#if CFG_ECRAN == ECRAN_ST7735
// TFT 160 x 128, paysage.
// =====================================================================================
namespace {

Adafruit_ST7735 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

const uint16_t kGrey = 0x7BEF;
const int16_t kBarX = 4, kBarY = 80, kBarW = 152, kBarH = 9;
const uint8_t kLineChars = 25;

int16_t g_markerX = -1;
uint16_t g_markerColor = 0;
uint16_t g_headerColor = 1;  // valeur impossible : premier tracé forcé
char g_header[28] = "";

uint16_t stateColor(const DisplayData& d) {
  if (d.s.phase == HeaterPhase::Fault || (d.s.pumpFaults & PUMP_FAULT_LATCHED)) return ST77XX_RED;
  return d.s.valid ? ST77XX_GREEN : ST77XX_ORANGE;
}

void printAt(int16_t x, int16_t y, uint8_t size, uint16_t fg, uint16_t bg, const char* text) {
  tft.setTextSize(size);
  tft.setTextColor(fg, bg);  // avec fond : réécrit par-dessus, sans scintillement
  tft.setCursor(x, y);
  tft.print(text);
}

int16_t barX(float lambda) { return kBarX + 2 + (int16_t)(barFraction(lambda) * (kBarW - 5) + 0.5f); }

void drawStoichLine() { tft.drawFastVLine(barX(1.0f), kBarY + 1, kBarH - 2, kGrey); }

}  // namespace

void displayBegin() {
  tft.initR(ST7735_MODELE);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  printAt(4, 13, 1, kGrey, ST77XX_BLACK, "LAMBDA");
  tft.drawRect(kBarX, kBarY, kBarW, kBarH, ST77XX_WHITE);
  const float ticks[] = {0.8f, 0.9f, 1.0f, 1.1f, 1.2f};
  for (uint8_t i = 0; i < 5; ++i) {
    tft.drawFastVLine(barX(ticks[i]), kBarY + kBarH, (i == 2) ? 4 : 2, ST77XX_WHITE);
  }
  drawStoichLine();
}

void displayUpdate(const DisplayData& d) {
  const uint16_t c = stateColor(d);
  char b[32];
  char v1[8];
  char v2[8];

  // Bandeau : phase de chauffe, redessiné seulement s'il change.
  snprintf(b, sizeof b, "%-15s%10s", heaterPhaseName(d.s.phase), d.simulation ? "SIMULATION" : "LSU ADV");
  if (c != g_headerColor || strcmp(b, g_header) != 0) {
    tft.fillRect(0, 0, 160, 11, c);
    printAt(2, 2, 1, ST77XX_BLACK, c, b);
    g_headerColor = c;
    copyText(g_header, sizeof g_header, b);
  }

  // Lambda en grand (3 décimales), puis AFR.
  lambdaText(b, sizeof b, d);
  printAt(4, 23, 4, d.s.valid ? ST77XX_WHITE : kGrey, ST77XX_BLACK, b);
  afrText(v1, sizeof v1, d);
  snprintf(b, sizeof b, "AFR %s", v1);
  printAt(4, 60, 2, d.s.valid ? ST77XX_WHITE : kGrey, ST77XX_BLACK, b);

  // Barre de richesse : riche à gauche, pauvre à droite, repère à lambda 1.
  const bool onBar = d.s.valid && d.s.range == lsu::Range::InTable;
  const int16_t x = onBar ? barX(d.lambda) : -1;
  if (x != g_markerX || c != g_markerColor) {
    if (g_markerX >= 0) tft.fillRect(g_markerX - 1, kBarY + 1, 3, kBarH - 2, ST77XX_BLACK);
    drawStoichLine();
    if (x >= 0) tft.fillRect(x - 1, kBarY + 1, 3, kBarH - 2, c);
    g_markerX = x;
    g_markerColor = c;
  }

  // Résistance interne, tension de chauffe, courant de pompe, sortie.
  riText(v1, sizeof v1, d.s.esr_Ohm);
  formatFixed(v2, sizeof v2, d.s.heater_V, 1, 4);
  snprintf(b, sizeof b, "Ri%s ohm Chauffe %sV", v1, v2);
  padRight(b, sizeof b, kLineChars);
  printAt(4, 94, 1, ST77XX_WHITE, ST77XX_BLACK, b);

  formatFixed(v1, sizeof v1, d.s.pumpMeasured_mA, 3, 6);
  formatFixed(v2, sizeof v2, d.out_V, 2, 4);
  snprintf(b, sizeof b, "Ip%smA Sortie %sV", v1, v2);
  padRight(b, sizeof b, kLineChars);
  printAt(4, 104, 1, ST77XX_WHITE, ST77XX_BLACK, b);

  messageText(b, sizeof b, d);
  padRight(b, sizeof b, kLineChars);
  printAt(4, 116, 1, c, ST77XX_BLACK, b);
}

// =====================================================================================
#elif CFG_ECRAN == ECRAN_SSD1306
// OLED 128 x 64 : l'image est recomposée en mémoire puis envoyée d'un bloc.
// =====================================================================================
namespace {
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool g_oledOk = false;
}  // namespace

void displayBegin() {
  g_oledOk = oled.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADRESSE);
  if (!g_oledOk) return;
  oled.clearDisplay();
  oled.setTextWrap(false);
  oled.display();
}

void displayUpdate(const DisplayData& d) {
  if (!g_oledOk) return;
  char b[28];
  char v1[8];
  char v2[8];
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(heaterPhaseName(d.s.phase));
  if (d.simulation) {
    oled.setCursor(110, 0);
    oled.print("SIM");
  }

  lambdaText(b, sizeof b, d);
  oled.setTextSize(3);
  oled.setCursor(0, 11);
  oled.print(b);

  afrText(v1, sizeof v1, d);
  oled.setTextSize(1);
  oled.setCursor(98, 12);
  oled.print("AFR");
  oled.setCursor(98, 24);
  oled.print(v1);

  // Barre de richesse.
  oled.drawRect(0, 37, 128, 7, SSD1306_WHITE);
  const int16_t x1 = 2 + (int16_t)(barFraction(1.0f) * 123.0f + 0.5f);
  oled.drawFastVLine(x1, 35, 11, SSD1306_WHITE);
  if (d.s.valid && d.s.range == lsu::Range::InTable) {
    const int16_t x = 2 + (int16_t)(barFraction(d.lambda) * 123.0f + 0.5f);
    oled.fillRect(x - 1, 38, 3, 5, SSD1306_WHITE);
  }

  riText(v1, sizeof v1, d.s.esr_Ohm);
  formatFixed(v2, sizeof v2, d.s.heater_V, 1, 4);
  snprintf(b, sizeof b, "Ri%s Uh%sV", v1, v2);
  oled.setCursor(0, 47);
  oled.print(b);
  formatFixed(v1, sizeof v1, d.out_V, 2, 4);
  oled.setCursor(92, 47);
  oled.print(v1);
  oled.print("V");

  messageText(b, sizeof b, d);
  oled.setCursor(0, 56);
  oled.print(b);

  oled.display();
}

// =====================================================================================
#elif CFG_ECRAN == ECRAN_LCD_I2C
// LCD 16 x 2 HD44780 derrière un PCF8574. Câblage du module le plus courant :
// P0 = RS, P1 = RW, P2 = E, P3 = rétroéclairage, P4 à P7 = D4 à D7.
// =====================================================================================
namespace {
const uint8_t kRs = 0x01, kEn = 0x04, kBacklight = 0x08;
bool g_lcdOk = false;
char g_rows[2][17] = {"", ""};

void pcfWrite(uint8_t v) {
  Wire.beginTransmission(LCD_ADRESSE);
  Wire.write((uint8_t)(v | kBacklight));
  Wire.endTransmission();
}

void lcdNibble(uint8_t nibble, uint8_t mode) {
  const uint8_t v = (uint8_t)((nibble << 4) | mode);
  pcfWrite(v | kEn);  // une trame I2C dure bien plus que l'impulsion minimale de E
  pcfWrite(v);
  delayMicroseconds(50);
}

void lcdByte(uint8_t b, uint8_t mode) {
  lcdNibble(b >> 4, mode);
  lcdNibble(b & 0x0F, mode);
}

void lcdCommand(uint8_t c) {
  lcdByte(c, 0);
  if (c <= 0x03) delay(2);  // effacement et retour : 1,52 ms
}

void lcdRow(uint8_t row, const char* text) {
  if (strcmp(g_rows[row], text) == 0) return;  // rien n'a changé
  lcdCommand((uint8_t)(0x80 | (row ? 0x40 : 0x00)));
  for (uint8_t i = 0; i < 16 && text[i]; ++i) lcdByte((uint8_t)text[i], kRs);
  copyText(g_rows[row], sizeof g_rows[row], text);
}
}  // namespace

void displayBegin() {
  Wire.beginTransmission(LCD_ADRESSE);
  g_lcdOk = (Wire.endTransmission() == 0);
  if (!g_lcdOk) return;
  delay(50);
  pcfWrite(0);
  // Passage en mode 4 bits (séquence de la fiche HD44780).
  lcdNibble(0x03, 0);
  delay(5);
  lcdNibble(0x03, 0);
  delay(5);
  lcdNibble(0x03, 0);
  delayMicroseconds(150);
  lcdNibble(0x02, 0);
  lcdCommand(0x28);  // 4 bits, 2 lignes, 5 x 8
  lcdCommand(0x0C);  // affichage allumé, sans curseur
  lcdCommand(0x01);  // effacement
  lcdCommand(0x06);  // écriture de gauche à droite
}

void displayUpdate(const DisplayData& d) {
  if (!g_lcdOk) return;
  char b[24];
  char v1[8];
  char v2[8];
  // Ligne 1 : « L0.998  AFR14.67 »
  lambdaText(v1, sizeof v1, d);
  afrText(v2, sizeof v2, d);
  snprintf(b, sizeof b, "L%s  AFR%s", v1, v2);
  padRight(b, sizeof b, 16);
  lcdRow(0, b);
  // Ligne 2 : « Ri  301 Uh 7.6V » en mesure, sinon la phase de chauffe.
  riText(v1, sizeof v1, d.s.esr_Ohm);
  if (d.s.valid && !d.simulation) {
    formatFixed(v2, sizeof v2, d.s.heater_V, 1, 4);
    snprintf(b, sizeof b, "Ri%s Uh%sV", v1, v2);
  } else if (d.s.pumpFaults != PUMP_FAULT_NONE) {
    snprintf(b, sizeof b, "POMPE  Ri%s", v1);
  } else {
    snprintf(b, sizeof b, "%-7sRi%s", d.s.valid ? "SIMUL" : heaterPhaseShort(d.s.phase), v1);
  }
  padRight(b, sizeof b, 16);
  lcdRow(1, b);
}

// =====================================================================================
#else  // ECRAN_AUCUN
// =====================================================================================
void displayBegin() {}
void displayUpdate(const DisplayData& d) { (void)d; }
#endif
