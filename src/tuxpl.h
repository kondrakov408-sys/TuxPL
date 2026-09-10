#ifndef TUXPL_H
#define TUXPL_H

#include <stddef.h>
#include <stdint.h>
#include "debruijn.h"
#include "rns.h"

#define TUXPL_VERSION "2.0.0"

#define TUX_MEM_SIZE         65536
#define TUX_HISTORY_DEPTH    32
#define TUX_MAX_ACTIVE_CODE  16384
#define TUX_NUM_OPCODES      42
#define TUX_TIME_DEBT_LIMIT  5000

/* Опкоды TuxPL 2.0.0: 30 классических + 12 расширенных */
typedef enum {
    /* 0..7 */
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_DUP, OP_SWAP, OP_POP, OP_PRINTCHAR,
    /* 8..12 */
    OP_PUSH, OP_LOAD, OP_STORE, OP_LOADIND, OP_STOREIND,
    /* 13..16 */
    OP_JMP, OP_JZ, OP_JNZ, OP_CMP,
    /* 17..21 */
    OP_LISTNEW, OP_LISTPUSH, OP_LISTGET, OP_LISTSET, OP_LISTLEN,
    /* 22..23 */
    OP_PRINTNUM, OP_INPUTNUM,
    /* 24..29 */
    OP_REGGET, OP_REGSET, OP_FISH, OP_CRAZY, OP_CAST, OP_DIR,
    /* 30..41: Новые опкоды TuxPL 2.0.0 */
    OP_PUSH_PC, OP_SET_PC, OP_SWAP_PC, OP_ADD_PC, OP_XOR_PC,
    OP_CLONE, OP_DECAY, OP_WAKE, OP_REINTERPRET, OP_UNDO,
    OP_PAY_TIME, OP_NOP
} Opcode;

/* Семантические теги типов */
enum {
    TUX_TYPE_I8     = 0,
    TUX_TYPE_I16    = 1,
    TUX_TYPE_I32    = 2,
    TUX_TYPE_I64    = 3,
    TUX_TYPE_TRIT   = 4,
    TUX_TYPE_ADDR   = 5,
    TUX_TYPE_OPCODE = 6
};

/* Возрастные категории */
enum {
    TUX_AGE_YOUNG = 0,
    TUX_AGE_ADULT = 1,
    TUX_AGE_OLD   = 2,
    TUX_AGE_DEAD  = 3
};

/* Битовые флаги TuxCell */
#define TUX_FLAG_DORMANT     0x01
#define TUX_FLAG_MUTATED     0x02
#define TUX_FLAG_CLONED      0x04
#define TUX_FLAG_EXECUTABLE  0x08
#define TUX_FLAG_IMMUTABLE   0x10
#define TUX_FLAG_CORRUPTED   0x20
#define TUX_FLAG_RESERVED_1  0x40
#define TUX_FLAG_RESERVED_2  0x80

/* Ячейка памяти Unified Memory */
typedef struct {
    int64_t  val;          /* Каноническое хранилище данных */
    uint16_t raw_code;     /* 16-битный state payload */
    uint8_t  type_tag;     /* Семантический тип */
    uint8_t  age;          /* TUX_AGE_* */
    uint8_t  gen;          /* Поколение 0..255 */
    uint8_t  flags;        /* TUX_FLAG_* */
    uint32_t lineage;      /* Детерминированный ID родословной */
    uint32_t exec_count;   /* Счётчик исполнений (эволюционный, не откатывается) */
} TuxCell;

/* Паспорт программы */
typedef struct {
    uint64_t source_hash;   /* FNV-1a хэш сырых байтов до парсинга */
    uint64_t bit_len;       /* Точный вес файла в битах */
    char     tux_checksum;  /* T/U/X буква */
} ProgramFingerprint;

/* Геном программы */
typedef struct {
    uint64_t chromosomes[4]; /* G0..G3 */
    uint32_t generation;
    uint32_t divergence;
} TuxGenome;

/* Пул детерминированной энтропии */
typedef struct {
    uint64_t pool;
    uint64_t step_counter;
    uint64_t seed;
} TuxEntropy;

/* Динамический вектор чисел (для совместимости стека) */
typedef struct {
    int64_t *d;
    size_t n, cap;
} Vec;

/* Контекст исполнения (TUX_A / TUX_B) */
typedef struct {
    uint16_t pc_code;
    uint16_t pc_data;
    int64_t  regs[4];
    uint8_t  reg_tags[4];
    Vec      stack;
    Vec      stack_tags;
    uint8_t  dir;
    uint8_t  consecutive_pushes;
    uint8_t  active;
} TuxContext;

/* Планировщик */
typedef struct {
    uint8_t  current_ctx_id;
    uint64_t sched_state;
} TuxScheduler;

/* Долг времени */
typedef struct {
    uint64_t accumulated_debt;
    uint64_t debt_limit;
    uint64_t paid_total;
} TuxTimeDebt;

