#include "hal.h"

#include <Wire.h>

#include "config.h"

namespace {
inline float clamp01(float d) {
  if (!(d > 0.0f)) return 0.0f;  // NaN compris
  return d > 1.0f ? 1.0f : d;
}
// Rapport cyclique donnant 0 mA de pompe.
inline float pumpZeroDuty() { return CIRCUIT_ZERO_POMPE_V / ADC_REFERENCE_V; }
}  // namespace

// =====================================================================================
#if defined(ARDUINO_ARCH_RP2040)
// =====================================================================================
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/watchdog.h>
#include <pico/critical_section.h>
#include <pico/time.h>

namespace {
critical_section_t g_lock;
void (*volatile g_tick)() = nullptr;
volatile uint32_t g_periodUs = 400;

// Le Pico règle la fréquence par tranche PWM : chaque sortie a sa tranche.
void pwmStart(uint8_t pin, float freq, float duty) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  const uint slice = pwm_gpio_to_slice_num(pin);
  float div = (float)clock_get_hz(clk_sys) / (freq * (PWM_RESOLUTION + 1));
  if (div < 1.0f) div = 1.0f;
  if (div > 255.0f) div = 255.0f;
  pwm_set_clkdiv(slice, div);
  pwm_set_wrap(slice, PWM_RESOLUTION);
  pwm_set_gpio_level(pin, (uint16_t)(clamp01(duty) * (PWM_RESOLUTION + 1) + 0.5f));
  pwm_set_enabled(slice, true);
}

void pwmSet(uint8_t pin, float duty) {
  // Niveau = PWM_RESOLUTION + 1 : sortie toujours haute.
  pwm_set_gpio_level(pin, (uint16_t)(clamp01(duty) * (PWM_RESOLUTION + 1) + 0.5f));
}
}  // namespace

void hal::safeOutputsBegin() {
  pinMode(PIN_CHAUFFE, OUTPUT);
  digitalWrite(PIN_CHAUFFE, LOW);
  pinMode(PIN_INJECTION, OUTPUT);
  digitalWrite(PIN_INJECTION, LOW);
  pwmStart(PIN_CHAUFFE, CHAUFFE_PWM_HZ, 0.0f);
  pwmStart(PIN_POMPE, POMPE_PWM_HZ, pumpZeroDuty());
}

void hal::begin() {
  critical_section_init(&g_lock);
  analogReadResolution(12);
#if I2C_UTILISE
  Wire.begin();  // SDA GP4, SCL GP5
  Wire.setClock(I2C_FREQUENCE);
#endif
}

void hal::heaterSetDuty(float duty) { pwmSet(PIN_CHAUFFE, duty); }
void hal::pumpSetDuty(float duty) { pwmSet(PIN_POMPE, duty); }
void hal::injectionWrite(bool high) { gpio_put(PIN_INJECTION, high); }

void hal::outPwmBegin() { pwmStart(PIN_SORTIE_PWM, SORTIE_PWM_HZ, 0.0f); }
void hal::outPwmSetDuty(float duty) { pwmSet(PIN_SORTIE_PWM, duty); }

float hal::readVoltage(uint8_t pin, uint8_t samples) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) sum += analogRead(pin);
  return (float)sum / (float)samples * ADC_REFERENCE_V / ADC_PLEINE_ECHELLE;
}

void hal::fastLoopBegin(void (*tick)(), float rate_Hz) {
  g_periodUs = (uint32_t)(1000000.0f / rate_Hz + 0.5f);
  g_tick = tick;  // le second cœur démarre dès que ce pointeur est posé
}

void hal::lock() { critical_section_enter_blocking(&g_lock); }
void hal::unlock() { critical_section_exit(&g_lock); }

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

// Second cœur : boucle de pilotage de la sonde, cadencée à l'horloge en microsecondes.
void setup1() {}

