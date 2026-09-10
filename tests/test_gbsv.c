#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/gbsv.h"

static int pass_count = 0;
static int fail_count = 0;

#define TEST_ASSERT(cond, name) do { \
    if (cond) { \
        pass_count++; \
        printf("  [PASS] %s\n", (name)); \
    } else { \
        fail_count++; \
        printf("  [FAIL] %s (line %d)\n", (name), __LINE__); \
    } \
} while (0)

static void test_galois_field_isomorphism(void) {
    gbsv_init();
    bool all_inv_ok = true;
    for (int x = 1; x <= 255; x++) {
        uint8_t inv = gbsv_gf_inv((uint8_t)x);
        uint8_t prod = gbsv_gf_mul((uint8_t)x, inv);
        if (prod != 1) {
            all_inv_ok = false;
            break;
        }
    }
    TEST_ASSERT(all_inv_ok, "F_{2^8} multiplicative inverse isomorphism (x * x^-1 == 1 for x in [1, 255])");
    TEST_ASSERT(gbsv_gf_inv(0) == 0, "F_{2^8} zero element inverse (0^-1 == 0)");
    TEST_ASSERT(gbsv_gf_mul(0, 42) == 0, "F_{2^8} zero multiplication (0 * x == 0)");
}

static void test_special_chars_parsing(void) {
    /* Проверка устойчивости к спецсимволам '|' (124) и ']' (93) */
    const char *line_pipe = "{:[~'Tux'~] (tUX) :[e|||U];~}";
    GbsvSignature sig_pipe;
    size_t pfx_len = 0;
    bool ok_pipe = gbsv_parse_terminator(line_pipe, strlen(line_pipe), &pfx_len, &sig_pipe);
    TEST_ASSERT(ok_pipe, "Positional parser parses chi_gf == '|' without confusion");
    TEST_ASSERT(sig_pipe.b64 == 'e' && sig_pipe.gf == '|' && sig_pipe.tux == 'U' && sig_pipe.zeta == '~',
                "Tokens extracted correctly when chi_gf == '|'");
    TEST_ASSERT(pfx_len == strlen("{:[~'Tux'~] (tUX) "), "Prefix length matches accurately with '|'");

    const char *line_bracket = "{:[~'Tux'~] (tUX) :[A|]|T];!}";
    GbsvSignature sig_bracket;
    bool ok_bracket = gbsv_parse_terminator(line_bracket, strlen(line_bracket), &pfx_len, &sig_bracket);
    TEST_ASSERT(ok_bracket, "Positional parser parses chi_gf == ']' without confusion");
    TEST_ASSERT(sig_bracket.b64 == 'A' && sig_bracket.gf == ']' && sig_bracket.tux == 'T' && sig_bracket.zeta == '!',
                "Tokens extracted correctly when chi_gf == ']'");

    /* Проверка спецсимволов ';' и '}' внутри chi_gf */
    const char *line_semi = "{:[~'Tux'~] (tUX) :[Z|;|X];?}";
    GbsvSignature sig_semi;
    bool ok_semi = gbsv_parse_terminator(line_semi, strlen(line_semi), &pfx_len, &sig_semi);
    TEST_ASSERT(ok_semi, "Positional parser parses chi_gf == ';'");
    TEST_ASSERT(sig_semi.gf == ';', "Token chi_gf extracted as ';'");
}

static void test_golden_vector_and_sensitivity(void) {
    const char *prefix1 = "{:[~'Tux'~] (tUX) (TuuUuuUUuX) ";
    size_t len1 = strlen(prefix1);

    GbsvSignature sig1 = gbsv_calculate((const uint8_t *)prefix1, len1, NULL, 0);
    GbsvSignature sig2 = gbsv_calculate((const uint8_t *)prefix1, len1, NULL, 0);

    TEST_ASSERT(sig1.b64 == sig2.b64 && sig1.gf == sig2.gf &&
                sig1.tux == sig2.tux && sig1.zeta == sig2.zeta,
                "GBSV calculation is 100% deterministic");

    /* Построение валидной строки и проверка верификатором */
    char full_line[256];
    snprintf(full_line, sizeof(full_line), "%s:[%c|%c|%c];%c}",
             prefix1, sig1.b64, sig1.gf, sig1.tux, sig1.zeta);

    size_t parsed_pfx_len = 0;
    GbsvSignature parsed_sig;
    bool parse_ok = gbsv_parse_terminator(full_line, strlen(full_line), &parsed_pfx_len, &parsed_sig);
    TEST_ASSERT(parse_ok, "Constructed line parses successfully with GBSV terminator");
    TEST_ASSERT(parsed_pfx_len == len1, "Parsed prefix length matches original prefix exactly");
    TEST_ASSERT(parsed_sig.b64 == sig1.b64 && parsed_sig.gf == sig1.gf &&
                parsed_sig.tux == sig1.tux && parsed_sig.zeta == sig1.zeta,
                "Parsed signature matches computed signature");

    /* gbsv_verify_line_or_die не падает на валидной строке */
    gbsv_verify_line_or_die(full_line, strlen(full_line), NULL, 0);
    pass_count++;
    printf("  [PASS] gbsv_verify_line_or_die succeeds on golden line\n");

    /* Проверка 1-битной чувствительности */
    uint8_t mutated[256];
    memcpy(mutated, prefix1, len1);
    mutated[len1 / 2] ^= 0x01; /* инвертируем ровно 1 бит в середине префикса */

    GbsvSignature sig_mut = gbsv_calculate(mutated, len1, NULL, 0);
    bool b64_changed = (sig1.b64 != sig_mut.b64);
    bool gf_changed = (sig1.gf != sig_mut.gf);
    bool zeta_changed = (sig1.zeta != sig_mut.zeta);

    TEST_ASSERT(b64_changed, "1-bit mutation alters sigma_b64 projection");
    TEST_ASSERT(gf_changed, "1-bit mutation alters chi_gf Galois field syndrome");
    TEST_ASSERT(zeta_changed, "1-bit mutation alters zeta Hamming collapse");
}

