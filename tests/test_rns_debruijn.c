#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "../src/debruijn.h"
#include "../src/rns.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "[-] ASSERTION FAILED: %s (at line %d)\n", msg, __LINE__); \
        return 0; \
    } \
    g_tests_passed++; \
} while (0)

/* 1. Математическая верификация констант Безу и модуля M */
static int test_rns_constants(void) {
    uint64_t M = rns_get_modulus_m();
    TEST_ASSERT(M == 7436429ULL, "M must equal exactly 7436429 (7*11*13*17*19*23)");

    const uint32_t *m = rns_get_moduli();
    uint64_t prod = 1;
    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        prod *= m[i];
    }
    TEST_ASSERT(prod == M, "Product of moduli must match M");

    const uint64_t *C = rns_get_bezout_constants();
    for (int i = 0; i < RNS_NUM_MODULI; i++) {
        for (int j = 0; j < RNS_NUM_MODULI; j++) {
            if (i == j) {
                TEST_ASSERT((C[i] % m[j]) == 1ULL, "C[i] mod m[i] == 1");
            } else {
                TEST_ASSERT((C[i] % m[j]) == 0ULL, "C[i] mod m[j] == 0 for j != i");
            }
        }
    }
    return 1;
}

/* 2. Изоморфизм CRT: граничные значения и плотный поток без нуль-терминатора */
static int test_rns_isomorphism_edge_cases(void) {
    const uint32_t edges[] = {
        0, 1, 2, 6, 7, 10, 11, 12, 13, 16, 17, 18, 19, 22, 23,
        42, 100, 255, 1024, 65535, 127928, 1000000, 7436427, 7436428
    };
    size_t n_edges = sizeof(edges) / sizeof(edges[0]);

    for (size_t i = 0; i < n_edges; i++) {
        uint32_t val = edges[i];
        char buf[RNS_BUF_SIZE];
        int enc_res = rns_encode_operand(val, buf);
        TEST_ASSERT(enc_res == 0, "rns_encode_operand must succeed for valid val");
        TEST_ASSERT(strlen(buf) == 12, "Encoded buffer must be 12 characters");

        /* Проверка декодирования по строке */
        uint32_t dec_val = 0;
        int dec_res = rns_decode_operand_str(buf, &dec_val);
        TEST_ASSERT(dec_res == 0, "rns_decode_operand_str must succeed");
        TEST_ASSERT(dec_val == val, "Decoded value must match original");

        /* Проверка декодирования из плотного потока (без \0 внутри и с хвостом) */
        char dense_stream[32];
        memcpy(dense_stream, buf, 12);
        memcpy(dense_stream + 12, "TAIL_OPCODES_AND_GARBAGE", 20);
        uint32_t dense_dec = 0;
        int dense_res = rns_decode_operand(dense_stream, sizeof(dense_stream), &dense_dec);
        TEST_ASSERT(dense_res == 0, "rns_decode_operand must parse dense stream with len >= 12");
        TEST_ASSERT(dense_dec == val, "Dense stream decode must match original");
    }

    /* Проверка выхода за границы диапазона [0..M-1] */
    char dummy_buf[RNS_BUF_SIZE];
    TEST_ASSERT(rns_encode_operand(7436429, dummy_buf) == -1, "Encode M must fail");
    TEST_ASSERT(rns_encode_operand(10000000, dummy_buf) == -1, "Encode > M must fail");
    TEST_ASSERT(rns_encode_operand(0, NULL) == -1, "Encode with NULL must fail");

    return 1;
}

/* 3. Массовый стресс-тест CRT на 100,000 псевдослучайных чисел */
static int test_rns_isomorphism_stress_100k(void) {
    uint64_t rng = 0x123456789ABCDEF0ULL;
    const uint64_t M = rns_get_modulus_m();

    for (int iter = 0; iter < 100000; iter++) {
        rng = rng * 6364136223846793005ULL + 1ULL;
        uint32_t val = (uint32_t)((rng >> 32) % M);

        char buf[RNS_BUF_SIZE];
        if (rns_encode_operand(val, buf) != 0) {
            TEST_ASSERT(0, "Stress encode failed");
        }
        uint32_t dec = 0;
        if (rns_decode_operand(buf, 12, &dec) != 0) {
            TEST_ASSERT(0, "Stress decode failed");
        }
        if (dec != val) {
            TEST_ASSERT(0, "Stress mismatch between val and dec");
        }
    }
    TEST_ASSERT(1, "100,000 CRT iterations passed");
    return 1;
}

/* 4. Лавинный эффект RNS и валидация неверных входных данных */
static int test_rns_avalanche_and_errors(void) {
    uint32_t base = 42000;
    char buf[RNS_BUF_SIZE];
    rns_encode_operand(base, buf);

    /* Мутация одного символа */
    for (int pos = 0; pos < 12; pos++) {
        char orig_char = buf[pos];
        char mutant_char = (orig_char == 'T') ? 'x' : 'T';
        buf[pos] = mutant_char;

        uint32_t mutated_val = 0;
        int res = rns_decode_operand(buf, 12, &mutated_val);
        TEST_ASSERT(res == 0, "Mutated string over valid alphabet must decode");
        TEST_ASSERT(mutated_val != base, "1-symbol mutation must change decoded value");

        buf[pos] = orig_char; /* восстановление */
    }

    /* Тест невалидных символов и длины */
    uint32_t dec_out = 0;
    TEST_ASSERT(rns_decode_operand(NULL, 12, &dec_out) == -1, "NULL buffer must fail");
    TEST_ASSERT(rns_decode_operand(buf, 11, &dec_out) == -1, "len < 12 must fail");
    
    char corrupt_buf[12];
    memcpy(corrupt_buf, buf, 12);
    corrupt_buf[5] = 'Z'; /* Не в алфавите {T,t,U,u,X,x} */
    TEST_ASSERT(rns_decode_operand(corrupt_buf, 12, &dec_out) == -1, "Invalid char must fail");

    return 1;
}

