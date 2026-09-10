#ifndef DIAG_H
#define DIAG_H

#include <stddef.h>
#include <stdint.h>

/*
 * TuxPL 2.0.0 Control-Flow Integrity & Diagnostic Subsystem
 *
 * POSIX Process Exit Codes:
 *   1 - Base runtime panics (GBSV, I/O, parser syntax, math)
 *   2 - Structural specification violations & companion metadata invariants
 *   3 - Critical memory, resource budget, and architectural limits
 */

typedef enum {
    /* --- Exit Code 1: Base Runtime, Syntax, I/O, Cryptographic Verifications --- */
    PANIC_IO_ERROR,
    PANIC_SYNTAX_ERROR,
    PANIC_DIVZERO,
    PANIC_ARITHMETIC_OVERFLOW,
    PANIC_BAD_INPUT,
    PANIC_GBSV_SYNTAX,
    PANIC_GBSV_B64,
    PANIC_GBSV_GF,
    PANIC_GBSV_TUX,
    PANIC_GBSV_ZETA,

    /* --- Exit Code 2: Structural Specification & Companion Metadata Invariants --- */
    PANIC_SPEC_FILENAME_MISMATCH,
    PANIC_SPEC_MISSING_LIBRARY,
    PANIC_SPEC_LINE_CYCLE,
    PANIC_SPEC_CHECKSUM,
    PANIC_SPEC_WHITESPACE_PARITY,
    PANIC_SPEC_BAD_JUMP,
    PANIC_SPEC_NO_HISTORY,
    PANIC_SPEC_DORMANT_EXEC,
    PANIC_SPEC_PARADOX,
    PANIC_SPEC_AGE_LIMIT,
    PANIC_COMPANION_ORPHAN,
    PANIC_COMPANION_HERESY,

    /* --- Exit Code 3: Critical Memory, Resource Budget, and Architectural Limits --- */
    PANIC_BUDGET_EXHAUSTION,
    PANIC_STACK_GRAVITY_OVERFLOW,
    PANIC_AFFINE_USE_AFTER_MOVE,
    PANIC_TYPE_MISMATCH,
    PANIC_STACK_UNDERFLOW,
    PANIC_MEM_OUT_OF_BOUNDS,
    PANIC_DECODE_COLLAPSE,
    PANIC_MUTATION_COLLAPSE,
    PANIC_GENOME_DIVERGENCE,
    PANIC_EXECUTION_COLLAPSE,

    PANIC_CODE_COUNT
} VmPanicCode;

/* Core diagnostic reporter: prints [VM_PANIC] <CATEGORY>: <reason> and exits */
void vm_panic(VmPanicCode code, const char *fmt, ...) __attribute__((noreturn));

/* Return POSIX exit code (1, 2, or 3) for a given panic code */
int vm_panic_get_exit_code(VmPanicCode code);

/* Return canonical category string identifier */
const char *vm_panic_code_name(VmPanicCode code);

/* Optional debug telemetry output */
void vm_diag_debug(const char *msg);

/* Legacy mapping macros for transitional compatibility */
#define troll_die(cat) vm_panic((cat), NULL)
#define troll_debug(msg) vm_diag_debug(msg)

#define TR_GARBAGE               PANIC_SYNTAX_ERROR
#define TR_OPEN                  PANIC_SYNTAX_ERROR
#define TR_SPACE1                PANIC_SYNTAX_ERROR
#define TR_SEP                   PANIC_SYNTAX_ERROR
#define TR_END                   PANIC_SYNTAX_ERROR
#define TR_SIX                   PANIC_SYNTAX_ERROR
#define TR_EMPTYLINE             PANIC_SYNTAX_ERROR
#define TR_EMPTYFILE             PANIC_SYNTAX_ERROR
#define TR_OVERFLOW              PANIC_ARITHMETIC_OVERFLOW
#define TR_STACK                 PANIC_STACK_UNDERFLOW
#define TR_DIVZERO               PANIC_DIVZERO
#define TR_MEMRANGE              PANIC_MEM_OUT_OF_BOUNDS
#define TR_LISTRANGE             PANIC_MEM_OUT_OF_BOUNDS
#define TR_BADJUMP               PANIC_SPEC_BAD_JUMP
#define TR_BADINPUT              PANIC_BAD_INPUT
#define TR_CURSED_NAME           PANIC_SPEC_FILENAME_MISMATCH
#define TR_CURSED_NO_LIB         PANIC_SPEC_MISSING_LIBRARY
#define TR_CURSED_LINE_CYCLE     PANIC_SPEC_LINE_CYCLE
#define TR_CURSED_SYNTAX         PANIC_SYNTAX_ERROR
#define TR_CURSED_CHECKSUM       PANIC_SPEC_CHECKSUM
#define TR_STARVATION            PANIC_BUDGET_EXHAUSTION
#define TR_AVALANCHE             PANIC_STACK_GRAVITY_OVERFLOW
#define TR_USE_AFTER_MOVE        PANIC_AFFINE_USE_AFTER_MOVE
#define TR_TYPE_MISMATCH         PANIC_TYPE_MISMATCH
#define TR_WHITESPACE_TAMPERED   PANIC_SPEC_WHITESPACE_PARITY
#define TR_DECODE                PANIC_DECODE_COLLAPSE
#define TR_MUTATION              PANIC_MUTATION_COLLAPSE
#define TR_PARADOX              PANIC_SPEC_PARADOX
#define TR_GENOME                PANIC_GENOME_DIVERGENCE
#define TR_AGE                   PANIC_SPEC_AGE_LIMIT
#define TR_ORPHAN                PANIC_COMPANION_ORPHAN
#define TR_HERESY                PANIC_COMPANION_HERESY
#define TR_NO_HISTORY            PANIC_SPEC_NO_HISTORY
#define TR_DORMANT               PANIC_SPEC_DORMANT_EXEC
#define TR_EXECUTION             PANIC_EXECUTION_COLLAPSE
#define TR_GBSV_SYNTAX           PANIC_GBSV_SYNTAX
#define TR_GBSV_B64              PANIC_GBSV_B64
#define TR_GBSV_GF               PANIC_GBSV_GF
#define TR_GBSV_TUX              PANIC_GBSV_TUX
#define TR_GBSV_ZETA             PANIC_GBSV_ZETA

#endif /* DIAG_H */
