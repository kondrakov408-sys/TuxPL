#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const int CRAZY_TRIT[3][3] = {
    {1, 0, 0},
    {1, 0, 2},
    {2, 2, 1}
};

uint64_t tux_crazy64(uint64_t a, uint64_t b) {
    uint64_t lo_res = 0;
    uint64_t p3 = 1;
    for (int i = 0; i < 20; i++) {
        int tr_a = (int)((a / p3) % 3);
        int tr_b = (int)((b / p3) % 3);
        lo_res = (lo_res + (uint64_t)CRAZY_TRIT[tr_a][tr_b] * p3);
        p3 *= 3;
    }

    uint64_t hi_res = 0;
    p3 = 1;
    uint64_t a_hi = a >> 32;
    uint64_t b_hi = b >> 32;
    for (int i = 0; i < 20; i++) {
        int tr_a = (int)((a_hi / p3) % 3);
        int tr_b = (int)((b_hi / p3) % 3);
        hi_res = (hi_res + (uint64_t)CRAZY_TRIT[tr_a][tr_b] * p3);
        p3 *= 3;
    }

    uint64_t mixed = lo_res ^ (hi_res << 24) ^ (hi_res >> 8);
    mixed ^= (a << 1) ^ (b >> 1);
    return mixed;
}

uint64_t tux_source_hash(const char *raw_bytes, size_t len) {
    uint64_t h = 0xCBF29CE484222325ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)raw_bytes[i];
        h *= 0x100000001B3ULL;
    }
    return h;
}

uint64_t tux_derive_program_key(const ProgramFingerprint *fp) {
    uint64_t k = fp->source_hash ^ (fp->bit_len * 0x9E3779B97F4A7C15ULL);
    k ^= ((uint64_t)(uint8_t)fp->tux_checksum << 56);
    return tux_crazy64(k, fp->bit_len);
}

static uint64_t get_deterministic_time(void) {
    const char *mock_time = getenv("TUX_TIME");
    if (mock_time) {
        return (uint64_t)strtoull(mock_time, NULL, 0);
    }
    return (uint64_t)time(NULL);
}

void tux_entropy_init(TuxEntropy *ent, uint64_t program_key) {
    const char *mock_seed = getenv("TUX_ENTROPY_SEED");
    if (mock_seed) {
        ent->seed = (uint64_t)strtoull(mock_seed, NULL, 0);
    } else {
        uint64_t t = get_deterministic_time();
        ent->seed = program_key ^ t;
    }
    ent->step_counter = 0;
    ent->pool = tux_crazy64(ent->seed, 0x517CC1B727220A95ULL);
}

void tux_entropy_step(TuxEntropy *ent, uint64_t pc, uint64_t reg0, uint64_t genome0) {
    ent->pool = tux_crazy64(ent->pool ^ pc ^ ent->step_counter, reg0 + genome0);
    ent->step_counter++;
}

void tux_genome_init(TuxGenome *gen, uint64_t program_key, const uint64_t *tu_genome_seed) {
    if (tu_genome_seed) {
        for (int i = 0; i < 4; i++) {
            gen->chromosomes[i] = tu_genome_seed[i];
        }
    } else {
        const char *mock_gen = getenv("TUX_GENOME");
        if (mock_gen) {
            uint64_t seed = (uint64_t)strtoull(mock_gen, NULL, 0);
            gen->chromosomes[0] = seed;
            gen->chromosomes[1] = tux_crazy64(seed, 0x9E3779B97F4A7C15ULL);
            gen->chromosomes[2] = tux_crazy64(gen->chromosomes[1], 0x517CC1B727220A95ULL);
            gen->chromosomes[3] = tux_crazy64(gen->chromosomes[2], 0xDEADBEEFCAFEBABEULL);
        } else {
            gen->chromosomes[0] = program_key;
            gen->chromosomes[1] = tux_crazy64(program_key, 0xDEADBEEFCAFEBABEULL);
            gen->chromosomes[2] = tux_crazy64(gen->chromosomes[1], 0x9E3779B97F4A7C15ULL);
            gen->chromosomes[3] = gen->chromosomes[0] ^ gen->chromosomes[1] ^ gen->chromosomes[2];
        }
    }
    gen->generation = 0;
    gen->divergence = 0;
}

void tux_genome_evolve(TuxGenome *gen, int64_t result, uint64_t entropy_pool, const int64_t regs[4], uint64_t pc) {
    gen->chromosomes[0] = tux_crazy64(gen->chromosomes[0] ^ (uint64_t)result, entropy_pool);
    gen->chromosomes[1] = tux_crazy64(gen->chromosomes[1] ^ gen->chromosomes[0], (uint64_t)regs[1]);
    gen->chromosomes[2] = tux_crazy64(gen->chromosomes[2] ^ pc, gen->chromosomes[1]);
    uint64_t rot = (gen->chromosomes[3] >> 13) | (gen->chromosomes[3] << (64 - 13));
    gen->chromosomes[3] = rot ^ gen->chromosomes[0] ^ gen->chromosomes[2];
    gen->generation++;
    gen->divergence += (uint32_t)((uint64_t)result & 0xFF);
}
