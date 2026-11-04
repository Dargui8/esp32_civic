#include "hal.h"

#include <Wire.h>

#include "config.h"

namespace {
inline float clamp01(float d) {
  if (!(d > 0.0f)) return 0.0f;  // NaN compris
  return d > 1.0f ? 1.0f : d;
}
}  // namespace

// =====================================================================================
#if defined(ARDUINO_ARCH_ESP32)
// =====================================================================================
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#include <esp_task_wdt.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define LL_ESP32_CORE3 1
#else
#define LL_ESP32_CORE3 0
#endif

namespace {
// Canaux 0 et 2 : deux temporisateurs LEDC distincts, donc deux fréquences possibles.
const uint8_t kHeaterChannel = 0;
const uint8_t kOutChannel = 2;

void ledcStart(uint8_t pin, uint8_t channel, uint32_t freq, uint8_t bits) {
#if LL_ESP32_CORE3
  ledcAttachChannel(pin, freq, bits, channel);
  ledcWrite(pin, 0);
#else
  ledcSetup(channel, freq, bits);
  ledcAttachPin(pin, channel);
  ledcWrite(channel, 0);
#endif
}

void ledcSet(uint8_t pin, uint8_t channel, uint8_t bits, float duty) {
  const uint32_t top = (1UL << bits) - 1;  // top = 100 % (le core complète le dernier pas)
  const uint32_t value = (uint32_t)(clamp01(duty) * top + 0.5f);
#if LL_ESP32_CORE3
  (void)channel;
  ledcWrite(pin, value);
#else
  (void)pin;
  ledcWrite(channel, value);
#endif
}
}  // namespace

void hal::begin() {
#if I2C_UTILISE
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_FREQUENCE);
#endif
}

void hal::heaterBegin() {
  pinMode(PIN_CHAUFFE, OUTPUT);
  digitalWrite(PIN_CHAUFFE, LOW);
  ledcStart(PIN_CHAUFFE, kHeaterChannel, CHAUFFE_PWM_HZ, CHAUFFE_PWM_BITS);
}

void hal::heaterSetDuty(float duty) { ledcSet(PIN_CHAUFFE, kHeaterChannel, CHAUFFE_PWM_BITS, duty); }

void hal::outPwmBegin() { ledcStart(PIN_SORTIE_PWM, kOutChannel, SORTIE_PWM_HZ, SORTIE_PWM_BITS); }

void hal::outPwmSetDuty(float duty) { ledcSet(PIN_SORTIE_PWM, kOutChannel, SORTIE_PWM_BITS, duty); }

float hal::readVoltage(uint8_t pin, uint8_t samples) {
  // analogReadMilliVolts applique la calibration d'usine (eFuse) de l'ADC.
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) sum += analogReadMilliVolts(pin);
  return (float)sum / (float)samples / 1000.0f;
}

void hal::watchdogBegin() {
#if CHIEN_DE_GARDE
#if LL_ESP32_CORE3
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = CHIEN_DE_GARDE_MS;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) esp_task_wdt_init(&cfg);
#else
  esp_task_wdt_init((CHIEN_DE_GARDE_MS + 999) / 1000, true);
#endif
  esp_task_wdt_add(NULL);
#endif
}

void hal::watchdogFeed() {
#if CHIEN_DE_GARDE
  esp_task_wdt_reset();
#endif
}

// =====================================================================================
#elif defined(__AVR_ATmega2560__)
// =====================================================================================
#include <avr/wdt.h>

#if CFG_SORTIE == SORTIE_PWM && PIN_SORTIE_PWM != 11
#error "Mega : la sortie PWM Speeduino utilise le Timer1 (OC1A), donc la broche 11."
#endif

void hal::begin() {
  analogReference(DEFAULT);
#if I2C_UTILISE
  Wire.begin();
  Wire.setClock(I2C_FREQUENCE);
#endif
}

void hal::heaterBegin() {
  pinMode(PIN_CHAUFFE, OUTPUT);
  digitalWrite(PIN_CHAUFFE, LOW);
}