static void test_invalid_syntax_rejection(void) {
    size_t pfx_len = 0;
    GbsvSignature sig;

    /* Слишком короткая строка */
    TEST_ASSERT(!gbsv_parse_terminator("short", 5, &pfx_len, &sig), "Rejects string shorter than 12 chars");

    /* Нет разделяющего пробела перед :[ */
    const char *no_space = "{:[~'Tux'~]:[e|#|U];~}";
    TEST_ASSERT(!gbsv_parse_terminator(no_space, strlen(no_space), &pfx_len, &sig), "Rejects missing space before :[");

    /* Нарушены скобки или разделители */
    const char *bad_delims = "{:[~'Tux'~] :[e#U];~}";
    TEST_ASSERT(!gbsv_parse_terminator(bad_delims, strlen(bad_delims), &pfx_len, &sig), "Rejects missing pipes inside :[...] ");

    const char *bad_semi = "{:[~'Tux'~] :[e|#|U]:~}";
    TEST_ASSERT(!gbsv_parse_terminator(bad_semi, strlen(bad_semi), &pfx_len, &sig), "Rejects missing semicolon before zeta");

    const char *bad_end = "{:[~'Tux'~] :[e|#|U];~]";
    TEST_ASSERT(!gbsv_parse_terminator(bad_end, strlen(bad_end), &pfx_len, &sig), "Rejects closing bracket instead of brace");
}

static void test_multiline_chaining(void) {
    const char *p1 = "{:[~'Tux'~] (TuuUuuUuuuX) ";
    const char *p2 = "{:[~'Tux'~] (tUX) (TuuUUuUuuUX) ";
    const char *p3 = "{:[~'Tux'~] (tUX) (TuuUuUuX)\t(tUX) ";

    size_t l1 = strlen(p1);
    size_t l2 = strlen(p2);
    size_t l3 = strlen(p3);

    GbsvSignature s1 = gbsv_calculate((const uint8_t *)p1, l1, NULL, 0);
    char line1[256];
    snprintf(line1, sizeof(line1), "%s:[%c|%c|%c];%c}", p1, s1.b64, s1.gf, s1.tux, s1.zeta);

    GbsvSignature s2 = gbsv_calculate((const uint8_t *)p2, l2, (const uint8_t *)p1, l1);
    char line2[256];
    snprintf(line2, sizeof(line2), "%s:[%c|%c|%c];%c}", p2, s2.b64, s2.gf, s2.tux, s2.zeta);

    GbsvSignature s3 = gbsv_calculate((const uint8_t *)p3, l3, (const uint8_t *)p2, l2);
    char line3[256];
    snprintf(line3, sizeof(line3), "%s:[%c|%c|%c];%c}", p3, s3.b64, s3.gf, s3.tux, s3.zeta);

    /* Проверка последовательной верификации всей цепочки */
    gbsv_verify_line_or_die(line1, strlen(line1), NULL, 0);
    gbsv_verify_line_or_die(line2, strlen(line2), line1, strlen(line1));
    gbsv_verify_line_or_die(line3, strlen(line3), line2, strlen(line2));

    pass_count++;
    printf("  [PASS] Multiline chain verification across consecutive lines (1 -> 2 -> 3)\n");
}

int main(void) {
    printf("=== Running TuxPL 2.0.0 GBSV Verification Test Suite ===\n");

    test_galois_field_isomorphism();
    test_special_chars_parsing();
    test_golden_vector_and_sensitivity();
    test_invalid_syntax_rejection();
    test_multiline_chaining();

    printf("========================================================\n");
    printf("GBSV Tests: OK: %d, FAIL: %d\n", pass_count, fail_count);

    return (fail_count > 0) ? 1 : 0;
}
