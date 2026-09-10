#include "tuxpl.h"

void tux_mutate_cell(
    TuxCell *cell,
    int64_t result,
    uint64_t program_key,
    uint64_t genome0,
    uint64_t entropy_pool,
    uint16_t pc
) {
    cell->exec_count++;

    /* Старение инструкции */
    if (cell->exec_count >= 3 && cell->age < TUX_AGE_DEAD) {
        cell->age++;
        if (cell->age == TUX_AGE_DEAD) {
            cell->flags |= TUX_FLAG_CORRUPTED;
            cell->type_tag = TUX_TYPE_I64;
            cell->raw_code = (uint16_t)(tux_crazy64((uint64_t)cell->val, (uint64_t)pc) & 0xFFFF);
            return;
        }
    }

    /* Мутация кода, если ячейка не защищена IMMUTABLE */
    if (!(cell->flags & TUX_FLAG_IMMUTABLE) && cell->age != TUX_AGE_DEAD) {
        uint64_t delta = tux_crazy64((uint64_t)result, entropy_pool);
        cell->val ^= (int64_t)(delta % 256);
        cell->raw_code = mutation_encode(cell->val, program_key, genome0);
        cell->flags |= TUX_FLAG_MUTATED;
    }
}

void tux_clone_cell(
    TuxCell *mem,
    uint16_t src_addr,
    uint16_t dst_addr,
    const TuxGenome *genome,
    uint64_t mutation_state,
    size_t *active_code_count
) {
    if (*active_code_count >= TUX_MAX_ACTIVE_CODE) {
        troll_die(TR_MEMRANGE);
    }

    TuxCell *src = &mem[src_addr];
    TuxCell *dst = &mem[dst_addr];

    /* Насыщение поколения и ошибка TR_AGE */
    if (src->gen == 255) {
        dst->gen = 255;
        dst->flags |= TUX_FLAG_CORRUPTED;
        troll_die(TR_AGE);
    }

    dst->val = src->val;
    dst->raw_code = src->raw_code;
    dst->type_tag = src->type_tag;
    dst->age = TUX_AGE_YOUNG;
    dst->gen = (uint8_t)(src->gen + 1);
    dst->flags = TUX_FLAG_CLONED;
    dst->exec_count = 0;
    dst->lineage = (uint32_t)(tux_crazy64(
        (uint64_t)src->lineage ^ (uint64_t)src_addr,
        (uint64_t)dst_addr ^ genome->chromosomes[2] ^ mutation_state
    ) & 0xFFFFFFFF);

    (*active_code_count)++;
}

void tux_check_dormant_resonance(TuxCell *mem, const TuxGenome *genome) {
    uint16_t res_byte = (uint16_t)(genome->chromosomes[3] & 0xFF);
    for (size_t addr = res_byte; addr < TUX_MEM_SIZE; addr += 256) {
        if (mem[addr].flags & TUX_FLAG_DORMANT) {
            mem[addr].flags &= ~TUX_FLAG_DORMANT;
        }
    }
}
