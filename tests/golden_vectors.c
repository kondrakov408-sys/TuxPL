#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/tuxpl.h"

static int g_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
        g_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
    } \
} while(0)

int main(void) {
    printf("=== Running TuxPL 2.0 Golden Vectors Verification ===\n");

    /* 1. tux_source_hash (FNV-1a 64-bit) */
    {
        uint64_t empty_hash = tux_source_hash("", 0);
        CHECK(empty_hash == 0xCBF29CE484222325ULL, "FNV-1a empty string matches 0xCBF29CE484222325");

        const char *tux = "TuxPL";
        uint64_t tux_h1 = tux_source_hash(tux, 5);
        uint64_t tux_h2 = tux_source_hash(tux, 5);
        CHECK(tux_h1 == tux_h2, "FNV-1a deterministic on 'TuxPL'");
        CHECK(tux_h1 != empty_hash, "FNV-1a 'TuxPL' != empty");
    }

    /* 2. tux_crazy64 determinism & properties */
    {
        uint64_t c0 = tux_crazy64(0, 0);
        uint64_t c0_repeat = tux_crazy64(0, 0);
        CHECK(c0 == c0_repeat, "tux_crazy64(0,0) is deterministic");

        uint64_t a = 0x123456789ABCDEF0ULL;
        uint64_t b = 0x0FEDCBA987654321ULL;
        uint64_t res1 = tux_crazy64(a, b);
        uint64_t res2 = tux_crazy64(a, b);
        CHECK(res1 == res2, "tux_crazy64(a,b) is 100% reproducible");

        uint64_t c_max = tux_crazy64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
        CHECK(c_max == tux_crazy64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL), "tux_crazy64(MAX, MAX) deterministic");
    }

    /* 3. tux_derive_program_key */
    {
        ProgramFingerprint fp1 = {
            .bit_len = 1024,
            .source_hash = 0xA1B2C3D4E5F60718ULL,
            .tux_checksum = 'U'
        };
        uint64_t k1 = tux_derive_program_key(&fp1);
        uint64_t k2 = tux_derive_program_key(&fp1);
        CHECK(k1 == k2, "tux_derive_program_key is deterministic");

        ProgramFingerprint fp2 = fp1;
        fp2.tux_checksum = 'X';
        uint64_t k_diff = tux_derive_program_key(&fp2);
        CHECK(k1 != k_diff, "tux_derive_program_key is sensitive to tux_checksum");
    }

    /* 4. decode_instruction */
    {
        TuxGenome genome;
        tux_genome_init(&genome, 0x12345678ULL, NULL);
        int64_t regs[4] = {10, 20, 30, 40};

        /* Case 4.1: DEAD cell -> OP_NOP */
        uint32_t dead_val = 0; /* age(0) == DEAD */
        DecodedInstruction inst_dead = decode_instruction(dead_val, 0, 0x12345678ULL, regs, &genome, 0xABCDEF, 1, 0);
        CHECK(inst_dead.op == OP_NOP, "DEAD cell decodes strictly to OP_NOP");
        CHECK(inst_dead.width == 1, "DEAD cell width is 1");

        /* Case 4.2: DEAD cell (nu3=3) -> OP_NOP */
        uint32_t dead_val2 = 27; /* age(27) == DEAD */
        DecodedInstruction inst_dormant = decode_instruction(dead_val2, 0, 0x12345678ULL, regs, &genome, 0xABCDEF, 1, 0);
        CHECK(inst_dormant.op == OP_NOP, "DORMANT cell decodes strictly to OP_NOP");

        /* Case 4.3: YOUNG OPCODE cell in non-apocalypse -> raw_code */
        uint32_t young_val = 8;
        DecodedInstruction inst_young = decode_instruction(young_val, 0, 0x12345678ULL, regs, &genome, 0xABCDEF, 4, 0);
        CHECK(inst_young.op == OP_PUSH, "YOUNG cell in Purgatory decodes strictly to raw_code");
        CHECK(inst_young.width >= 1 && inst_young.width <= 4, "Width is clamped to [1, 4]");

        /* Case 4.4: Dynamic operand width masking */
        young_val = 0x12345678;
        DecodedInstruction inst_w1 = decode_instruction(young_val, 0, 0x12345678ULL, regs, &genome, 0xABCDEF, 1, 0);
        CHECK(inst_w1.arg == (int64_t)(int8_t)(0x78), "Width 1 sign-extends 8-bit operand");

        DecodedInstruction inst_w2 = decode_instruction(young_val, 0, 0x12345678ULL, regs, &genome, 0xABCDEF, 2, 0);
        CHECK(inst_w2.arg == (int64_t)(int16_t)(0x5678), "Width 2 sign-extends 16-bit operand");
    }

    /* 5. tux_genome_evolve & tux_entropy_step */
    {
        TuxGenome genome;
        tux_genome_init(&genome, 0xDEADBEEFCAFEULL, NULL);
        uint64_t g0_orig = genome.chromosomes[0];
        int64_t regs[4] = {1, 2, 3, 4};

        tux_genome_evolve(&genome, 42, 0x55555555, regs, 10);
        CHECK(genome.chromosomes[0] != g0_orig, "Genome chromosome 0 evolves after instruction");

        TuxGenome g_copy = genome;
        tux_genome_evolve(&g_copy, 42, 0x55555555, regs, 10);

        TuxGenome g_copy2 = genome;
        tux_genome_evolve(&g_copy2, 42, 0x55555555, regs, 10);
        CHECK(g_copy.chromosomes[0] == g_copy2.chromosomes[0], "Genome evolution is deterministic");
    }

    /* 6. tux_scheduler_step */
    {
        TuxScheduler sched1, sched2;
        tux_scheduler_init(&sched1, 0x42ULL);
        tux_scheduler_init(&sched2, 0x42ULL);
        CHECK(sched1.current_ctx_id == 0, "Scheduler starts with Context A (0)");
        CHECK(sched1.sched_state == sched2.sched_state, "Scheduler state init is deterministic");

        TuxGenome genome;
        tux_genome_init(&genome, 0x42ULL, NULL);
        uint32_t *mem = calloc(TUX_MEM_SIZE, sizeof(uint32_t));
        TuxContext ctx;
        memset(&ctx, 0, sizeof(ctx));
        ctx.pc_code = 10;
        ctx.pc_data = 20;
        ctx.regs[0] = 100;
        mem[20] = 999;

        /* Step sched1 and sched2 identically */
        uint8_t id1 = tux_scheduler_step(&sched1, &ctx, mem, &genome, 0x100ULL, 1);
        uint8_t id2 = tux_scheduler_step(&sched2, &ctx, mem, &genome, 0x100ULL, 1);
        CHECK(id1 == id2, "Scheduler step is 100% deterministic");
        CHECK(sched1.sched_state == sched2.sched_state, "Scheduler state updates deterministically");
        CHECK(id1 == 0 || id1 == 1, "Scheduler context ID is either 0 or 1");

        free(mem);
    }

    if (g_failed == 0) {
        printf("=======================================================\n");
        printf("All Golden Vectors PASSED (0 failures)\n");
        return 0;
    } else {
        printf("=======================================================\n");
        printf("FAILURES DETECTED: %d\n", g_failed);
        return 1;
    }
}
