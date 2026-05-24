#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/**
 * Умножение двух чисел по модулю (без переполнения 64 бит)
 * Используется алгоритм "русского крестьянского умножения"
 */
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

#endif