/* Запись в кольцевом буфере истории для UNDO */
typedef struct {
    uint8_t  ctx_id;
    uint16_t pc_code;
    uint16_t pc_data;
    int64_t  regs[4];
    uint8_t  reg_tags[4];
    uint16_t mem_addr;
    int64_t  mem_val_before;
    uint8_t  mem_tag_before;
    int64_t  stack_popped_val;
    uint8_t  stack_popped_tag;
    int      has_stack_popped;
    int      has_mem_modified;
    uint8_t  valid;
} TuxHistoryEntry;

typedef struct {
    TuxHistoryEntry entries[TUX_HISTORY_DEPTH];
    size_t head;
    size_t count;
} TuxHistoryBuffer;

/* Структура команды исходного кода (для парсера 1.0.0 и совместимости) */
typedef struct {
    Opcode op;
    int64_t arg;
    int type_tag;
    uint8_t initial_width;
} Cmd;

typedef struct {
    Cmd *cmds;
    size_t len;
    size_t cap;
    int is_purgatory;
    int is_apocalypse;
    int is_gbsv;
    char companion_name[64];
    int has_companion;
    ProgramFingerprint fp;
} Program;

#include "diag.h"

/* Режимы исполнения VM */
typedef enum {
    TUX_MODE_CLASSIC,
    TUX_MODE_STRICT,
    TUX_MODE_UNIFIED_VM,
    TUX_MODE_ADVERSARIAL,
    /* Совместимость со старыми алиасами */
    TUX_MODE_CURSED = TUX_MODE_STRICT,
    TUX_MODE_PURGATORY = TUX_MODE_UNIFIED_VM,
    TUX_MODE_APOCALYPSE = TUX_MODE_ADVERSARIAL
} TuxMode;

typedef struct {
    TuxMode mode;
    int is_reversible;
    int is_disasm;
    int is_trace;
    int is_trace_state;
    int is_gbsv;
    const char *filepath;
    const char *companion_path;
} TuxVMConfig;


/* genome.c — детерминированная математика, хэширование и эволюция */
uint64_t tux_crazy64(uint64_t a, uint64_t b);
uint64_t tux_source_hash(const char *raw_bytes, size_t len);
uint64_t tux_derive_program_key(const ProgramFingerprint *fp);
void tux_entropy_init(TuxEntropy *ent, uint64_t program_key);
void tux_entropy_step(TuxEntropy *ent, uint64_t pc, uint64_t reg0, uint64_t genome0);
void tux_genome_init(TuxGenome *gen, uint64_t program_key, const uint64_t *tu_genome_seed);
void tux_genome_evolve(TuxGenome *gen, int64_t result, uint64_t entropy_pool, const int64_t regs[4], uint64_t pc);

/* state.c — единая память Unified Memory, связанность регистров, история UNDO */
void tux_mem_init(TuxCell *mem, uint64_t program_key, uint64_t genome0);
uint16_t mutation_encode(int64_t val, uint64_t program_key, uint64_t genome0);
void tux_register_coupling(int64_t regs[4]);
void tux_history_push(TuxHistoryBuffer *hb, const TuxHistoryEntry *entry);
int  tux_history_undo(TuxHistoryBuffer *hb, TuxContext *ctx, TuxCell *mem);

/* decoder.c — динамический рантайм-декодер и ширина */
typedef struct {
    Opcode  op;
    int64_t arg;
    uint8_t width;
} DecodedInstruction;

DecodedInstruction decode_instruction(
    const TuxCell *cell,
    uint16_t pc,
    uint64_t program_key,
    const int64_t regs[4],
    const TuxGenome *genome,
    uint64_t entropy_pool,
    uint8_t prev_width,
    int is_apocalypse
);

/* mutation.c — самомодификация, старение, клонирование, распад */
void tux_mutate_cell(TuxCell *cell, int64_t result, uint64_t program_key, uint64_t genome0, uint64_t entropy_pool, uint16_t pc);
void tux_clone_cell(TuxCell *mem, uint16_t src_addr, uint16_t dst_addr, const TuxGenome *genome, uint64_t mutation_state, size_t *active_code_count);
void tux_check_dormant_resonance(TuxCell *mem, const TuxGenome *genome);

/* scheduler.c — детерминированный планировщик многозадачности */
void tux_scheduler_init(TuxScheduler *sched, uint64_t program_key);
uint8_t tux_scheduler_step(TuxScheduler *sched, const TuxContext *active_ctx, const TuxCell *mem, const TuxGenome *genome, uint64_t entropy_pool, uint64_t step_counter);

/* parse.c — парсинг и компиляция */
void parse_source(const char *src, Program *prog);
void parse_source_cursed(const char *src, Program *prog);
char calc_tux_checksum(const char *line, size_t len);
int  load_companion_file(const char *path, uint64_t expected_hash, uint64_t *out_genome, uint64_t *out_regs);

/* vm.c — исполнение */
void vm_run(const Program *prog, const TuxVMConfig *config);

#endif