/* 5. Автомат де Брейна: полнота графа (252 состояния x 42 опкода = 10,584 проверок) */
static int test_debruijn_completeness(void) {
    for (uint16_t q = 0; q < DEBRUIJN_NUM_STATES; q++) {
        for (uint8_t target_op = 0; target_op < 42; target_op++) {
            /* 1) Проверка синтеза пути фиксированной длины 3 */
            char fixed3_buf[4];
            int f_res = debruijn_encode_fixed3((uint8_t)q, target_op, fixed3_buf);
            TEST_ASSERT(f_res == 3, "debruijn_encode_fixed3 must return 3");
            TEST_ASSERT(strlen(fixed3_buf) == 3, "fixed3 length must be 3");

            uint8_t res_op = 0xFF;
            uint8_t next_q = 0xFF;
            int r_res = debruijn_resolve_sequence((uint8_t)q, fixed3_buf, 3, &res_op, &next_q);
            TEST_ASSERT(r_res == 0, "debruijn_resolve_sequence must succeed");
            TEST_ASSERT(res_op == target_op, "Resolved opcode must match target opcode");

            /* 2) Проверка поиска кратчайшего пути (длина 1..3) */
            char shortest_buf[8];
            int s_len = debruijn_encode_opcode((uint8_t)q, target_op, shortest_buf, sizeof(shortest_buf));
            TEST_ASSERT(s_len >= 1 && s_len <= 3, "Shortest path length must be in [1..3]");

            uint8_t s_op = 0xFF;
            int s_res = debruijn_resolve_sequence((uint8_t)q, shortest_buf, (size_t)s_len, &s_op, NULL);
            TEST_ASSERT(s_res == 0, "Resolve shortest path must succeed");
            TEST_ASSERT(s_op == target_op, "Resolved shortest path opcode must match target");
        }
    }
    return 1;
}

/* 6. Граничные случаи автомата де Брейна */
static int test_debruijn_edge_cases(void) {
    uint8_t op = 0, q_out = 0;
    TEST_ASSERT(debruijn_resolve_sequence(0, NULL, 3, &op, &q_out) == -1, "NULL seq must fail");
    TEST_ASSERT(debruijn_resolve_sequence(0, "TTT", 0, &op, &q_out) == -1, "len 0 must fail");
    TEST_ASSERT(debruijn_resolve_sequence(0, "T@T", 3, &op, &q_out) == -1, "invalid char must fail");

    char buf[4];
    TEST_ASSERT(debruijn_encode_fixed3(0, 42, buf) == -1, "target op 42 must fail");
    TEST_ASSERT(debruijn_encode_fixed3(0, 255, buf) == -1, "target op 255 must fail");
    TEST_ASSERT(debruijn_encode_opcode(0, 42, buf, sizeof(buf)) == -1, "target op 42 shortest must fail");

    return 1;
}

int main(void) {
    printf("[*] Starting TuxPL De Bruijn Mealy Machine & RNS-CRT Test Suite...\n");

    if (!test_rns_constants()) {
        fprintf(stderr, "[-] test_rns_constants FAILED\n");
        return 1;
    }
    printf("  [+] RNS Constants & Bézout Inverses: PASS\n");

    if (!test_rns_isomorphism_edge_cases()) {
        fprintf(stderr, "[-] test_rns_isomorphism_edge_cases FAILED\n");
        return 1;
    }
    printf("  [+] RNS CRT Isomorphism & Edge Cases: PASS\n");

    if (!test_rns_isomorphism_stress_100k()) {
        fprintf(stderr, "[-] test_rns_isomorphism_stress_100k FAILED\n");
        return 1;
    }
    printf("  [+] RNS 100,000 Pseudorandom Stress Test: PASS\n");

    if (!test_rns_avalanche_and_errors()) {
        fprintf(stderr, "[-] test_rns_avalanche_and_errors FAILED\n");
        return 1;
    }
    printf("  [+] RNS Avalanche Diffusion & Error Handling: PASS\n");

    if (!test_debruijn_completeness()) {
        fprintf(stderr, "[-] test_debruijn_completeness FAILED\n");
        return 1;
    }
    printf("  [+] De Bruijn Automaton Completeness (10,584 pairs BFS): PASS\n");

    if (!test_debruijn_edge_cases()) {
        fprintf(stderr, "[-] test_debruijn_edge_cases FAILED\n");
        return 1;
    }
    printf("  [+] De Bruijn Edge Cases & Error Handling: PASS\n");

    printf("\n[SUCCESS] All %d assertions PASSED! De Bruijn & RNS-CRT modules verified 100%%.\n", g_tests_passed);
    return 0;
}
