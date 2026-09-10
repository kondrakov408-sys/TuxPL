#include "tuxpl.h"
#include <stdlib.h>
#include <string.h>

void tux_mem_init(uint32_t *mem, uint64_t program_key, uint64_t genome0) {
    for (size_t i = 0; i < TUX_MEM_SIZE; i++) {
        uint64_t v = tux_crazy64((uint64_t)i ^ program_key, genome0);
        uint32_t w = (uint32_t)(v % TUX_ZM_M);
        if (w % 17 == 0) w = (w + 1) % TUX_ZM_M;
        mem[i] = w;
    }
}

uint16_t mutation_encode(int64_t val, uint64_t program_key, uint64_t genome0) {
    uint64_t g_rot = (genome0 >> 17) | (genome0 << (64 - 17));
    uint64_t mixed = tux_crazy64((uint64_t)val ^ program_key, g_rot);
    return (uint16_t)(mixed & 0xFFFF);
}

void tux_register_coupling(int64_t regs[4]) {
    int64_t r0 = regs[0], r1 = regs[1], r2 = regs[2], r3 = regs[3];
    int64_t d1 = (int64_t)(tux_crazy64((uint64_t)r0, (uint64_t)r1) % 256);
    int64_t d2 = (int64_t)(tux_crazy64((uint64_t)r1, (uint64_t)r2) % 256);
    int64_t d3 = (int64_t)(tux_crazy64((uint64_t)r2, (uint64_t)r3) % 256);
    int64_t d0 = (int64_t)(tux_crazy64((uint64_t)r3, (uint64_t)r0) % 256);

    regs[0] ^= d0;
    regs[1] += d1;
    regs[2] ^= d2;
    regs[3] += d3;
}

void tux_history_push(TuxHistoryBuffer *hb, const TuxHistoryEntry *entry) {
    hb->entries[hb->head] = *entry;
    hb->head = (hb->head + 1) % TUX_HISTORY_DEPTH;
    if (hb->count < TUX_HISTORY_DEPTH) {
        hb->count++;
    }
}

int tux_history_undo(TuxHistoryBuffer *hb, TuxContext *ctx, uint32_t *mem) {
    if (hb->count == 0) {
        return 0; /* TR_NO_HISTORY */
    }
    size_t idx = (hb->head + TUX_HISTORY_DEPTH - 1) % TUX_HISTORY_DEPTH;
    const TuxHistoryEntry *e = &hb->entries[idx];

    /* Восстанавливаем регистры текущего контекста */
    (void)e->pc_data;
    for (int i = 0; i < 4; i++) {
        ctx->regs[i] = e->regs[i];
        ctx->reg_tags[i] = e->reg_tags[i];
    }

    /* Восстанавливаем модифицированное значение ячейки данных */
    if (e->has_mem_modified) {
        mem[e->mem_addr] = e->mem_val_before;
    }

    /* Восстанавливаем стек */
    if (e->has_stack_popped) {
        if (ctx->stack.n == ctx->stack.cap) {
            size_t cap = ctx->stack.cap ? ctx->stack.cap * 2 : 16;
            int64_t *nd = realloc(ctx->stack.d, cap * sizeof(int64_t));
            if (!nd) exit(1);
            ctx->stack.d = nd;
            int64_t *nt = realloc(ctx->stack_tags.d, cap * sizeof(int64_t));
            if (!nt) exit(1);
            ctx->stack_tags.d = nt;
            ctx->stack.cap = cap;
            ctx->stack_tags.cap = cap;
        }
        ctx->stack.d[ctx->stack.n++] = e->stack_popped_val;
        ctx->stack_tags.d[ctx->stack_tags.n++] = e->stack_popped_tag;
    }

    hb->head = idx;
    hb->count--;
    return 1;
}
