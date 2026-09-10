#include "debruijn.h"
#include <string.h>

static const char DEBRUIJN_ALPHABET[DEBRUIJN_ALPHABET_SIZE] = {'T', 't', 'U', 'u', 'X', 'x'};

int debruijn_char_to_idx(char c) {
    switch (c) {
        case 'T': return 0;
        case 't': return 1;
        case 'U': return 2;
        case 'u': return 3;
        case 'X': return 4;
        case 'x': return 5;
        default:  return -1;
    }
}

char debruijn_idx_to_char(int idx) {
    if (idx >= 0 && idx < DEBRUIJN_ALPHABET_SIZE) {
        return DEBRUIJN_ALPHABET[idx];
    }
    return '\0';
}

uint8_t debruijn_transition(uint8_t q, char c) {
    int idx = debruijn_char_to_idx(c);
    if (idx < 0) {
        return q;
    }
    return (uint8_t)(((uint32_t)q * 6 + (uint32_t)idx) % DEBRUIJN_NUM_STATES);
}

uint8_t debruijn_emit_opcode(uint8_t q, char c) {
    int idx = debruijn_char_to_idx(c);
    if (idx < 0) {
        return 0xFF;
    }
    return (uint8_t)(((uint32_t)q ^ ((uint32_t)idx * 7)) % 42);
}

int debruijn_encode_opcode(uint8_t start_state, uint8_t target_opcode, char *out_buf, size_t max_len) {
    if (!out_buf || max_len < 2 || target_opcode >= 42) {
        return -1;
    }

    /* Depth 1: 6 checks */
    for (int c0 = 0; c0 < DEBRUIJN_ALPHABET_SIZE; c0++) {
        char ch0 = DEBRUIJN_ALPHABET[c0];
        if (debruijn_emit_opcode(start_state, ch0) == target_opcode) {
            out_buf[0] = ch0;
            out_buf[1] = '\0';
            return 1;
        }
    }

    if (max_len < 3) return -1;

    /* Depth 2: 36 checks */
    for (int c0 = 0; c0 < DEBRUIJN_ALPHABET_SIZE; c0++) {
        char ch0 = DEBRUIJN_ALPHABET[c0];
        uint8_t q1 = debruijn_transition(start_state, ch0);
        for (int c1 = 0; c1 < DEBRUIJN_ALPHABET_SIZE; c1++) {
            char ch1 = DEBRUIJN_ALPHABET[c1];
            if (debruijn_emit_opcode(q1, ch1) == target_opcode) {
                out_buf[0] = ch0;
                out_buf[1] = ch1;
                out_buf[2] = '\0';
                return 2;
            }
        }
    }

    if (max_len < 4) return -1;

    /* Depth 3: 216 checks (guaranteed reachability for any state and opcode) */
    for (int c0 = 0; c0 < DEBRUIJN_ALPHABET_SIZE; c0++) {
        char ch0 = DEBRUIJN_ALPHABET[c0];
        uint8_t q1 = debruijn_transition(start_state, ch0);
        for (int c1 = 0; c1 < DEBRUIJN_ALPHABET_SIZE; c1++) {
            char ch1 = DEBRUIJN_ALPHABET[c1];
            uint8_t q2 = debruijn_transition(q1, ch1);
            for (int c2 = 0; c2 < DEBRUIJN_ALPHABET_SIZE; c2++) {
                char ch2 = DEBRUIJN_ALPHABET[c2];
                if (debruijn_emit_opcode(q2, ch2) == target_opcode) {
                    out_buf[0] = ch0;
                    out_buf[1] = ch1;
                    out_buf[2] = ch2;
                    out_buf[3] = '\0';
                    return 3;
                }
            }
        }
    }

    return -1;
}

int debruijn_encode_fixed3(uint8_t start_state, uint8_t target_opcode, char out_buf[4]) {
    if (!out_buf || target_opcode >= 42) {
        return -1;
    }

    for (int c0 = 0; c0 < DEBRUIJN_ALPHABET_SIZE; c0++) {
        char ch0 = DEBRUIJN_ALPHABET[c0];
        uint8_t q1 = debruijn_transition(start_state, ch0);
        for (int c1 = 0; c1 < DEBRUIJN_ALPHABET_SIZE; c1++) {
            char ch1 = DEBRUIJN_ALPHABET[c1];
            uint8_t q2 = debruijn_transition(q1, ch1);
            for (int c2 = 0; c2 < DEBRUIJN_ALPHABET_SIZE; c2++) {
                char ch2 = DEBRUIJN_ALPHABET[c2];
                if (debruijn_emit_opcode(q2, ch2) == target_opcode) {
                    out_buf[0] = ch0;
                    out_buf[1] = ch1;
                    out_buf[2] = ch2;
                    out_buf[3] = '\0';
                    return 3;
                }
            }
        }
    }

    return -1;
}

int debruijn_resolve_sequence(uint8_t start_state, const char *seq, size_t len,
                             uint8_t *final_opcode, uint8_t *final_state) {
    if (!seq || len == 0) {
        return -1;
    }

    uint8_t q = start_state;
    uint8_t op = 0xFF;

    for (size_t i = 0; i < len; i++) {
        char c = seq[i];
        int idx = debruijn_char_to_idx(c);
        if (idx < 0) {
            return -1;
        }
        op = debruijn_emit_opcode(q, c);
        q = debruijn_transition(q, c);
    }

    if (final_opcode) {
        *final_opcode = op;
    }
    if (final_state) {
        *final_state = q;
    }
    return 0;
}
