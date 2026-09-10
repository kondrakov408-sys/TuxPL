#include "tuxpl.h"

void tux_scheduler_init(TuxScheduler *sched, uint64_t program_key) {
    sched->current_ctx_id = 0;
    sched->sched_state = tux_crazy64(program_key, 0x100000001B3ULL);
}

uint8_t tux_scheduler_step(
    TuxScheduler *sched,
    const TuxContext *active_ctx,
    const TuxCell *mem,
    const TuxGenome *genome,
    uint64_t entropy_pool,
    uint64_t step_counter
) {
    uint64_t v_pc = (uint64_t)active_ctx->pc_code;
    uint64_t v_reg = (uint64_t)active_ctx->regs[0];
    uint64_t v_mem = (uint64_t)mem[active_ctx->pc_data].val;

    uint64_t metric = tux_crazy64((v_pc + v_reg + v_mem), sched->sched_state);
    uint8_t next_id = (uint8_t)(metric % 2);

    sched->sched_state = tux_crazy64(
        sched->sched_state ^ step_counter,
        genome->chromosomes[0] ^ entropy_pool
    );
    sched->current_ctx_id = next_id;
    return next_id;
}
