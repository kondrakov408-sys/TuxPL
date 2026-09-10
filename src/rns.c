#include "rns.h"
#include "debruijn.h"
#include <string.h>

static const uint32_t RNS_M[RNS_NUM_MODULI] = {7, 11, 13, 17, 19, 23};

static const uint64_t RNS_C[RNS_NUM_MODULI] = {
    6374082ULL,  /* m0 = 7  */
    676039ULL,   /* m1 = 11 */
    1144066ULL,  /* m2 = 13 */
    5249244ULL,  /* m3 = 17 */
    782782ULL,   /* m4 = 19 */
    646646ULL    /* m5 = 23 */
};

static const char RNS_ALPHABET[6] = {'T', 't', 'U', 'u', 'X', 'x'};

void rns_decompose(uint32_t val, uint8_t residues[RNS_NUM_MODULI]) {
    if (!residues) return;
    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        residues[i] = (uint8_t)(val % RNS_M[i]);
    }
}

uint32_t rns_reconstruct(const uint8_t residues[RNS_NUM_MODULI]) {
    if (!residues) return 0;
    uint64_t sum = 0;
    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        sum = (sum + (uint64_t)residues[i] * RNS_C[i]) % RNS_MODULUS_M;
    }
    return (uint32_t)sum;
}

int rns_encode_operand(uint32_t val, char out_buf[RNS_BUF_SIZE]) {
    if (!out_buf || val >= RNS_MODULUS_M) {
        return -1;
    }

    uint8_t residues[RNS_NUM_MODULI];
    rns_decompose(val, residues);

    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        uint8_t r = residues[i];
        uint8_t d0 = (uint8_t)(r / 6);
        uint8_t d1 = (uint8_t)(r % 6);
        out_buf[2 * i]     = RNS_ALPHABET[d0];
        out_buf[2 * i + 1] = RNS_ALPHABET[d1];
    }
    out_buf[12] = '\0';
    return 0;
}

int rns_decode_operand(const char *in_buf, size_t len, uint32_t *val_out) {
    if (!in_buf || len < RNS_OPERAND_LEN || !val_out) {
        return -1;
    }

    uint8_t residues[RNS_NUM_MODULI];
    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        int d0 = debruijn_char_to_idx(in_buf[2 * i]);
        int d1 = debruijn_char_to_idx(in_buf[2 * i + 1]);
        if (d0 < 0 || d1 < 0) {
            return -1;
        }
        uint32_t v = (uint32_t)d0 * 6 + (uint32_t)d1;
        residues[i] = (uint8_t)(v % RNS_M[i]);
    }

    *val_out = rns_reconstruct(residues);
    return 0;
}

int rns_decode_operand_str(const char *str, uint32_t *val_out) {
    if (!str || strlen(str) != RNS_OPERAND_LEN) {
        return -1;
    }
    return rns_decode_operand(str, RNS_OPERAND_LEN, val_out);
}

uint64_t rns_get_modulus_m(void) {
    return RNS_MODULUS_M;
}

const uint32_t *rns_get_moduli(void) {
    return RNS_M;
}

const uint64_t *rns_get_bezout_constants(void) {
    return RNS_C;
}
