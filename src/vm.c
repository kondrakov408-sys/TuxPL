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

void vm_run(const Program *prog) {
    Vec stack = {0, 0, 0};
    Vec vars = {0, 0, 0};
    Vec mem = {0, 0, 0};
    Lists lists = {0, 0, 0};
    size_t pc = 0;

    while (pc < prog->len) {
        const Cmd *c = &prog->cmds[pc];
        switch (c->op) {
        case OP_PUSH:
            vec_push(&stack, c->arg);
            pc++;
            break;
        case OP_LOAD:
            vec_fit(&vars, (size_t)c->arg + 1);
            vec_push(&stack, vars.d[c->arg]);
            pc++;
            break;
        case OP_STORE: {
            int64_t v = spop(&stack);
            vec_fit(&vars, (size_t)c->arg + 1);
            vars.d[c->arg] = v;
            pc++;
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: {
            int64_t b = spop(&stack), a = spop(&stack);
            uint64_t r = (c->op == OP_ADD) ? (uint64_t)a + (uint64_t)b
                       : (c->op == OP_SUB) ? (uint64_t)a - (uint64_t)b
                                           : (uint64_t)a * (uint64_t)b;
            vec_push(&stack, (int64_t)r);
            pc++;
            break;
        }
        case OP_DIV: {
            int64_t b = spop(&stack), a = spop(&stack);
            if (b == 0) troll_die(TR_DIVZERO);
            if (a == INT64_MIN && b == -1) troll_die(TR_OVERFLOW);
            vec_push(&stack, a / b);
            pc++;
            break;
        }
        case OP_DUP: {
            if (stack.n == 0) troll_die(TR_STACK);
            vec_push(&stack, stack.d[stack.n - 1]);
            pc++;
            break;
        }
        case OP_SWAP: {
            int64_t b = spop(&stack), a = spop(&stack);
            vec_push(&stack, b);
            vec_push(&stack, a);
            pc++;
            break;
        }
        case OP_POP:
            spop(&stack);
            pc++;
            break;
        case OP_PRINTCHAR: {
            int64_t v = spop(&stack);
            fputc((int)(uint8_t)(v & 0xFF), stdout);
            pc++;
            break;
        }
        case OP_PRINTNUM: {
            int64_t v = spop(&stack);
            printf("%lld", (long long)v);
            pc++;
            break;
        }
        case OP_INPUTNUM: {
            vec_fit(&vars, (size_t)c->arg + 1);
            long long x;
            if (scanf(" %lld", &x) != 1) troll_die(TR_BADINPUT);
            vars.d[c->arg] = (int64_t)x;
            pc++;
            break;
        }
        case OP_LOADIND: {
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            vec_push(&stack, mem.d[a]);
            pc++;
            break;
        }
        case OP_STOREIND: {
            int64_t v = spop(&stack);
            int64_t a = mem_addr(spop(&stack));
            vec_fit(&mem, (size_t)a + 1);
            mem.d[a] = v;
            pc++;
            break;
        }
        case OP_CMP: {
            int64_t b = spop(&stack), a = spop(&stack);
            vec_push(&stack, (a > b) - (a < b));
            pc++;
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
            pc++;
            break;
        case OP_LISTPUSH: {
            int64_t v = spop(&stack);
            vec_push(list_at(&lists, c->arg), v);
            pc++;
            break;
        }
        case OP_LISTGET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            vec_push(&stack, L->d[i]);
            pc++;
            break;
        }
        case OP_LISTSET: {
            Vec *L = list_at(&lists, c->arg);
            int64_t v = spop(&stack);
            int64_t i = spop(&stack);
            if (i < 0 || (size_t)i >= L->n) troll_die(TR_LISTRANGE);
            L->d[i] = v;
            pc++;
            break;
        }
        case OP_LISTLEN:
            vec_push(&stack, (int64_t)list_at(&lists, c->arg)->n);
            pc++;
            break;
        }
    }

    free(stack.d);
    free(vars.d);
    free(mem.d);
    for (size_t i = 0; i < lists.n; i++) free(lists.d[i].d);
    free(lists.d);
}
