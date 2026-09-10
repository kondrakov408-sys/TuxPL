#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int64_t *d;
    size_t n, cap;
} Vec;

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

void vm_run(const Program *prog) {
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
    int dir = 0; /* 0: +1, 1: +5, 2: -1, 3: -5 */

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
                var_owned[c->arg] = 0; /* MOVED */
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
                    var_owned[c->arg] = 1; /* OWNED / RESTORED */
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
                regs[0] = (int64_t)r; /* Tu accumulator */
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
        case OP_REGGET:
            vec_push(&stack, regs[c->arg % 4]);
            vec_push(&stack_tags, reg_tags[c->arg % 4]);
            if (is_purg) {
                consecutive_pushes++;
                if (consecutive_pushes > 7) troll_die(TR_AVALANCHE);
            }
            break;
        case OP_REGSET:
            regs[c->arg % 4] = spop_tag(&stack, &stack_tags, &reg_tags[c->arg % 4]);
            if (is_purg) consecutive_pushes = 0;
            break;
        case OP_FISH:
            fish_grams += (c->arg > 0 ? (int)c->arg : 50);
            break;
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
            dir = (int)(c->arg % 4);
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
        case OP_JMP:
            pc = (size_t)c->arg;
            break;
        case OP_JZ: {
            int64_t v = spop(&stack);
            pc = (v == 0) ? (size_t)c->arg : pc + 1;
            break;
        }
        case OP_JNZ: {
            int64_t v = spop(&stack);
            pc = (v != 0) ? (size_t)c->arg : pc + 1;
            break;
        }
        case OP_LISTNEW:
            list_at(&lists, c->arg)->n = 0;
            break;
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
            int64_t v = spop(&stack);
            int64_t i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            break;
        }
        case OP_LISTLEN:
            vec_push(&stack, (int64_t)list_at(&lists, c->arg)->n);
            break;
        }

        if (c->op != OP_JMP && c->op != OP_JZ && c->op != OP_JNZ) {
            if (is_purg && dir != 0) {
                if (dir == 1) pc += 5;
                else if (dir == 2) pc--;
                else if (dir == 3) pc -= 5;
                else pc++;
            } else {
                pc++;
            }
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
