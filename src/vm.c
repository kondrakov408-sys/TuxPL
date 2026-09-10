#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void oom(void) {
    fprintf(stderr, "TuxPL: у Тукса кончилась память. Буквально.\n");
    exit(1);
}

static void vec_reserve(Vec *v, size_t need) {
    if (need <= v->cap) return;
    size_t cap = v->cap ? v->cap : 16;
    while (cap < need) cap *= 2;
    int64_t *d = realloc(v->d, cap * sizeof(int64_t));
    if (!d) oom();
    v->d = d;
    v->cap = cap;
}

static void vec_push(Vec *v, int64_t x) {
    vec_reserve(v, v->n + 1);
    v->d[v->n++] = x;
}

static void vec_fit(Vec *v, size_t size) {
    if (size <= v->n) return;
    vec_reserve(v, size);
    memset(v->d + v->n, 0, (size - v->n) * sizeof(int64_t));
    v->n = size;
}

typedef struct {
    Vec *d;
    size_t n, cap;
} Lists;

static Vec *list_at(Lists *L, int64_t idx) {
    if (idx < 0 || idx > (1LL << 20)) troll_die(TR_LISTRANGE);
    size_t i = (size_t)idx;
    if (i >= L->cap) {
        size_t cap = L->cap ? L->cap : 8;
        while (cap <= i) cap *= 2;
        Vec *d = realloc(L->d, cap * sizeof(Vec));
        if (!d) oom();
        memset(d + L->cap, 0, (cap - L->cap) * sizeof(Vec));
        L->d = d;
        L->cap = cap;
    }
    if (i >= L->n) L->n = i + 1;
    return &L->d[i];
}

static int64_t spop(Vec *s) {
    if (s->n == 0) troll_die(TR_STACK);
    return s->d[--s->n];
}

static int64_t mem_addr(int64_t a) {
    if (a < 0 || a > (1LL << 30)) troll_die(TR_MEMRANGE);
    return a;
}

static int64_t spop_tag(Vec *s, Vec *tags, int *out_tag) {
    if (s->n == 0) troll_die(TR_STACK);
    if (tags && tags->n > 0) {
        if (out_tag) *out_tag = (int)tags->d[--tags->n];
        else tags->n--;
    } else if (out_tag) {
        *out_tag = 0;
    }
    return s->d[--s->n];
}

static void run_classic(const Program *prog) {
    Vec stack = {0, 0, 0};
    Vec vars = {0, 0, 0};
    Vec mem = {0, 0, 0};
    Lists lists = {0, 0, 0};
    size_t pc = 0;

    while (pc < prog->len) {
        const Cmd *c = &prog->cmds[pc];
        switch (c->op) {
        case OP_PUSH: vec_push(&stack, c->arg); break;
        case OP_LOAD:
            vec_fit(&vars, (size_t)c->arg + 1);
            vec_push(&stack, vars.d[c->arg]);
            break;
        case OP_STORE:
            vec_fit(&vars, (size_t)c->arg + 1);
            vars.d[c->arg] = spop(&stack);
            break;
        case OP_ADD: case OP_SUB: case OP_MUL: {
            int64_t b = spop(&stack), a = spop(&stack);
            uint64_t r = (c->op == OP_ADD) ? (uint64_t)a + (uint64_t)b
                       : (c->op == OP_SUB) ? (uint64_t)a - (uint64_t)b
                                           : (uint64_t)a * (uint64_t)b;
            vec_push(&stack, (int64_t)r);
            break;
        }
        case OP_DIV: {
            int64_t b = spop(&stack), a = spop(&stack);
            if (b == 0) troll_die(TR_DIVZERO);
            if (a == INT64_MIN && b == -1) troll_die(TR_OVERFLOW);
            vec_push(&stack, a / b);
            break;
        }
        case OP_DUP:
            if (stack.n == 0) troll_die(TR_STACK);
            vec_push(&stack, stack.d[stack.n - 1]);
            break;
        case OP_SWAP: {
            int64_t b = spop(&stack), a = spop(&stack);
            vec_push(&stack, b); vec_push(&stack, a);
            break;
        }
        case OP_POP: spop(&stack); break;
        case OP_PRINTCHAR: {
            int64_t v = spop(&stack);
            fputc((int)(uint8_t)(v & 0xFF), stdout);
            break;
        }
        case OP_PRINTNUM: {
            int64_t v = spop(&stack);
            printf("%lld", (long long)v);
            break;
        }
        case OP_INPUTNUM: {
            vec_fit(&vars, (size_t)c->arg + 1);
            long long x;
            if (scanf(" %lld", &x) != 1) troll_die(TR_BADINPUT);
            vars.d[c->arg] = (int64_t)x;
            break;
        }
        case OP_LOADIND: {
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            vec_push(&stack, mem.d[a]);
            break;
        }
        case OP_STOREIND: {
            int64_t v = spop(&stack);
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            mem.d[a] = v;
            break;
        }
        case OP_CMP: {
            int64_t b = spop(&stack), a = spop(&stack);
            vec_push(&stack, (a > b) - (a < b));
            break;
        }
        case OP_JMP: pc = (size_t)c->arg; continue;
        case OP_JZ: {
            int64_t v = spop(&stack);
            if (v == 0) { pc = (size_t)c->arg; continue; }
            break;
        }
        case OP_JNZ: {
            int64_t v = spop(&stack);
            if (v != 0) { pc = (size_t)c->arg; continue; }
            break;
        }
        case OP_LISTNEW: list_at(&lists, c->arg)->n = 0; break;
        case OP_LISTPUSH: {
            int64_t v = spop(&stack);
            vec_push(list_at(&lists, c->arg), v);
            break;
        }
        case OP_LISTGET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            vec_push(&stack, L->d[i]);
            break;
        }
        case OP_LISTSET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t v = spop(&stack), i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            break;
        }
        case OP_LISTLEN:
            vec_push(&stack, (int64_t)list_at(&lists, c->arg)->n);
            break;
        default: break;
        }
        pc++;
    }

    free(stack.d);
    free(vars.d);
    free(mem.d);
    for (size_t i = 0; i < lists.n; i++) free(lists.d[i].d);
    free(lists.d);
}