void loop1() {
  void (*tick)() = g_tick;
  if (tick == nullptr) return;
  static uint32_t next = 0;
  if (next == 0) next = time_us_32();
  while ((int32_t)(time_us_32() - next) < 0) tight_loop_contents();
  next += g_periodUs;
  // Gros retard (débogueur, blocage) : on repart de maintenant sans rattraper.
  if ((int32_t)(time_us_32() - next) > (int32_t)(4 * g_periodUs)) next = time_us_32() + g_periodUs;
  tick();
}

// =====================================================================================
#elif defined(ARDUINO_ARCH_ESP32)
// =====================================================================================
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#include <esp_task_wdt.h>
#include <esp_timer.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define LL_ESP32_CORE3 1
#else
#define LL_ESP32_CORE3 0
#endif

namespace {
// Canaux 0, 2 et 4 : trois temporisateurs LEDC distincts, donc trois fréquences possibles.
const uint8_t kHeaterChannel = 0;
const uint8_t kOutChannel = 2;
const uint8_t kPumpChannel = 4;

portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
void (*g_tick)() = nullptr;
esp_timer_handle_t g_timer = nullptr;

void ledcStart(uint8_t pin, uint8_t channel, uint32_t freq, uint8_t bits) {
#if LL_ESP32_CORE3
  ledcAttachChannel(pin, freq, bits, channel);
#else
  ledcSetup(channel, freq, bits);
  ledcAttachPin(pin, channel);
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

void timerCallback(void*) {
  if (g_tick) g_tick();
}
}  // namespace

void hal::safeOutputsBegin() {
  pinMode(PIN_CHAUFFE, OUTPUT);
  digitalWrite(PIN_CHAUFFE, LOW);
  pinMode(PIN_INJECTION, OUTPUT);
  digitalWrite(PIN_INJECTION, LOW);
  ledcStart(PIN_CHAUFFE, kHeaterChannel, CHAUFFE_PWM_HZ, CHAUFFE_PWM_BITS);
  ledcSet(PIN_CHAUFFE, kHeaterChannel, CHAUFFE_PWM_BITS, 0.0f);
  ledcStart(PIN_POMPE, kPumpChannel, POMPE_PWM_HZ, POMPE_PWM_BITS);
  ledcSet(PIN_POMPE, kPumpChannel, POMPE_PWM_BITS, pumpZeroDuty());
}

void hal::begin() {
#if I2C_UTILISE
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_FREQUENCE);
#endif
}

void hal::heaterSetDuty(float duty) { ledcSet(PIN_CHAUFFE, kHeaterChannel, CHAUFFE_PWM_BITS, duty); }
void hal::pumpSetDuty(float duty) { ledcSet(PIN_POMPE, kPumpChannel, POMPE_PWM_BITS, duty); }
void hal::injectionWrite(bool high) { digitalWrite(PIN_INJECTION, high ? HIGH : LOW); }

void hal::outPwmBegin() {
  ledcStart(PIN_SORTIE_PWM, kOutChannel, SORTIE_PWM_HZ, SORTIE_PWM_BITS);
  ledcSet(PIN_SORTIE_PWM, kOutChannel, SORTIE_PWM_BITS, 0.0f);
}
void hal::outPwmSetDuty(float duty) { ledcSet(PIN_SORTIE_PWM, kOutChannel, SORTIE_PWM_BITS, duty); }

float hal::readVoltage(uint8_t pin, uint8_t samples) {
  // analogReadMilliVolts applique la calibration d'usine (eFuse) de l'ADC.
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; ++i) sum += analogReadMilliVolts(pin);
  return (float)sum / (float)samples / 1000.0f;
}

void hal::fastLoopBegin(void (*tick)(), float rate_Hz) {
  g_tick = tick;
  esp_timer_create_args_t args = {};
  args.callback = &timerCallback;
  args.name = "sonde";
  esp_timer_create(&args, &g_timer);
  esp_timer_start_periodic(g_timer, (uint64_t)(1000000.0f / rate_Hz + 0.5f));
}

void hal::lock() { portENTER_CRITICAL(&g_mux); }
void hal::unlock() { portEXIT_CRITICAL(&g_mux); }

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

#endif
