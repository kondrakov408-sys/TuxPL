#ifndef TUX_GBSV_H
#define TUX_GBSV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char b64;
    char gf;
    char tux;
    char zeta;
} GbsvSignature;

void gbsv_init(void);

/* Извлекает префикс P и токены терминатора. Возвращает true, если синтаксис 11-байтного окна валиден. */
bool gbsv_parse_terminator(const char *raw_line, size_t len, 
                           size_t *out_prefix_len, GbsvSignature *out_sig);

/* Вычисляет эталонную подпись */
GbsvSignature gbsv_calculate(const uint8_t *P_curr, size_t N_curr, 
                             const uint8_t *P_prev, size_t N_prev);

/* Полная валидация строки с завершением через troll_die() при ошибке */
void gbsv_verify_line_or_die(const char *curr_line, size_t curr_len,
                             const char *prev_line, size_t prev_len);

/* Вспомогательные функции поля Галуа (для верификации изоморфизма в тестах) */
uint8_t gbsv_gf_mul(uint8_t a, uint8_t b);
uint8_t gbsv_gf_inv(uint8_t a);

#endif