static void run_cursed(const Program *prog) {
    Vec stack = {0, 0, 0};
    Vec stack_tags = {0, 0, 0};
    Vec vars = {0, 0, 0};
    Vec vars_tags = {0, 0, 0};
    Vec mem = {0, 0, 0};
    Lists lists = {0, 0, 0};
    size_t pc = 0;

    int is_purg = prog->is_purgatory;
    int fish_grams = 100;
    int consecutive_pushes = 0;
    int64_t regs[4] = {0, 0, 0, 0};
    int reg_tags[4] = {0, 0, 0, 0};
    uint8_t var_owned[64];
    memset(var_owned, 0, sizeof(var_owned));
    int dir = 0;

    while (pc < prog->len) {
        const Cmd *c = &prog->cmds[pc];

        if (is_purg) {
            fish_grams--;
            if (fish_grams < 0) troll_die(TR_STARVATION);
        }

        switch (c->op) {
        case OP_PUSH:
            vec_push(&stack, c->arg);
            vec_push(&stack_tags, c->type_tag);
            if (is_purg) {
                consecutive_pushes++;
                if (consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            }
            break;
        case OP_LOAD:
            if (is_purg && c->arg >= 0 && c->arg < 64) {
                if (!var_owned[c->arg]) troll_die(TR_USE_AFTER_MOVE);
                var_owned[c->arg] = 0;
            }
            vec_fit(&vars, (size_t)c->arg + 1);
            vec_fit(&vars_tags, (size_t)c->arg + 1);
            vec_push(&stack, vars.d[c->arg]);
            vec_push(&stack_tags, vars_tags.d[c->arg]);
            if (is_purg) {
                consecutive_pushes++;
                if (consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            }
            break;
        case OP_STORE: {
            int tag = 0;
            int64_t v = spop_tag(&stack, &stack_tags, &tag);
            if (is_purg) {
                consecutive_pushes = 0;
                if (c->arg >= 0 && c->arg < 64) {
                    var_owned[c->arg] = 1;
                }
            }
            vec_fit(&vars, (size_t)c->arg + 1);
            vec_fit(&vars_tags, (size_t)c->arg + 1);
            vars.d[c->arg] = v;
            vars_tags.d[c->arg] = tag;
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&stack, &stack_tags, &tb);
            int64_t a = spop_tag(&stack, &stack_tags, &ta);
            if (is_purg) {
                consecutive_pushes = 0;
                if (ta != tb) troll_die(TR_TYPE_MISMATCH);
            }
            uint64_t r = (c->op == OP_ADD) ? (uint64_t)a + (uint64_t)b
                       : (c->op == OP_SUB) ? (uint64_t)a - (uint64_t)b
                                           : (uint64_t)a * (uint64_t)b;
            vec_push(&stack, (int64_t)r);
            vec_push(&stack_tags, ta);
            if (is_purg) {
                regs[0] = (int64_t)r;
                reg_tags[0] = ta;
            }
            break;
        }
        case OP_DIV: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&stack, &stack_tags, &tb);
            int64_t a = spop_tag(&stack, &stack_tags, &ta);
            if (is_purg) {
                consecutive_pushes = 0;
                if (ta != tb) troll_die(TR_TYPE_MISMATCH);
            }
            if (b == 0) troll_die(TR_DIVZERO);
            if (a == INT64_MIN && b == -1) troll_die(TR_OVERFLOW);
            vec_push(&stack, a / b);
            vec_push(&stack_tags, ta);
            if (is_purg) {
                regs[0] = a / b;
                reg_tags[0] = ta;
            }
            break;
        }
        case OP_DUP: {
            if (stack.n == 0) troll_die(TR_STACK);
            int tag = stack_tags.n > 0 ? (int)stack_tags.d[stack_tags.n - 1] : 0;
            vec_push(&stack, stack.d[stack.n - 1]);
            vec_push(&stack_tags, tag);
            if (is_purg) {
                consecutive_pushes++;
                if (consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            }
            break;
        }
        case OP_SWAP: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&stack, &stack_tags, &tb);
            int64_t a = spop_tag(&stack, &stack_tags, &ta);
            vec_push(&stack, b);
            vec_push(&stack_tags, tb);
            vec_push(&stack, a);
            vec_push(&stack_tags, ta);
            if (is_purg) consecutive_pushes = 0;
            break;
        }
        case OP_POP:
            spop_tag(&stack, &stack_tags, NULL);
            if (is_purg) consecutive_pushes = 0;
            break;
        case OP_PRINTCHAR: {
            int64_t v = spop_tag(&stack, &stack_tags, NULL);
            fputc((int)(uint8_t)(v & 0xFF), stdout);
            if (is_purg) consecutive_pushes = 0;
            break;
        }
        case OP_PRINTNUM: {
            int64_t v = spop_tag(&stack, &stack_tags, NULL);
            printf("%lld", (long long)v);
            if (is_purg) consecutive_pushes = 0;
            break;
        }
        case OP_REGGET: {
            size_t ridx = (size_t)(((c->arg % 4) + 4) % 4);
            vec_push(&stack, regs[ridx]);
            vec_push(&stack_tags, reg_tags[ridx]);
            if (is_purg) {
                consecutive_pushes++;
                if (consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            }
            break;
        }
        case OP_REGSET: {
            size_t ridx = (size_t)(((c->arg % 4) + 4) % 4);
            regs[ridx] = spop_tag(&stack, &stack_tags, &reg_tags[ridx]);
            if (is_purg) consecutive_pushes = 0;
            break;
        }
        case OP_FISH: {
            int add = (c->arg > 0 && c->arg <= 10000) ? (int)c->arg : 50;
            fish_grams += add;
            if (fish_grams > 100000) fish_grams = 100000;
            break;
        }
        case OP_CRAZY: {
            static const int crazy[3][3] = {
                {1, 0, 0},
                {1, 0, 2},
                {2, 2, 1}
            };
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&stack, &stack_tags, &tb);
            int64_t a = spop_tag(&stack, &stack_tags, &ta);
            int64_t res = 0, p3 = 1;
            for (int t = 0; t < 10; t++) {
                int tr_a = (int)((a / p3) % 3); if (tr_a < 0) tr_a += 3;
                int tr_b = (int)((b / p3) % 3); if (tr_b < 0) tr_b += 3;
                res += (int64_t)crazy[tr_a][tr_b] * p3;
                p3 *= 3;
            }
            vec_push(&stack, res);
            vec_push(&stack_tags, ta);
            if (is_purg) consecutive_pushes = 0;
            break;
        }
        case OP_CAST:
            if (stack_tags.n > 0) {
                stack_tags.d[stack_tags.n - 1] = c->arg;
            }
            break;
        case OP_DIR:
            dir = (int)(((c->arg % 4) + 4) % 4);
            break;
        case OP_INPUTNUM: {
            vec_fit(&vars, (size_t)c->arg + 1);
            long long x;
            if (scanf(" %lld", &x) != 1) troll_die(TR_BADINPUT);
            vars.d[c->arg] = (int64_t)x;
            break;
        }
        case OP_LOADIND: {
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            vec_push(&stack, mem.d[a]);
            break;
        }
        case OP_STOREIND: {
            int64_t v = spop(&stack);
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            mem.d[a] = v;
            break;
        }
        case OP_CMP: {
            int64_t b = spop(&stack), a = spop(&stack);
            vec_push(&stack, (a > b) - (a < b));
            break;
        }
        case OP_JMP: pc = (size_t)c->arg; continue;
        case OP_JZ: {
            int64_t v = spop(&stack);
            if (v == 0) { pc = (size_t)c->arg; continue; }
            break;
        }
        case OP_JNZ: {
            int64_t v = spop(&stack);
            if (v != 0) { pc = (size_t)c->arg; continue; }
            break;
        }
        case OP_LISTNEW: list_at(&lists, c->arg)->n = 0; break;
        case OP_LISTPUSH: {
            int64_t v = spop(&stack);
            vec_push(list_at(&lists, c->arg), v);
            break;
        }
        case OP_LISTGET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            vec_push(&stack, L->d[i]);
            break;
        }
        case OP_LISTSET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t v = spop(&stack), i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            break;
        }
        case OP_LISTLEN:
            vec_push(&stack, (int64_t)list_at(&lists, c->arg)->n);
            break;
        default: break;
        }

        if (is_purg && dir != 0) {
            if (dir == 1) pc += 5;
            else if (dir == 2) pc--;
            else if (dir == 3) pc -= 5;
            else pc++;
        } else {
            pc++;
        }
    }

    free(stack.d);
    free(stack_tags.d);
    free(vars.d);
    free(vars_tags.d);
    free(mem.d);
    for (size_t i = 0; i < lists.n; i++) free(lists.d[i].d);
    free(lists.d);
}

