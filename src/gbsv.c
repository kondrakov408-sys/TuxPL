#include "gbsv.h"
#include "tuxpl.h"
#include <string.h>

static const char B64_TABLE[65] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char ZETA_TABLE[5] = {'!', '?', '~', '%', '&'};
static const uint8_t TUX_SEED[6] = {'~', '"', 'T', 'U', 'X', '"'};

static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static uint8_t gf_inv[256];
static bool tables_initialized = false;

static inline uint8_t xtime(uint8_t x) {
    return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0));
}

void gbsv_init(void) {
    if (tables_initialized) return;

    /* Инициализация таблиц для поля F_{2^8} (AES полином 0x11B, генератор alpha = 0x03) */
    uint8_t x = 1;
    gf_exp[0] = 1;
    gf_log[0] = 0; /* Не определен математически, по соглашению 0 */
    gf_log[1] = 0;

    for (int i = 1; i < 255; i++) {
        /* Умножение на 3 (alpha = 0x03 = x + 1): x * 3 = xtime(x) ^ x */
        x = xtime(x) ^ x;
        gf_exp[i] = x;
        gf_log[x] = (uint8_t)i;
    }

    for (int i = 255; i < 512; i++) {
        gf_exp[i] = gf_exp[i - 255];
    }

    gf_inv[0] = 0;
    for (int i = 1; i < 256; i++) {
        uint8_t log_val = gf_log[i];
        gf_inv[i] = gf_exp[255 - log_val];
    }

    tables_initialized = true;
}

static inline uint8_t gf_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return gf_exp[(size_t)gf_log[a] + (size_t)gf_log[b]];
}

uint8_t gbsv_gf_mul(uint8_t a, uint8_t b) {
    gbsv_init();
    return gf_mul(a, b);
}

uint8_t gbsv_gf_inv(uint8_t a) {
    gbsv_init();
    return gf_inv[a];
}

/* 2.1. Расчет sigma_b64 */
static char compute_b64(const uint8_t *P, size_t N) {
    /* Подсчет k = sum(popcount(P[j] & 0xAA)) % 64 */
    uint32_t k_sum = 0;
    for (size_t j = 0; j < N; j++) {
        k_sum += (uint32_t)__builtin_popcount((unsigned int)(P[j] & 0xAA));
    }
    uint8_t k = (uint8_t)(k_sum % 64);
    uint8_t shift = (uint8_t)(k % 6);

    /* Нарезка на 6-битные блоки (big-endian) */
    size_t M = (N * 8 + 5) / 6;
    uint32_t acc = 0;

    for (size_t i = 0; i < M; i++) {
        size_t bit_idx = i * 6;
        size_t byte_idx = bit_idx / 8;
        size_t bit_off = bit_idx % 8; /* 0..7 от MSB */

        uint8_t b0 = P[byte_idx];
        size_t avail = 8 - bit_off;
        uint8_t chunk_i = 0;

        if (avail >= 6) {
            chunk_i = (uint8_t)((b0 >> (avail - 6)) & 0x3F);
        } else {
            size_t needed = 6 - avail;
            uint8_t part0 = (uint8_t)((b0 & ((1 << avail) - 1)) << needed);
            uint8_t b1 = (byte_idx + 1 < N) ? P[byte_idx + 1] : 0;
            uint8_t part1 = (uint8_t)((b1 >> (8 - needed)) & ((1 << needed) - 1));
            chunk_i = (part0 | part1) & 0x3F;
        }

        /* Циклический 6-битный сдвиг влево на shift */
        uint8_t val_i;
        if (shift == 0) {
            val_i = chunk_i & 0x3F;
        } else {
            val_i = (uint8_t)(((chunk_i << shift) | (chunk_i >> (6 - shift))) & 0x3F);
        }

        acc += (val_i ^ ((uint32_t)i & 0x3F));
    }

    uint8_t idx = (uint8_t)(acc % 64);
    return B64_TABLE[idx];
}

/* 2.2. Расчет chi_gf */
static char compute_gf(const uint8_t *P, size_t N) {
    uint8_t S = 0;
    for (size_t i = 0; i < N; i++) {
        uint32_t exp_idx = (uint32_t)(i + 1) % 255;
        uint8_t alpha_pow = gf_exp[exp_idx];
        uint8_t y_i = gf_mul(P[i], alpha_pow);
        uint8_t inv_y = gf_inv[y_i];
        S ^= inv_y;
    }
    return (char)(33 + (S % 94));
}

