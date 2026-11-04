// Mise en forme des nombres pour l'écran, sans printf flottant (absent sur AVR).
#pragma once

#include <stdint.h>

// Écrit v avec 'decimals' décimales (6 au plus), aligné à droite sur 'width' caractères
// (0 : pas d'alignement). NaN ou valeur trop grande : « --- ». Renvoie la longueur écrite.
inline uint8_t formatFixed(char* out, uint8_t size, float v, uint8_t decimals, uint8_t width) {
  if (size == 0) return 0;
  if (decimals > 6) decimals = 6;
  char tmp[24];
  uint8_t n = 0;
  uint32_t scale = 1;
  for (uint8_t i = 0; i < decimals; ++i) scale *= 10u;
  const bool neg = v < 0.0f;
  const float a = neg ? -v : v;
  if (!(a == a) || a * (float)scale >= 4.0e9f) {
    tmp[n++] = '-';
    tmp[n++] = '-';
    tmp[n++] = '-';
  } else {
    const uint32_t iv = (uint32_t)(a * (float)scale + 0.5f);
    uint32_t ip = iv / scale;
    uint32_t fp = iv % scale;
    if (neg && iv != 0) tmp[n++] = '-';
    char digits[11];
    uint8_t nd = 0;
    do {
      digits[nd++] = (char)('0' + ip % 10u);
      ip /= 10u;
    } while (ip != 0);
    while (nd != 0) tmp[n++] = digits[--nd];
    if (decimals != 0) {
      tmp[n++] = '.';
      for (int8_t i = (int8_t)decimals - 1; i >= 0; --i) {
        tmp[n + i] = (char)('0' + fp % 10u);
        fp /= 10u;
      }
      n += decimals;
    }
  }
  uint8_t pad = (width > n) ? (uint8_t)(width - n) : 0;
  uint8_t len = 0;
  while (pad-- != 0 && len + 1 < size) out[len++] = ' ';
  for (uint8_t i = 0; i < n && len + 1 < size; ++i) out[len++] = tmp[i];
  out[len] = '\0';
  return len;
}
