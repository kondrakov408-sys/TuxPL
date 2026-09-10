#ifndef DEBRUIJN_H
#define DEBRUIJN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBRUIJN_NUM_STATES    252
#define DEBRUIJN_ALPHABET_SIZE 6
#define DEBRUIJN_DEFAULT_SEED  0x5A
#define DEBRUIJN_MAX_PATH_LEN  3

/* Alphabet order: T=0, t=1, U=2, u=3, X=4, x=5 */
int debruijn_char_to_idx(char c);
char debruijn_idx_to_char(int idx);

/*
 * State transition function: delta(q, c) = (q * 6 + idx(c)) % 252
 * Returns new state [0..251], or current q if c is invalid.
 */
uint8_t debruijn_transition(uint8_t q, char c);

/*
 * Mealy output function: lambda(q, c) = (q ^ (idx(c) * 7)) % 42
 * Returns opcode [0..41], or 0xFF if c is invalid.
 */
uint8_t debruijn_emit_opcode(uint8_t q, char c);

/*
 * Synthesizes shortest character sequence (from 1 to 3 chars) from start_state
 * that emits target_opcode at the last transition.
 * Writes null-terminated string to out_buf (requires max_len >= 4).
 * Returns number of characters written, or -1 on error.
 */
int debruijn_encode_opcode(uint8_t start_state, uint8_t target_opcode, char *out_buf, size_t max_len);

/*
 * Synthesizes a fixed 3-character path from start_state that emits target_opcode
 * at the 3rd transition.
 * Writes 3 chars + '\0' into out_buf (must be at least 4 bytes).
 * Returns 3 on success, -1 on error.
 */
int debruijn_encode_fixed3(uint8_t start_state, uint8_t target_opcode, char out_buf[4]);

/*
 * Feeds a sequence of length 'len' into the automaton starting from start_state.
 * Output opcode of the LAST transition is stored in *final_opcode.
 * Final automaton state after the sequence is stored in *final_state (if non-NULL).
 * Returns 0 on success, -1 if any character is invalid or len == 0.
 */
int debruijn_resolve_sequence(uint8_t start_state, const char *seq, size_t len,
                             uint8_t *final_opcode, uint8_t *final_state);

#ifdef __cplusplus
}
#endif

#endif /* DEBRUIJN_H */
