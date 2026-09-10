#include "tuxpl.h"

DecodedInstruction decode_instruction(
    const TuxCell *cell,
    uint16_t pc,
    uint64_t program_key,
    const int64_t regs[4],
    const TuxGenome *genome,
    uint64_t entropy_pool,
    uint8_t prev_width,
    int is_apocalypse
) {
    DecodedInstruction res;
    res.op = OP_NOP;
    res.arg = 0;
    res.width = 1;

    /* 1. Инвариант DEAD ячейки: строго OP_NOP */
    if (cell->age == TUX_AGE_DEAD) {
        res.op = OP_NOP;
        res.arg = 0;
        res.width = 1;
        return res;
    }

    /* 2. Спящая ячейка: безопасный NOP */
    if (cell->flags & TUX_FLAG_DORMANT) {
        res.op = OP_NOP;
        res.arg = 0;
        res.width = 1;
        return res;
    }

    /* 3. Первичный вход: raw_code для OPCODE, val для остальных */
    uint16_t c_in = (cell->type_tag == TUX_TYPE_OPCODE)
                    ? cell->raw_code
                    : (uint16_t)((uint64_t)cell->val & 0xFFFF);

    /* 4. Синтез динамической ширины текущей инструкции W_t */
    uint64_t w_mix = tux_crazy64((uint64_t)cell->val, genome->chromosomes[1] ^ (uint64_t)prev_width);
    res.width = (uint8_t)(((w_mix >> 16) & 0x03) + 1);

    /* 5. Контекстное динамическое смешивание */
    uint64_t ctx_mix = tux_crazy64(
        program_key ^ genome->chromosomes[0] ^ entropy_pool,
        (uint64_t)pc ^ (uint64_t)regs[0]
    );
    int apply_shift = is_apocalypse && ((cell->flags & TUX_FLAG_MUTATED) || (cell->type_tag != TUX_TYPE_OPCODE) || (cell->gen > 0));
    uint64_t shift = apply_shift ? ((ctx_mix + (uint64_t)cell->gen) % TUX_NUM_OPCODES) : 0;

    /* 6. Возрастные домены декодирования опкода */
    uint64_t raw_op = 0;
    if (cell->age == TUX_AGE_YOUNG) {
        raw_op = (c_in + shift) % TUX_NUM_OPCODES;
    } else if (cell->age == TUX_AGE_ADULT) {
        raw_op = (c_in + shift + genome->chromosomes[1]) % TUX_NUM_OPCODES;
    } else { /* TUX_AGE_OLD */
        raw_op = tux_crazy64(c_in + shift, (uint64_t)cell->age ^ genome->chromosomes[2]) % TUX_NUM_OPCODES;
    }
    res.op = (Opcode)raw_op;

    /* 7. Маскирование операнда шириной prev_width */
    int64_t raw_arg = cell->val;
    if (prev_width == 1) {
        res.arg = (int64_t)(int8_t)(raw_arg & 0xFF);
    } else if (prev_width == 2) {
        res.arg = (int64_t)(int16_t)(raw_arg & 0xFFFF);
    } else if (prev_width == 3) {
        res.arg = (int64_t)(int32_t)(raw_arg & 0xFFFFFFFF);
    } else {
        res.arg = raw_arg;
    }

    return res;
}