void hal::heaterSetDuty(float duty) {
  // analogWrite : 0 = toujours bas, 255 = toujours haut. Broche 9 : Timer2, 490 Hz.
  analogWrite(PIN_CHAUFFE, (int)(clamp01(duty) * 255.0f + 0.5f));
}

void hal::outPwmBegin() {
  // Timer1 en PWM rapide, sommet ICR1 = 1023 : 16 MHz / 1024 = 15,6 kHz, 10 bits.
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  TCCR1A = _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = 1023;
  OCR1A = 0;
}

void hal::outPwmSetDuty(float duty) {
  // En PWM rapide, la sortie reste haute OCR1A + 1 coups sur 1024.
  const int16_t counts = (int16_t)(clamp01(duty) * 1024.0f + 0.5f);
  if (counts <= 0) {
    TCCR1A &= ~_BV(COM1A1);  // déconnectée : vrai 0 V
    digitalWrite(11, LOW);
  } else {
    OCR1A = (uint16_t)(counts - 1);
    TCCR1A |= _BV(COM1A1);
  }
}

float hal::readVoltage(uint8_t pin, uint8_t samples) {
  analogRead(pin);  // première lecture après changement de voie écartée
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) sum += analogRead(pin);
  return (float)sum / (float)samples * ADC_REFERENCE_V / ADC_PLEINE_ECHELLE;
}

void hal::watchdogBegin() {
#if CHIEN_DE_GARDE
  // Attention aux anciens chargeurs de Mega (clones) qui bouclent après un reset du
  // chien de garde : passer CHIEN_DE_GARDE à 0 si la carte redémarre sans fin.
  wdt_enable(WDTO_2S);
#endif
}

void hal::watchdogFeed() {
#if CHIEN_DE_GARDE
  wdt_reset();
#endif
}

// =====================================================================================
#elif defined(ARDUINO_ARCH_RP2040)
// =====================================================================================
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/watchdog.h>

namespace {
// Le Pico règle la fréquence par tranche PWM : chauffe et sortie sont sur deux tranches.
void pwmStart(uint8_t pin, float freq) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  const uint slice = pwm_gpio_to_slice_num(pin);
  float div = (float)clock_get_hz(clk_sys) / (freq * (PWM_RESOLUTION + 1));
  if (div < 1.0f) div = 1.0f;
  if (div > 255.0f) div = 255.0f;
  pwm_set_clkdiv(slice, div);
  pwm_set_wrap(slice, PWM_RESOLUTION);
  pwm_set_gpio_level(pin, 0);
  pwm_set_enabled(slice, true);
}

void pwmSet(uint8_t pin, float duty) {
  // Niveau = PWM_RESOLUTION + 1 : sortie toujours haute.
  pwm_set_gpio_level(pin, (uint16_t)(clamp01(duty) * (PWM_RESOLUTION + 1) + 0.5f));
}
}  // namespace

void hal::begin() {
  analogReadResolution(12);
#if I2C_UTILISE
  Wire.begin();  // SDA GP4, SCL GP5
  Wire.setClock(I2C_FREQUENCE);
#endif
}

void hal::heaterBegin() {
  pinMode(PIN_CHAUFFE, OUTPUT);
  digitalWrite(PIN_CHAUFFE, LOW);
  pwmStart(PIN_CHAUFFE, CHAUFFE_PWM_HZ);
}

void hal::heaterSetDuty(float duty) { pwmSet(PIN_CHAUFFE, duty); }

void hal::outPwmBegin() { pwmStart(PIN_SORTIE_PWM, SORTIE_PWM_HZ); }

void hal::outPwmSetDuty(float duty) { pwmSet(PIN_SORTIE_PWM, duty); }

float hal::readVoltage(uint8_t pin, uint8_t samples) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) sum += analogRead(pin);
  return (float)sum / (float)samples * ADC_REFERENCE_V / ADC_PLEINE_ECHELLE;
}

void hal::watchdogBegin() {
#if CHIEN_DE_GARDE
  watchdog_enable(CHIEN_DE_GARDE_MS, true);
#endif
}

void hal::watchdogFeed() {
#if CHIEN_DE_GARDE
  watchdog_update();
#endif
}

#endif
