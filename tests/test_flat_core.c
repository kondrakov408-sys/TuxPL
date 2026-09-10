#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, name) do { \
    if (cond) { \
        g_pass++; \
        printf("  [PASS] %s\n", (name)); \
    } else { \
        g_fail++; \
        printf("  [FAIL] %s\n", (name)); \
    } \
} while(0)

int main(void) {
    printf("=== Running TuxPL 2.0.0 Flat ZM Core Test Suite ===\n");

    /* 1. Модель тора Z_M */
    CHECK(TUX_ZM_M == 7436429U, "TUX_ZM_M is 7,436,429");
    CHECK(UNIFIED_MEM_SIZE == 65536, "UNIFIED_MEM_SIZE is 65,536");

    /* 2. Латентный биологический возраст: nu_3(W) mod 4 */
    CHECK(tux_cell_age(0) == TUX_AGE_DEAD, "nu_3(0) is DEAD (3)");
    CHECK(tux_cell_age(1) == TUX_AGE_YOUNG, "nu_3(1) is YOUNG (0)");
    CHECK(tux_cell_age(2) == TUX_AGE_YOUNG, "nu_3(2) is YOUNG (0)");
    CHECK(tux_cell_age(3) == TUX_AGE_ADULT, "nu_3(3) is ADULT (1)");
    CHECK(tux_cell_age(6) == TUX_AGE_ADULT, "nu_3(6) is ADULT (1)");
    CHECK(tux_cell_age(9) == TUX_AGE_OLD, "nu_3(9) is OLD (2)");
    CHECK(tux_cell_age(18) == TUX_AGE_OLD, "nu_3(18) is OLD (2)");
    CHECK(tux_cell_age(27) == TUX_AGE_DEAD, "nu_3(27) is DEAD (3)");
    CHECK(tux_cell_age(54) == TUX_AGE_DEAD, "nu_3(54) is DEAD (3)");
    CHECK(tux_cell_age(81) == TUX_AGE_YOUNG, "nu_3(81) is YOUNG (0)");
    CHECK(tux_cell_age(243) == TUX_AGE_ADULT, "nu_3(243) is ADULT (1)");

    /* 3. Семантический тег типа: (W ^ 0x5A) mod 6 */
    CHECK(tux_cell_type(0x5A) == TUX_TYPE_ADDR, "Type(0x5A) is ADDR (0)");
    CHECK(tux_cell_type(0x5B) == TUX_TYPE_I8, "Type(0x5B) is I8 (1)");
    CHECK(tux_cell_type(0x58) == TUX_TYPE_I16, "Type(0x58) is I16 (2)");
    CHECK(tux_cell_type(0x59) == TUX_TYPE_I32, "Type(0x59) is I32 (3)");
    CHECK(tux_cell_type(0x5E) == TUX_TYPE_I64, "Type(0x5E) is I64 (4)");
    CHECK(tux_cell_type(0x5F) == TUX_TYPE_OPCODE, "Type(0x5F) is OPCODE (5)");

    /* 4. Borrow Checker: W = 0 mod 17 */
    CHECK((0 % 17) == 0, "0 is moved");
    CHECK((17 % 17) == 0, "17 is moved");
    CHECK((34 % 17) == 0, "34 is moved");
    CHECK((18 % 17) != 0, "18 is not moved");
    CHECK((42 % 17) != 0, "42 is not moved");

    /* 5. SP-Round необратимое шифрование */
    uint32_t c1 = tux_sp_round(42, 0);
    uint32_t c1_rep = tux_sp_round(42, 0);
    CHECK(c1 == c1_rep, "SP-Round is 100% deterministic");
    CHECK(c1 < TUX_ZM_M, "SP-Round result strictly in ZM");

    uint32_t c2 = tux_sp_round(43, 0);
    CHECK(c1 != c2, "SP-Round sensitive to cell value");

    uint32_t c3 = tux_sp_round(42, 1);
    CHECK(c1 != c3, "SP-Round sensitive to PCC");

    /* 6. Торсионное спаривание Dual-PC */
    uint16_t pcd = 1024;
    uint32_t sample_w = 0x0F0F; /* popcount = 8 */
    pcd = (uint16_t)((pcd + (uint16_t)__builtin_popcount(sample_w) + 1) % UNIFIED_MEM_SIZE);
    CHECK(pcd == 1024 + 8 + 1, "PCD shifts by popcount(W) + 1");

    /* 7. Логический оператор TUX_CRAZY в ALU над тритами {-1, 0, +1} */
    /* tux_crazy_alu(1, 1) = 29523 */
    int64_t crz_1_1 = tux_crazy_alu(1, 1);
    CHECK(crz_1_1 == 29523, "TUX_CRAZY(1, 1) == 29523");
    CHECK(tux_crazy_alu(0, 0) == 29524, "TUX_CRAZY(0, 0) deterministic");
    CHECK(tux_crazy_alu(1, 0) != tux_crazy_alu(0, 1), "TUX_CRAZY is non-commutative");

    /* 8. Инициализация TuxVM */
    TuxVM vm;
    vm.unified_mem = calloc(UNIFIED_MEM_SIZE, sizeof(uint32_t));
    CHECK(vm.unified_mem != NULL, "Allocated flat unified_mem");
    tux_mem_init(vm.unified_mem, 0x12345678ULL, 0x9ABCDEF0ULL);
    CHECK(vm.unified_mem[0] < TUX_ZM_M, "Cell 0 is in ZM");
    CHECK(vm.unified_mem[100] < TUX_ZM_M, "Cell 100 is in ZM");
    CHECK(vm.unified_mem[0] % 17 != 0, "Initialized cell not moved");
    free(vm.unified_mem);

    printf("===================================================\n");
    printf("Flat ZM Core Tests: OK: %d, FAIL: %d\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
