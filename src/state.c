#include "tuxpl.h"
#include <stdlib.h>
#include <string.h>

void tux_mem_init(TuxCell *mem, uint64_t program_key, uint64_t genome0) {
    for (size_t i = 0; i < TUX_MEM_SIZE; i++) {
        mem[i].val = (int64_t)tux_crazy64((uint64_t)i ^ program_key, genome0);
        mem[i].raw_code = mutation_encode(mem[i].val, program_key, genome0);
        mem[i].type_tag = TUX_TYPE_I64;
        mem[i].age = TUX_AGE_YOUNG;
        mem[i].gen = 0;
        mem[i].flags = 0;
        mem[i].lineage = (uint32_t)(tux_crazy64((uint64_t)i, program_key) & 0xFFFFFFFF);
        mem[i].exec_count = 0;
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

int tux_history_undo(TuxHistoryBuffer *hb, TuxContext *ctx, TuxCell *mem) {
    if (hb->count == 0) {
        return 0; /* TR_NO_HISTORY */
    }
    size_t idx = (hb->head + TUX_HISTORY_DEPTH - 1) % TUX_HISTORY_DEPTH;
    const TuxHistoryEntry *e = &hb->entries[idx];

    /* Восстанавливаем данные и регистры текущего контекста */
    ctx->pc_data = e->pc_data;
    for (int i = 0; i < 4; i++) {
        ctx->regs[i] = e->regs[i];
        ctx->reg_tags[i] = e->reg_tags[i];
    }

    /* Восстанавливаем модифицированное значение ячейки данных (только val и tag!) */
    if (e->has_mem_modified) {
        mem[e->mem_addr].val = e->mem_val_before;
        mem[e->mem_addr].type_tag = e->mem_tag_before;
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
