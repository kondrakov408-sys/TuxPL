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


/* 6. Логический оператор TUX_CRAZY над тритами {-1, 0, +1} */
static inline int trit_crazy(int ta, int tb) {
    static const int tbl[3][3] = {
        { +1, -1,  0 },  /* ta = -1 */
        { +1, +1, -1 },  /* ta =  0 */
        { -1,  0,  0 }   /* ta = +1 */
    };
    return tbl[ta + 1][tb + 1];
}

int64_t tux_crazy_alu(int64_t a, int64_t b) {
    int64_t res = 0;
    int64_t p3 = 1;
    for (int i = 0; i < 10; i++) {
        int rem_a = (int)((a % 3 + 3) % 3);
        int ta = (rem_a == 1) ? 1 : ((rem_a == 2) ? -1 : 0);
        a = (ta == 1) ? (a - 1) / 3 : ((ta == -1) ? (a + 1) / 3 : a / 3);

        int rem_b = (int)((b % 3 + 3) % 3);
        int tb = (rem_b == 1) ? 1 : ((rem_b == 2) ? -1 : 0);
        b = (tb == 1) ? (b - 1) / 3 : ((tb == -1) ? (b + 1) / 3 : b / 3);

        int tr = trit_crazy(ta, tb);
        res += (int64_t)tr * p3;
        p3 *= 3;
    }
    return res;
}

static uint32_t find_safe_code_word(int target_op) {
    for (uint32_t k = 1; k < 100; k++) {
        uint32_t w = (k * 42U + (uint32_t)target_op) % TUX_ZM_M;
        if (w != 0 && w % 17 != 0) {
            return w;
        }
    }
    return 42U + (uint32_t)target_op;
}