/* 2.3. Расчет tau_tux */
static char compute_tux(const uint8_t *P, size_t N) {
    uint64_t W = 0;
    for (size_t i = 0; i < N; i++) {
        W += ((uint64_t)P[i]) << (i % 8);
    }

    if (W == 0) {
        return 'T';
    }

    int nu_2 = __builtin_ctzll(W);
    int nu_3 = 0;
    uint64_t tmp = W;
    while (tmp % 3 == 0) {
        nu_3++;
        tmp /= 3;
    }

    return (nu_3 > nu_2) ? 'X' : ((nu_2 > nu_3) ? 'U' : 'T');
}

/* 2.4. Расчет zeta */
static char compute_zeta(const uint8_t *P_curr, size_t N_curr, const uint8_t *P_prev, size_t N_prev) {
    if (!P_prev || N_prev == 0) {
        P_prev = TUX_SEED;
        N_prev = sizeof(TUX_SEED);
    }

    size_t max_len = (N_curr > N_prev) ? N_curr : N_prev;
    size_t d_H = 0;

    for (size_t j = 0; j < max_len; j++) {
        uint8_t b1 = (j < N_curr) ? P_curr[j] : 0x00;
        uint8_t b2 = (j < N_prev) ? P_prev[j] : 0x00;
        d_H += (size_t)__builtin_popcount((unsigned int)(b1 ^ b2));
    }

    return ZETA_TABLE[d_H % 5];
}

GbsvSignature gbsv_calculate(const uint8_t *P_curr, size_t N_curr, 
                             const uint8_t *P_prev, size_t N_prev) {
    gbsv_init();
    GbsvSignature sig;
    sig.b64 = compute_b64(P_curr, N_curr);
    sig.gf = compute_gf(P_curr, N_curr);
    sig.tux = compute_tux(P_curr, N_curr);
    sig.zeta = compute_zeta(P_curr, N_curr, P_prev, N_prev);
    return sig;
}

bool gbsv_parse_terminator(const char *raw_line, size_t len, 
                           size_t *out_prefix_len, GbsvSignature *out_sig) {
    while (len > 0 && (raw_line[len - 1] == '\r' || raw_line[len - 1] == '\n')) {
        len--;
    }

    if (len < 12) {
        return false;
    }

    size_t T_start = len - 11;

    if (raw_line[T_start + 0] != ':' || raw_line[T_start + 1] != '[') return false;
    if (raw_line[T_start + 3] != '|') return false;
    if (raw_line[T_start + 5] != '|') return false;
    if (raw_line[T_start + 7] != ']' || raw_line[T_start + 8] != ';') return false;
    if (raw_line[T_start + 10] != '}') return false;
    if (raw_line[T_start - 1] != ' ') return false;

    if (out_sig) {
        out_sig->b64 = raw_line[T_start + 2];
        out_sig->gf = raw_line[T_start + 4];
        out_sig->tux = raw_line[T_start + 6];
        out_sig->zeta = raw_line[T_start + 9];
    }

    if (out_prefix_len) {
        *out_prefix_len = T_start;
    }

    return true;
}

void gbsv_verify_line_or_die(const char *curr_line, size_t curr_len,
                             const char *prev_line, size_t prev_len) {
    size_t N_curr = 0;
    GbsvSignature actual_sig;

    if (!gbsv_parse_terminator(curr_line, curr_len, &N_curr, &actual_sig)) {
        troll_die(TR_GBSV_SYNTAX);
    }

    const uint8_t *P_curr = (const uint8_t *)curr_line;
    const uint8_t *P_prev = NULL;
    size_t N_prev = 0;

    if (prev_line && prev_len > 0) {
        size_t prev_prefix_len = 0;
        if (gbsv_parse_terminator(prev_line, prev_len, &prev_prefix_len, NULL)) {
            P_prev = (const uint8_t *)prev_line;
            N_prev = prev_prefix_len;
        } else {
            /* Если предыдущая строка не GBSV, обрезаем переводы строк */
            while (prev_len > 0 && (prev_line[prev_len - 1] == '\r' || prev_line[prev_len - 1] == '\n')) {
                prev_len--;
            }
            P_prev = (const uint8_t *)prev_line;
            N_prev = prev_len;
        }
    }

    GbsvSignature expected_sig = gbsv_calculate(P_curr, N_curr, P_prev, N_prev);

    if (actual_sig.b64 != expected_sig.b64) {
        troll_die(TR_GBSV_B64);
    }
    if (actual_sig.gf != expected_sig.gf) {
        troll_die(TR_GBSV_GF);
    }
    if (actual_sig.tux != expected_sig.tux) {
        troll_die(TR_GBSV_TUX);
    }
    if (actual_sig.zeta != expected_sig.zeta) {
        troll_die(TR_GBSV_ZETA);
    }
}
