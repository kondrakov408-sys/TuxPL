#ifndef RNS_H
#define RNS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RNS_NUM_MODULI   6
#define RNS_MODULUS_M    7436429ULL
#define RNS_OPERAND_LEN  12
#define RNS_BUF_SIZE     13  /* 12 characters + null terminator */

/*
 * Decomposes an integer val in [0..M-1] into 6 residues modulo (7, 11, 13, 17, 19, 23).
 */
void rns_decompose(uint32_t val, uint8_t residues[RNS_NUM_MODULI]);

/*
 * Reconstructs an integer from 6 residues using Chinese Remainder Theorem (CRT)
 * with exact Bézout modular inverses:
 * C = {6374082, 676039, 1144066, 5249244, 782782, 646646}.
 */
uint32_t rns_reconstruct(const uint8_t residues[RNS_NUM_MODULI]);

/*
 * Encodes an operand val in [0..M-1] into exactly 12 characters over the
 * alphabet {T, t, U, u, X, x} followed by a null-terminator.
 * out_buf MUST be at least 13 bytes (RNS_BUF_SIZE).
 * Returns 0 on success, -1 if val >= RNS_MODULUS_M or out_buf is NULL.
 */
int rns_encode_operand(uint32_t val, char out_buf[RNS_BUF_SIZE]);

/*
 * Decodes an operand from a buffer with explicit length 'len'.
 * Reads exactly the first 12 characters (requires len >= 12).
 * Does NOT require a null-terminator, enabling safe parsing from dense instruction streams.
 * Returns 0 on success and stores decoded value in *val_out.
 * Returns -1 if in_buf is NULL, len < 12, or contains invalid characters.
 */
int rns_decode_operand(const char *in_buf, size_t len, uint32_t *val_out);

/*
 * Convenience wrapper for null-terminated string:
 * requires strlen(str) == 12.
 */
int rns_decode_operand_str(const char *str, uint32_t *val_out);

/*
 * Accessors for modular constants.
 */
uint64_t rns_get_modulus_m(void);
const uint32_t *rns_get_moduli(void);
const uint64_t *rns_get_bezout_constants(void);

#ifdef __cplusplus
}
#endif

#endif /* RNS_H */