static void run_unified_vm(const Program *prog, const TuxVMConfig *config) {
    TuxVM vm;
    memset(&vm, 0, sizeof(vm));
    vm.unified_mem = calloc(UNIFIED_MEM_SIZE, sizeof(uint32_t));
    if (!vm.unified_mem) oom();
    vm.config = *config;

    uint64_t program_key = tux_derive_program_key(&prog->fp);

    /* Загрузка companion, если требуется */
    uint64_t tu_genome[4] = {0, 0, 0, 0};
    uint64_t tu_regs[4] = {0, 0, 0, 0};
    int has_companion_data = 0;

    const char *cpath = config->companion_path ? config->companion_path : (prog->has_companion ? prog->companion_name : NULL);
    if (cpath) {
        load_companion_file(cpath, prog->fp.source_hash, tu_genome, tu_regs);
        has_companion_data = 1;
    } else if (!config->is_no_shadow) {
        vm_panic(PANIC_COMPANION_ORPHAN, "Missing required companion file (.tu) for adversarial execution");
    }

    tux_genome_init(&vm.genome, program_key, has_companion_data ? tu_genome : NULL);
    tux_entropy_init(&vm.entropy, program_key);

    /* 1. Детерминированная инициализация ячеек в ZM */
    tux_mem_init(vm.unified_mem, program_key, vm.genome.chromosomes[0]);

    /* 2. Загрузка кода программы в плоскую память */
    for (size_t i = 0; i < prog->len && i < UNIFIED_MEM_SIZE; i++) {
        vm.unified_mem[i] = find_safe_code_word((int)prog->cmds[i].op);
    }

    /* 3. Инициализация контекстов */
    vm.ctxA.pc_code = 0;
    vm.ctxA.pc_data = 1024;
    vm.ctxA.active = 1;

    memset(&vm.ctxB, 0, sizeof(vm.ctxB));
    vm.ctxB.pc_code = (uint16_t)((program_key ^ vm.genome.chromosomes[1]) % UNIFIED_MEM_SIZE);
    if (vm.ctxB.pc_code < prog->len && prog->len < UNIFIED_MEM_SIZE) {
        vm.ctxB.pc_code = (uint16_t)(prog->len + (vm.ctxB.pc_code % (UNIFIED_MEM_SIZE - prog->len)));
    }
    vm.ctxB.pc_data = (uint16_t)((vm.ctxB.pc_code + 512 + (vm.genome.chromosomes[2] % 1024)) % UNIFIED_MEM_SIZE);
    if (vm.ctxB.pc_data == vm.ctxB.pc_code) vm.ctxB.pc_data = (vm.ctxB.pc_data + 1) % UNIFIED_MEM_SIZE;
    if (has_companion_data) {
        for (int r = 0; r < 4; r++) vm.ctxB.regs[r] = (int64_t)tu_regs[r];
    } else {
        for (int r = 0; r < 4; r++) vm.ctxB.regs[r] = (int64_t)vm.genome.chromosomes[r];
    }
    vm.ctxB.active = !config->is_no_shadow;

    tux_scheduler_init(&vm.sched, program_key);

    vm.time_debt.debt_limit = TUX_TIME_DEBT_LIMIT;

    size_t active_code_count = prog->len;
    uint8_t current_width = 1;
    vm.gas_budget = 100;
    const char *env_gas_purg = getenv("TUX_GAS_BUDGET");
    if (env_gas_purg) vm.gas_budget = atoi(env_gas_purg);

    Vec vars = {0, 0, 0};
    Vec vars_tags = {0, 0, 0};
    Lists lists = {0, 0, 0};
    uint8_t var_owned[64];
    memset(var_owned, 0, sizeof(var_owned));

    const uint64_t max_steps = 1000000;

    while (vm.step_counter < max_steps) {
        TuxContext *act = (vm.sched.current_ctx_id == 1 && vm.ctxB.active) ? &vm.ctxB : &vm.ctxA;
        const char *ctx_name = (act == &vm.ctxB) ? "TUX_B" : "TUX_A";

        /* ФАЗА 1: FETCH */
        uint16_t pc = act->pc_code;
        uint32_t raw_w = vm.unified_mem[pc];

        /* Проверка Borrow Checker: если ячейка перемещена (W == 0 mod 17) */
        if (raw_w % 17 == 0) {
            vm_panic(PANIC_AFFINE_USE_AFTER_MOVE, "Affine borrow checker: cell moved (W = 0 mod 17)");
        }

        /* Вычисление латентных инвариантов */
        uint8_t cell_age = tux_cell_age(raw_w);
        uint8_t cell_type = tux_cell_type(raw_w);

        /* ФАЗА 2: ДИНАМИЧЕСКИЙ СИНТЕЗ ОПКОДА */
        DecodedInstruction inst = decode_instruction(
            raw_w, pc, program_key, act->regs, &vm.genome, vm.entropy.pool, current_width,
            !config->is_no_shadow
        );
        current_width = inst.width;
        Opcode op = (act == &vm.ctxA && pc < prog->len) ? prog->cmds[pc].op : inst.op;

        /* Операнд */
        int64_t op_arg = (pc < prog->len && act == &vm.ctxA) ? prog->cmds[pc].arg : inst.arg;

        if (config->is_trace || config->is_trace_state) {
            fprintf(stderr, "[STEP %llu][%s] PC: %u (PCD: %u) | OP: %d (W: %u, age: %d, type: %d) | Debt: %llu",
                    (unsigned long long)vm.step_counter, ctx_name, pc, act->pc_data, op, raw_w, cell_age, cell_type,
                    (unsigned long long)vm.time_debt.accumulated_debt);
            if (config->is_trace_state) {
                fprintf(stderr, " | G0: 0x%016llX | E: 0x%016llX",
                        (unsigned long long)vm.genome.chromosomes[0], (unsigned long long)vm.entropy.pool);
            }
            fprintf(stderr, "\n");
        }

        /* ФАЗА 3: EXECUTE */
        vm.time_debt.accumulated_debt += (op == OP_CLONE ? 5 : (op == OP_CRAZY || op == OP_CAST ? 1 + (vm.entropy.pool % 3) : 1));
        if (vm.time_debt.accumulated_debt > vm.time_debt.debt_limit) {
            tux_genome_evolve(&vm.genome, -1, vm.entropy.pool, act->regs, pc);
        }

        vm.gas_budget--;
        if (vm.gas_budget < 0) vm_panic(PANIC_BUDGET_EXHAUSTION, "Operational metabolic gas budget exhausted");

        /* Снимок для UNDO */
        TuxHistoryEntry h_entry;
        memset(&h_entry, 0, sizeof(h_entry));
        h_entry.ctx_id = vm.sched.current_ctx_id;
        h_entry.pc_code = act->pc_code;
        h_entry.pc_data = act->pc_data;
        memcpy(h_entry.regs, act->regs, sizeof(act->regs));
        memcpy(h_entry.reg_tags, act->reg_tags, sizeof(act->reg_tags));

        uint16_t pending_pc = pc;
        int is_branch_taken = 0;
        int64_t last_exec_result = 0;

        /* Защита конкурирующего контекста TUX_B от краша в неинициализированной памяти */
        if (act == &vm.ctxB) {
            int needed_stack = 0;
            switch (op) {
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
            if (act->stack.n < (size_t)needed_stack) op = OP_NOP;
            if (op == OP_DIV && act->stack.n >= 2 && act->stack.d[act->stack.n - 1] == 0) op = OP_NOP;
            if (op >= OP_LISTNEW && op <= OP_LISTLEN) op = OP_NOP;
            if (op == OP_INPUTNUM || op == OP_PRINTCHAR || op == OP_PRINTNUM) op = OP_NOP;
            if (op == OP_LOAD || op == OP_STORE) {
                if (op_arg < 0 || op_arg >= 64 || (op == OP_LOAD && !var_owned[op_arg])) op = OP_NOP;
            }
            if (op == OP_LOADIND && act->stack.n >= 1) {
                int64_t addr = act->stack.d[act->stack.n - 1];
                if (addr < 0 || addr >= UNIFIED_MEM_SIZE) op = OP_NOP;
            }
            if (op == OP_STOREIND && act->stack.n >= 2) {
                int64_t addr = act->stack.d[act->stack.n - 2];
                if (addr < (int64_t)prog->len || addr >= UNIFIED_MEM_SIZE) op = OP_NOP;
            }
            if (op == OP_DECAY && act->stack.n >= 1) {
                int64_t addr = act->stack.d[act->stack.n - 1];
                if (addr < (int64_t)prog->len || addr >= UNIFIED_MEM_SIZE) op = OP_NOP;
            }
            if (op == OP_CLONE && act->stack.n >= 2) {
                int64_t dst = act->stack.d[act->stack.n - 1];
                if (dst < (int64_t)prog->len || dst >= UNIFIED_MEM_SIZE) op = OP_NOP;
            }
            if (op == OP_UNDO && (!config->is_reversible || vm.hist.count == 0)) op = OP_NOP;
            if (op == OP_PUSH && act->consecutive_pushes >= 7) op = OP_NOP;
            if (op == OP_PAY_TIME && vm.gas_budget < 10) op = OP_NOP;
        }

        switch (op) {
        case OP_PUSH: {
            int push_tag = (act == &vm.ctxA && pc < prog->len) ? prog->cmds[pc].type_tag : cell_type;
            vec_push(&act->stack, op_arg);
            vec_push(&act->stack_tags, push_tag);
            act->consecutive_pushes++;
            if (act == &vm.ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        }
        case OP_LOAD:
            if (act == &vm.ctxA && op_arg >= 0 && op_arg < 64) {
                if (!var_owned[op_arg]) troll_die(TR_USE_AFTER_MOVE);
                var_owned[op_arg] = 0;
            }
            vec_fit(&vars, (size_t)op_arg + 1);
            vec_fit(&vars_tags, (size_t)op_arg + 1);
            vec_push(&act->stack, vars.d[op_arg]);
            vec_push(&act->stack_tags, vars_tags.d[op_arg]);
            act->consecutive_pushes++;
            if (act == &vm.ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        case OP_STORE: {
            int tag = 0;
            int64_t v = spop_tag(&act->stack, &act->stack_tags, &tag);
            act->consecutive_pushes = 0;
            if (op_arg >= 0 && op_arg < 64) var_owned[op_arg] = 1;
            vec_fit(&vars, (size_t)op_arg + 1);
            vec_fit(&vars_tags, (size_t)op_arg + 1);
            vars.d[op_arg] = v;
            vars_tags.d[op_arg] = tag;
            last_exec_result = v;
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            act->consecutive_pushes = 0;
            if (act == &vm.ctxA && ta != tb) troll_die(TR_TYPE_MISMATCH);
            uint64_t r = (op == OP_ADD) ? (uint64_t)a + (uint64_t)b
                       : (op == OP_SUB) ? (uint64_t)a - (uint64_t)b
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
            if (act == &vm.ctxA && ta != tb) troll_die(TR_TYPE_MISMATCH);
            if (act == &vm.ctxA && b == 0) troll_die(TR_DIVZERO);
            if (act == &vm.ctxA && a == INT64_MIN && b == -1) troll_die(TR_OVERFLOW);
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
                if (act == &vm.ctxA) troll_die(TR_STACK);
                break;
            }
            int tag = act->stack_tags.n > 0 ? (int)act->stack_tags.d[act->stack_tags.n - 1] : 0;
            vec_push(&act->stack, act->stack.d[act->stack.n - 1]);
            vec_push(&act->stack_tags, tag);
            act->consecutive_pushes++;
            if (act == &vm.ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
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
            size_t ridx = (size_t)(((op_arg % 4) + 4) % 4);
            vec_push(&act->stack, act->regs[ridx]);
            vec_push(&act->stack_tags, act->reg_tags[ridx]);
            act->consecutive_pushes++;
            if (act == &vm.ctxA && act->consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            break;
        }
        case OP_REGSET: {
            size_t ridx = (size_t)(((op_arg % 4) + 4) % 4);
            int tag_tmp = 0;
            act->regs[ridx] = spop_tag(&act->stack, &act->stack_tags, &tag_tmp);
            act->reg_tags[ridx] = (uint8_t)tag_tmp;
            act->consecutive_pushes = 0;
            break;
        }
        case OP_FISH: {
            int add = (op_arg > 0 && op_arg <= 10000) ? (int)op_arg : 50;
            vm.gas_budget += add;
            if (vm.gas_budget > 100000) vm.gas_budget = 100000;
            break;
        }
        case OP_CRAZY: {
            int tb = 0, ta = 0;
            int64_t b = spop_tag(&act->stack, &act->stack_tags, &tb);
            int64_t a = spop_tag(&act->stack, &act->stack_tags, &ta);
            int64_t crz = tux_crazy_alu(a, b);
            vec_push(&act->stack, crz);
            vec_push(&act->stack_tags, ta);
            act->consecutive_pushes = 0;
            last_exec_result = crz;
            break;
        }
        case OP_CAST:
            if (act->stack_tags.n > 0) {
                act->stack_tags.d[act->stack_tags.n - 1] = op_arg;
            }
            break;
        case OP_DIR:
            act->dir = (uint8_t)(((op_arg % 4) + 4) % 4);
            break;
        case OP_INPUTNUM: {
            vec_fit(&vars, (size_t)op_arg + 1);
            long long x;
            if (scanf(" %lld", &x) != 1) troll_die(TR_BADINPUT);
            vars.d[op_arg] = (int64_t)x;
            break;
        }
        case OP_LOADIND: {
            int64_t a = spop(&act->stack);
            if (act == &vm.ctxA && (a < 0 || a >= UNIFIED_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a < 0 || a >= UNIFIED_MEM_SIZE) a = (a % UNIFIED_MEM_SIZE + UNIFIED_MEM_SIZE) % UNIFIED_MEM_SIZE;
            uint32_t val32 = vm.unified_mem[a];
            if (val32 % 17 == 0) {
                vm_panic(PANIC_AFFINE_USE_AFTER_MOVE, "Affine borrow checker: cell moved (W = 0 mod 17)");
            }
            vec_push(&act->stack, (int64_t)val32);
            vec_push(&act->stack_tags, tux_cell_type(val32));
            last_exec_result = (int64_t)val32;
            /* Торсионное спаривание: чтение данных через PCD сдвигает фазу PCC */
            if ((uint16_t)a == act->pc_data) {
                act->pc_code = (uint16_t)((act->pc_code + 1) % UNIFIED_MEM_SIZE);
            }
            break;
        }
        case OP_STOREIND: {
            int64_t v = spop(&act->stack);
            int64_t a = spop(&act->stack);
            if (act == &vm.ctxA && (a < 0 || a >= UNIFIED_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a < 0 || a >= UNIFIED_MEM_SIZE) a = (a % UNIFIED_MEM_SIZE + UNIFIED_MEM_SIZE) % UNIFIED_MEM_SIZE;
            h_entry.has_mem_modified = 1;
            h_entry.mem_addr = (uint16_t)a;
            h_entry.mem_val_before = vm.unified_mem[a];

            uint32_t val32 = (uint32_t)((v % TUX_ZM_M + TUX_ZM_M) % TUX_ZM_M);
            vm.unified_mem[a] = val32;
            last_exec_result = (int64_t)val32;
            break;
        }
        case OP_CMP: {
            int64_t b = spop(&act->stack), a = spop(&act->stack);
            vec_push(&act->stack, (a > b) - (a < b));
            vec_push(&act->stack_tags, TUX_TYPE_I8);
            break;
        }
        case OP_JMP:
            pending_pc = (uint16_t)(op_arg % UNIFIED_MEM_SIZE);
            is_branch_taken = 1;
            break;
        case OP_JZ: {
            int64_t v = spop(&act->stack);
            if (v == 0) {
                pending_pc = (uint16_t)(op_arg % UNIFIED_MEM_SIZE);
                is_branch_taken = 1;
            }
            break;
        }
        case OP_JNZ: {
            int64_t v = spop(&act->stack);
            if (v != 0) {
                pending_pc = (uint16_t)(op_arg % UNIFIED_MEM_SIZE);
                is_branch_taken = 1;
            }
            break;
        }
        case OP_LISTNEW: list_at(&lists, op_arg)->n = 0; break;
        case OP_LISTPUSH: {
            int64_t v = spop(&act->stack);
            vec_push(list_at(&lists, op_arg), v);
            break;
        }
        case OP_LISTGET: {
            Vec *L = list_at(&lists, op_arg);
            int64_t i = spop(&act->stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            vec_push(&act->stack, L->d[i]);
            break;
        }
        case OP_LISTSET: {
            Vec *L = list_at(&lists, op_arg);
            int64_t v = spop(&act->stack), i = spop(&act->stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            break;
        }
        case OP_LISTLEN:
            vec_push(&act->stack, (int64_t)list_at(&lists, op_arg)->n);
            break;

        /* TuxPL 2.0.0 расширения */
        case OP_PUSH_PC:
            vec_push(&act->stack, (int64_t)act->pc_code);
            vec_push(&act->stack_tags, TUX_TYPE_ADDR);
            break;
        case OP_SET_PC: {
            int64_t a = spop(&act->stack);
            pending_pc = (uint16_t)(a % UNIFIED_MEM_SIZE);
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
            while (n_pc < 0) n_pc += UNIFIED_MEM_SIZE;
            pending_pc = (uint16_t)(n_pc % UNIFIED_MEM_SIZE);
            is_branch_taken = 1;
            break;
        }
        case OP_XOR_PC: {
            int64_t mask = spop(&act->stack);
            pending_pc = (uint16_t)((act->pc_code ^ (uint16_t)mask) % UNIFIED_MEM_SIZE);
            is_branch_taken = 1;
            break;
        }
        case OP_CLONE: {
            int64_t dst = spop(&act->stack);
            int64_t src = spop(&act->stack);
            tux_clone_cell(vm.unified_mem, (uint16_t)(src % UNIFIED_MEM_SIZE), (uint16_t)(dst % UNIFIED_MEM_SIZE), &vm.genome, vm.entropy.pool, &active_code_count);
            break;
        }
        case OP_DECAY: {
            int64_t a = spop(&act->stack);
            if (act == &vm.ctxA && (a < 0 || a >= UNIFIED_MEM_SIZE)) troll_die(TR_MEMRANGE);
            if (a >= 0 && a < UNIFIED_MEM_SIZE) {
                vm.unified_mem[a] = 0; /* age(0) == DEAD */
            }
            break;
        }
        case OP_WAKE: {
            int64_t a = spop(&act->stack);
            if (a >= 0 && a < UNIFIED_MEM_SIZE) {
                if (vm.unified_mem[a] == 0) vm.unified_mem[a] = 1;
            }
            break;
        }
        case OP_REINTERPRET:
            if (act->stack_tags.n > 0) {
                act->stack_tags.d[act->stack_tags.n - 1] = op_arg;
            }
            break;
        case OP_UNDO:
            if (!tux_history_undo(&vm.hist, act, vm.unified_mem)) {
                if (act == &vm.ctxA) troll_die(TR_NO_HISTORY);
            }
            break;
        case OP_PAY_TIME: {
            uint64_t pay = vm.time_debt.accumulated_debt > 50 ? 50 : vm.time_debt.accumulated_debt;
            vm.time_debt.accumulated_debt -= pay;
            vm.time_debt.paid_total += pay;
            vm.gas_budget -= 10;
            if (vm.gas_budget < 0) vm_panic(PANIC_BUDGET_EXHAUSTION, "Metabolic gas budget exhausted during temporal debt payment");
            break;
        }
        case OP_NOP:
        default:
            break;
        }

        /* Сохранение в историю при необходимости */
        if (config->is_reversible) {
            tux_history_push(&vm.hist, &h_entry);
        }

        /* ФАЗА 4: MUTATE — SP-Round необратимое шифрование */
        uint32_t prev_w = vm.unified_mem[pc];
        vm.unified_mem[pc] = tux_sp_round(prev_w, pc);

        /* ФАЗА 5: REGISTER COUPLING (внутри active_ctx в режиме состязательности) */
        if (vm.ctxB.active) {
            tux_register_coupling(act->regs);
        }

        /* ФАЗА 6: GENOME UPDATE */
        tux_genome_evolve(&vm.genome, last_exec_result, vm.entropy.pool, act->regs, pc);
        tux_entropy_step(&vm.entropy, pc, (uint64_t)act->regs[0], vm.genome.chromosomes[0]);

        /* ФАЗА 7: PC UPDATE и торсионное возмущение PCD */
        if (is_branch_taken) {
            act->pc_code = pending_pc % UNIFIED_MEM_SIZE;
        } else {
            int dir_offset = (act->dir == 1 ? 5 : (act->dir == 2 ? -1 : (act->dir == 3 ? -5 : 1)));
            int64_t next_pc = (int64_t)act->pc_code + dir_offset;
            while (next_pc < 0) next_pc += UNIFIED_MEM_SIZE;
            act->pc_code = (uint16_t)(next_pc % UNIFIED_MEM_SIZE);
        }

        /* Торсионное спаривание счетчиков Dual-PC при инкременте PCC */
        act->pc_data = (uint16_t)((act->pc_data + (uint16_t)__builtin_popcount(prev_w) + 1) % UNIFIED_MEM_SIZE);

        if (act == &vm.ctxB && act->pc_code < prog->len && prog->len < UNIFIED_MEM_SIZE) {
            act->pc_code = (uint16_t)(prog->len + (act->pc_code % (UNIFIED_MEM_SIZE - prog->len)));
        }

        /* ФАЗА 8: SCHEDULE */
        if (vm.ctxB.active) {
            tux_scheduler_step(&vm.sched, act, vm.unified_mem, &vm.genome, vm.entropy.pool, vm.step_counter);
        }

        /* Завершение программы: Context A выполнил всю программу */
        if (act == &vm.ctxA && act->pc_code >= prog->len) {
            break;
        }

        /* ФАЗА 9: step_counter++ */
        vm.step_counter++;
    }

    free(vm.ctxA.stack.d);
    free(vm.ctxA.stack_tags.d);
    free(vm.ctxB.stack.d);
    free(vm.ctxB.stack_tags.d);
    free(vars.d);
    free(vars_tags.d);
    free(vm.unified_mem);
    for (size_t i = 0; i < lists.n; i++) free(lists.d[i].d);
    free(lists.d);
}

void vm_run(const Program *prog, const TuxVMConfig *config) {
    run_unified_vm(prog, config);
}