static void run_unified_vm(const Program *prog, const TuxVMConfig *config) {
    TuxCell *memory = calloc(TUX_MEM_SIZE, sizeof(TuxCell));
    if (!memory) oom();

    uint64_t program_key = tux_derive_program_key(&prog->fp);

    /* Загрузка companion, если требуется */
    uint64_t tu_genome[4] = {0, 0, 0, 0};
    uint64_t tu_regs[4] = {0, 0, 0, 0};
    int has_companion_data = 0;

    const char *cpath = config->companion_path ? config->companion_path : (prog->has_companion ? prog->companion_name : NULL);
    if (cpath) {
        load_companion_file(cpath, prog->fp.source_hash, tu_genome, tu_regs);
        has_companion_data = 1;
    }

    TuxGenome genome;
    tux_genome_init(&genome, program_key, has_companion_data ? tu_genome : NULL);

    TuxEntropy entropy;
    tux_entropy_init(&entropy, program_key);

    /* 1. Детерминированная инициализация всех 65536 ячеек */
    tux_mem_init(memory, program_key, genome.chromosomes[0]);

    /* 2. Загрузка кода программы в начало Unified Memory */
    for (size_t i = 0; i < prog->len && i < TUX_MEM_SIZE; i++) {
        memory[i].val = prog->cmds[i].arg;
        memory[i].raw_code = (uint16_t)prog->cmds[i].op;
        memory[i].type_tag = TUX_TYPE_OPCODE;
        memory[i].age = TUX_AGE_YOUNG;
        memory[i].gen = 0;
        memory[i].flags = TUX_FLAG_EXECUTABLE;
        memory[i].lineage = (uint32_t)i;
        memory[i].exec_count = 0;
    }

    /* 3. Инициализация контекстов */
    TuxContext ctxA;
    memset(&ctxA, 0, sizeof(ctxA));
    ctxA.pc_code = 0;
    ctxA.pc_data = 1024;
    ctxA.active = 1;

    TuxContext ctxB;
    memset(&ctxB, 0, sizeof(ctxB));
    ctxB.pc_code = (uint16_t)((program_key ^ genome.chromosomes[1]) % TUX_MEM_SIZE);
    if (ctxB.pc_code < prog->len && prog->len < TUX_MEM_SIZE) {
        ctxB.pc_code = (uint16_t)(prog->len + (ctxB.pc_code % (TUX_MEM_SIZE - prog->len)));
    }
    ctxB.pc_data = (uint16_t)((ctxB.pc_code + 512 + (genome.chromosomes[2] % 1024)) % TUX_MEM_SIZE);
    if (ctxB.pc_data == ctxB.pc_code) ctxB.pc_data = (ctxB.pc_data + 1) % TUX_MEM_SIZE;
    if (has_companion_data) {
        for (int r = 0; r < 4; r++) ctxB.regs[r] = (int64_t)tu_regs[r];
    } else {
        for (int r = 0; r < 4; r++) ctxB.regs[r] = (int64_t)genome.chromosomes[r];
    }
    ctxB.active = (config->mode == TUX_MODE_APOCALYPSE);

    TuxScheduler sched;
    tux_scheduler_init(&sched, program_key);

    TuxTimeDebt time_debt = {0, TUX_TIME_DEBT_LIMIT, 0};
    TuxHistoryBuffer hist = { .head = 0, .count = 0 };

    uint64_t step_counter = 0;
    size_t active_code_count = prog->len;
    uint8_t current_width = 1;
    int fish_grams = 500;
    Vec vars = {0, 0, 0};
    Vec vars_tags = {0, 0, 0};
    Lists lists = {0, 0, 0};
    uint8_t var_owned[64];
    memset(var_owned, 0, sizeof(var_owned));

    const uint64_t max_steps = 1000000;

    while (step_counter < max_steps) {
        TuxContext *act = (sched.current_ctx_id == 1 && ctxB.active) ? &ctxB : &ctxA;
        const char *ctx_name = (act == &ctxB) ? "TUX_B" : "TUX_A";

        /* ФАЗА 1: FETCH */
        uint16_t pc = act->pc_code;
        const TuxCell *fetched = &memory[pc];

        /* ФАЗА 2: DECODE */
        DecodedInstruction inst = decode_instruction(
            fetched, pc, program_key, act->regs, &genome, entropy.pool, current_width,
            (config->mode == TUX_MODE_APOCALYPSE)
        );
        current_width = inst.width;

        if (config->is_trace || config->is_trace_state) {
            fprintf(stderr, "[STEP %llu][%s] PC: %u | OP: %d (raw: 0x%04X, age: %d) | Debt: %llu",
                    (unsigned long long)step_counter, ctx_name, pc, inst.op, fetched->raw_code, fetched->age, (unsigned long long)time_debt.accumulated_debt);
            if (config->is_trace_state) {
                fprintf(stderr, " | G0: 0x%016llX | E: 0x%016llX", (unsigned long long)genome.chromosomes[0], (unsigned long long)entropy.pool);
            }
            fprintf(stderr, "\n");
        }

        /* ФАЗА 3: EXECUTE */
        time_debt.accumulated_debt += (inst.op == OP_CLONE ? 5 : (inst.op == OP_CRAZY || inst.op == OP_CAST ? 1 + (entropy.pool % 3) : 1));
        if (time_debt.accumulated_debt > time_debt.debt_limit) {
            memory[pc].flags |= TUX_FLAG_CORRUPTED;
            tux_genome_evolve(&genome, -1, entropy.pool, act->regs, pc);
        }

        fish_grams--;
        if (fish_grams < 0) troll_die(TR_STARVATION);

        /* Снимок для UNDO / --REVERSIBLE */
        TuxHistoryEntry h_entry;
        memset(&h_entry, 0, sizeof(h_entry));
        h_entry.ctx_id = sched.current_ctx_id;
        h_entry.pc_code = act->pc_code;
        h_entry.pc_data = act->pc_data;
        memcpy(h_entry.regs, act->regs, sizeof(act->regs));
        memcpy(h_entry.reg_tags, act->reg_tags, sizeof(act->reg_tags));

        uint16_t pending_pc = pc;
        int is_branch_taken = 0;
        int64_t last_exec_result = 0;

        /* Защита конкурирующего контекста TUX_B от краша в неинициализированной памяти */
        if (act == &ctxB) {
            int needed_stack = 0;
            switch (inst.op) {
            case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:
            case OP_SWAP: case OP_STOREIND: case OP_CMP:
            case OP_CRAZY: case OP_CLONE: case OP_LISTSET:
                needed_stack = 2;
                break;
            case OP_STORE: case OP_LOADIND: case OP_POP:
            case OP_PRINTCHAR: case OP_PRINTNUM: case OP_REGSET:
            case OP_SET_PC: case OP_ADD_PC: case OP_XOR_PC:
            case OP_DECAY: case OP_WAKE: case OP_DUP:
            case OP_JZ: case OP_JNZ: case OP_LISTPUSH: case OP_LISTGET:
                needed_stack = 1;
                break;
            default:
                needed_stack = 0;
                break;
            }
            if (act->stack.n < (size_t)needed_stack) {
                inst.op = OP_NOP;
            }
            if (inst.op == OP_DIV && act->stack.n >= 2 && act->stack.d[act->stack.n - 1] == 0) {
                inst.op = OP_NOP;
            }
            if (inst.op >= OP_LISTNEW && inst.op <= OP_LISTLEN) {
                inst.op = OP_NOP;
            }
            if (inst.op == OP_INPUTNUM || inst.op == OP_PRINTCHAR || inst.op == OP_PRINTNUM) {
                inst.op = OP_NOP;
            }
            if (inst.op == OP_LOAD || inst.op == OP_STORE) {
                if (inst.arg < 0 || inst.arg >= 64 || (inst.op == OP_LOAD && !var_owned[inst.arg])) {
                    inst.op = OP_NOP;
                }
            }
            if (inst.op == OP_LOADIND && act->stack.n >= 1) {
                int64_t addr = act->stack.d[act->stack.n - 1];
                if (addr < 0 || addr >= TUX_MEM_SIZE) inst.op = OP_NOP;
            }
            if (inst.op == OP_STOREIND && act->stack.n >= 2) {
                int64_t addr = act->stack.d[act->stack.n - 2];
                if (addr < (int64_t)prog->len || addr >= TUX_MEM_SIZE) inst.op = OP_NOP;
            }
            if (inst.op == OP_DECAY && act->stack.n >= 1) {
                int64_t addr = act->stack.d[act->stack.n - 1];
                if (addr < (int64_t)prog->len || addr >= TUX_MEM_SIZE) inst.op = OP_NOP;
            }
            if (inst.op == OP_CLONE && act->stack.n >= 2) {
                int64_t dst = act->stack.d[act->stack.n - 1];
                if (dst < (int64_t)prog->len || dst >= TUX_MEM_SIZE) inst.op = OP_NOP;
            }
            if (inst.op == OP_UNDO && (!config->is_reversible || hist.count == 0)) {
                inst.op = OP_NOP;
            }
            if (inst.op == OP_PUSH && act->consecutive_pushes >= 7) {
                inst.op = OP_NOP;
            }
            if (inst.op == OP_PAY_TIME && fish_grams < 10) {
                inst.op = OP_NOP;
            }
        }

        switch (inst.op) {
        case OP_PUSH:
            vec_push(&act->stack, inst.arg);
            vec_push(&act->stack_tags, fetched->type_tag);
            act->consecutive_pushes++;
            if (act == &ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        case OP_LOAD:
            if (act == &ctxA && inst.arg >= 0 && inst.arg < 64) {
                if (!var_owned[inst.arg]) troll_die(TR_USE_AFTER_MOVE);
                var_owned[inst.arg] = 0;
            }
            vec_fit(&vars, (size_t)inst.arg + 1);
            vec_fit(&vars_tags, (size_t)inst.arg + 1);
            vec_push(&act->stack, vars.d[inst.arg]);
            vec_push(&act->stack_tags, vars_tags.d[inst.arg]);
            act->consecutive_pushes++;
            if (act == &ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        case OP_STORE: {
            int tag = 0;
            int64_t v = spop_tag(&act->stack, &act->stack_tags, &tag);
            act->consecutive_pushes = 0;
            if (inst.arg >= 0 && inst.arg < 64) var_owned[inst.arg] = 1;
            vec_fit(&vars, (size_t)inst.arg + 1);
            vec_fit(&vars_tags, (size_t)inst.arg + 1);
            vars.d[inst.arg] = v;
            vars_tags.d[inst.arg] = tag;
            last_exec_result = v;
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            act->consecutive_pushes = 0;
            if (act == &ctxA && ta != tb) troll_die(TR_TYPE_MISMATCH);
            uint64_t r = (inst.op == OP_ADD) ? (uint64_t)a + (uint64_t)b
                       : (inst.op == OP_SUB) ? (uint64_t)a - (uint64_t)b
                                             : (uint64_t)a * (uint64_t)b;
            vec_push(&act->stack, (int64_t)r);
            vec_push(&act->stack_tags, ta);
            act->regs[0] = (int64_t)r;
            act->reg_tags[0] = ta;
            last_exec_result = (int64_t)r;
            break;
        }
        case OP_DIV: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            act->consecutive_pushes = 0;
            if (act == &ctxA && ta != tb) troll_die(TR_TYPE_MISMATCH);
            if (act == &ctxA && b == 0) troll_die(TR_DIVZERO);
            if (act == &ctxA && a == INT64_MIN && b == -1) troll_die(TR_OVERFLOW);
            int64_t div_res = (b != 0) ? (a / b) : 0;
            vec_push(&act->stack, div_res);
            vec_push(&act->stack_tags, ta);
            act->regs[0] = div_res;
            act->reg_tags[0] = ta;
            last_exec_result = div_res;
            break;
        }
        case OP_DUP: {
            if (act->stack.n == 0) {
                if (act == &ctxA) troll_die(TR_STACK);
                break;
            }
            int tag = act->stack_tags.n > 0 ? (int)act->stack_tags.d[act->stack_tags.n - 1] : 0;
            vec_push(&act->stack, act->stack.d[act->stack.n - 1]);
            vec_push(&act->stack_tags, tag);
            act->consecutive_pushes++;
            if (act == &ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        }
        case OP_SWAP: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            vec_push(&act->stack, b);
            vec_push(&act->stack_tags, tb);
            vec_push(&act->stack, a);
            vec_push(&act->stack_tags, ta);
            act->consecutive_pushes = 0;
            break;
        }
        case OP_POP:
            spop_tag(&act->stack, &act->stack_tags, NULL);
            act->consecutive_pushes = 0;
            break;
        case OP_PRINTCHAR: {
            int64_t v = spop_tag(&act->stack, &act->stack_tags, NULL);
            fputc((int)(uint8_t)(v & 0xFF), stdout);
            act->consecutive_pushes = 0;
            last_exec_result = v;
            break;
        }
        case OP_PRINTNUM: {
            int64_t v = spop_tag(&act->stack, &act->stack_tags, NULL);
            printf("%lld", (long long)v);
            act->consecutive_pushes = 0;
            last_exec_result = v;
            break;
        }
        case OP_REGGET: {
            size_t ridx = (size_t)(((inst.arg % 4) + 4) % 4);
            vec_push(&act->stack, act->regs[ridx]);
            vec_push(&act->stack_tags, act->reg_tags[ridx]);
            act->consecutive_pushes++;
            if (act == &ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        }
        case OP_REGSET: {
            size_t ridx = (size_t)(((inst.arg % 4) + 4) % 4);
            int tag_tmp = 0;
            act->regs[ridx] = spop_tag(&act->stack, &act->stack_tags, &tag_tmp);
            act->reg_tags[ridx] = (uint8_t)tag_tmp;
            act->consecutive_pushes = 0;
            break;
        }
        case OP_FISH: {
            int add = (inst.arg > 0 && inst.arg <= 10000) ? (int)inst.arg : 50;
            fish_grams += add;
            if (fish_grams > 100000) fish_grams = 100000;
            break;
        }
        case OP_CRAZY: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            uint64_t crz = tux_crazy64((uint64_t)a, (uint64_t)b);
            vec_push(&act->stack, (int64_t)crz);
            vec_push(&act->stack_tags, ta);
            act->consecutive_pushes = 0;
            last_exec_result = (int64_t)crz;
            break;
        }
        case OP_CAST:
            if (act->stack_tags.n > 0) {
                act->stack_tags.d[act->stack_tags.n - 1] = inst.arg;
            }
            break;
        case OP_DIR:
            act->dir = (uint8_t)(((inst.arg % 4) + 4) % 4);
            break;
        case OP_INPUTNUM: {
            vec_fit(&vars, (size_t)inst.arg + 1);
            long long x;
            if (scanf(" %lld", &x) != 1) troll_die(TR_BADINPUT);
            vars.d[inst.arg] = (int64_t)x;
            break;
        }
        case OP_LOADIND: {
            int64_t a = spop(&act->stack);
            if (act == &ctxA && (a < 0 || a >= TUX_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a < 0 || a >= TUX_MEM_SIZE) a = (a % TUX_MEM_SIZE + TUX_MEM_SIZE) % TUX_MEM_SIZE;
            vec_push(&act->stack, memory[a].val);
            vec_push(&act->stack_tags, memory[a].type_tag);
            last_exec_result = memory[a].val;
            break;
        }
        case OP_STOREIND: {
            int64_t v = spop(&act->stack);
            int64_t a = spop(&act->stack);
            if (act == &ctxA && (a < 0 || a >= TUX_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a < 0 || a >= TUX_MEM_SIZE) a = (a % TUX_MEM_SIZE + TUX_MEM_SIZE) % TUX_MEM_SIZE;
            h_entry.has_mem_modified = 1;
            h_entry.mem_addr = (uint16_t)a;
            h_entry.mem_val_before = memory[a].val;
            h_entry.mem_tag_before = memory[a].type_tag;

            memory[a].val = v;
            memory[a].raw_code = mutation_encode(v, program_key, genome.chromosomes[0]);
            memory[a].flags |= TUX_FLAG_MUTATED;
            last_exec_result = v;
            break;
        }
        case OP_CMP: {
            int64_t b = spop(&act->stack), a = spop(&act->stack);
            vec_push(&act->stack, (a > b) - (a < b));
            vec_push(&act->stack_tags, TUX_TYPE_I8);
            break;
        }
        case OP_JMP:
            pending_pc = (uint16_t)(inst.arg % TUX_MEM_SIZE);
            is_branch_taken = 1;
            break;
        case OP_JZ: {
            int64_t v = spop(&act->stack);
            if (v == 0) {
                pending_pc = (uint16_t)(inst.arg % TUX_MEM_SIZE);
                is_branch_taken = 1;
            }
            break;
        }
        case OP_JNZ: {
            int64_t v = spop(&act->stack);
            if (v != 0) {
                pending_pc = (uint16_t)(inst.arg % TUX_MEM_SIZE);
                is_branch_taken = 1;
            }
            break;
        }
        case OP_LISTNEW: list_at(&lists, inst.arg)->n = 0; break;
        case OP_LISTPUSH: {
            int64_t v = spop(&act->stack);
            vec_push(list_at(&lists, inst.arg), v);
            break;
        }
        case OP_LISTGET: {
            Vec *L = list_at(&lists, inst.arg);
            int64_t i = spop(&act->stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            vec_push(&act->stack, L->d[i]);
            break;
        }
        case OP_LISTSET: {
            Vec *L = list_at(&lists, inst.arg);
            int64_t v = spop(&act->stack), i = spop(&act->stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            break;
        }
        case OP_LISTLEN:
            vec_push(&act->stack, (int64_t)list_at(&lists, inst.arg)->n);
            break;

        /* TuxPL 2.0.0 расширения */
        case OP_PUSH_PC:
            vec_push(&act->stack, (int64_t)act->pc_code);
            vec_push(&act->stack_tags, TUX_TYPE_ADDR);
            break;
        case OP_SET_PC: {
            int64_t a = spop(&act->stack);
            pending_pc = (uint16_t)(a % TUX_MEM_SIZE);
            is_branch_taken = 1;
            break;
        }
        case OP_SWAP_PC: {
            uint16_t tmp = act->pc_code;
            act->pc_code = act->pc_data;
            act->pc_data = tmp;
            pending_pc = act->pc_code;
            is_branch_taken = 1;
            break;
        }
        case OP_ADD_PC: {
            int64_t off = spop(&act->stack);
            int64_t n_pc = (int64_t)act->pc_code + off;
            while (n_pc < 0) n_pc += TUX_MEM_SIZE;
            pending_pc = (uint16_t)(n_pc % TUX_MEM_SIZE);
            is_branch_taken = 1;
            break;
        }
        case OP_XOR_PC: {
            int64_t mask = spop(&act->stack);
            pending_pc = (uint16_t)((act->pc_code ^ (uint16_t)mask) % TUX_MEM_SIZE);
            is_branch_taken = 1;
            break;
        }
        case OP_CLONE: {
            int64_t dst = spop(&act->stack);
            int64_t src = spop(&act->stack);
            tux_clone_cell(memory, (uint16_t)(src % TUX_MEM_SIZE), (uint16_t)(dst % TUX_MEM_SIZE), &genome, entropy.pool, &active_code_count);
            break;
        }
        case OP_DECAY: {
            int64_t a = spop(&act->stack);
            if (act == &ctxA && (a < 0 || a >= TUX_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a >= 0 && a < TUX_MEM_SIZE) {
                memory[a].age = TUX_AGE_DEAD;
                memory[a].flags |= TUX_FLAG_CORRUPTED;
                memory[a].type_tag = TUX_TYPE_I64;
                memory[a].raw_code = (uint16_t)(tux_crazy64((uint64_t)memory[a].val, (uint64_t)a) & 0xFFFF);
            }
            break;
        }
        case OP_WAKE: {
            int64_t a = spop(&act->stack);
            if (a >= 0 && a < TUX_MEM_SIZE) {
                memory[a].flags &= ~TUX_FLAG_DORMANT;
            }
            break;
        }
        case OP_REINTERPRET:
            if (act->stack_tags.n > 0) {
                act->stack_tags.d[act->stack_tags.n - 1] = inst.arg;
            }
            break;
        case OP_UNDO:
            if (!tux_history_undo(&hist, act, memory)) {
                if (act == &ctxA) troll_die(TR_NO_HISTORY);
            }
            break;
        case OP_PAY_TIME: {
            uint64_t pay = time_debt.accumulated_debt > 50 ? 50 : time_debt.accumulated_debt;
            time_debt.accumulated_debt -= pay;
            time_debt.paid_total += pay;
            fish_grams -= 10;
            if (fish_grams < 0) troll_die(TR_STARVATION);
            break;
        }
        case OP_NOP:
        default:
            break;
        }

        /* Сохранение в историю при необходимости */
        if (config->is_reversible) {
            tux_history_push(&hist, &h_entry);
        }

        /* ФАЗА 4: MUTATE */
        tux_mutate_cell(&memory[pc], last_exec_result, program_key, genome.chromosomes[0], entropy.pool, pc);

        /* ФАЗА 5: REGISTER COUPLING (внутри active_ctx) */
        tux_register_coupling(act->regs);

        /* ФАЗА 6: GENOME UPDATE (производит G_{t+1}) */
        tux_genome_evolve(&genome, last_exec_result, entropy.pool, act->regs, pc);
        tux_entropy_step(&entropy, pc, (uint64_t)act->regs[0], genome.chromosomes[0]);

        /* ФАЗА 6.1: DORMANT RESONANCE (использует G_{t+1}) */
        tux_check_dormant_resonance(memory, &genome);

        /* ФАЗА 7: PC UPDATE (ТОЛЬКО для active_ctx, использует G_{t+1}) */
        if (is_branch_taken) {
            act->pc_code = pending_pc % TUX_MEM_SIZE;
        } else {
            int dir_offset = (act->dir == 1 ? 5 : (act->dir == 2 ? -1 : (act->dir == 3 ? -5 : 1)));
            int64_t next_pc = (int64_t)act->pc_code + dir_offset;
            while (next_pc < 0) next_pc += TUX_MEM_SIZE;
            act->pc_code = (uint16_t)(next_pc % TUX_MEM_SIZE);
        }
        if (act == &ctxB && act->pc_code < prog->len && prog->len < TUX_MEM_SIZE) {
            act->pc_code = (uint16_t)(prog->len + (act->pc_code % (TUX_MEM_SIZE - prog->len)));
        }

        /* ФАЗА 8: SCHEDULE (использует G_{t+1} и актуальное состояние памяти) */
        if (config->mode == TUX_MODE_APOCALYPSE) {
            tux_scheduler_step(&sched, act, memory, &genome, entropy.pool, step_counter);
        }

        /* Завершение программы: если в Purgatory или Apocalypse основной контекст дошел до конца исходного кода и нет прыжка */
        if ((config->mode == TUX_MODE_PURGATORY || config->mode == TUX_MODE_APOCALYPSE) && act == &ctxA && act->pc_code >= prog->len) {
            break;
        }

        /* ФАЗА 9: step_counter++ */
        step_counter++;
    }

    free(ctxA.stack.d);
    free(ctxA.stack_tags.d);
    free(ctxB.stack.d);
    free(ctxB.stack_tags.d);
    free(vars.d);
    free(vars_tags.d);
    free(memory);
    for (size_t i = 0; i < lists.n; i++) free(lists.d[i].d);
    free(lists.d);
}

void vm_run(const Program *prog, const TuxVMConfig *config) {
    if (config->mode == TUX_MODE_CLASSIC) {
        run_classic(prog);
    } else if (config->mode == TUX_MODE_CURSED) {
        run_cursed(prog);
    } else {
        run_unified_vm(prog, config);
    }
}
