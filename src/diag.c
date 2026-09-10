#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static const char *const PANIC_NAMES[PANIC_CODE_COUNT] = {
    [PANIC_IO_ERROR]               = "PANIC_IO_ERROR",
    [PANIC_SYNTAX_ERROR]           = "PANIC_SYNTAX_ERROR",
    [PANIC_DIVZERO]                = "PANIC_DIVZERO",
    [PANIC_ARITHMETIC_OVERFLOW]    = "PANIC_ARITHMETIC_OVERFLOW",
    [PANIC_BAD_INPUT]              = "PANIC_BAD_INPUT",
    [PANIC_GBSV_SYNTAX]            = "PANIC_GBSV_SYNTAX",
    [PANIC_GBSV_B64]               = "PANIC_GBSV_B64",
    [PANIC_GBSV_GF]                = "PANIC_GBSV_GF",
    [PANIC_GBSV_TUX]               = "PANIC_GBSV_TUX",
    [PANIC_GBSV_ZETA]              = "PANIC_GBSV_ZETA",

    [PANIC_SPEC_FILENAME_MISMATCH] = "PANIC_SPEC_FILENAME_MISMATCH",
    [PANIC_SPEC_MISSING_LIBRARY]   = "PANIC_SPEC_MISSING_LIBRARY",
    [PANIC_SPEC_LINE_CYCLE]        = "PANIC_SPEC_LINE_CYCLE",
    [PANIC_SPEC_CHECKSUM]          = "PANIC_SPEC_CHECKSUM",
    [PANIC_SPEC_WHITESPACE_PARITY] = "PANIC_SPEC_WHITESPACE_PARITY",
    [PANIC_SPEC_BAD_JUMP]          = "PANIC_SPEC_BAD_JUMP",
    [PANIC_SPEC_NO_HISTORY]        = "PANIC_SPEC_NO_HISTORY",
    [PANIC_SPEC_DORMANT_EXEC]      = "PANIC_SPEC_DORMANT_EXEC",
    [PANIC_SPEC_PARADOX]           = "PANIC_SPEC_PARADOX",
    [PANIC_SPEC_AGE_LIMIT]         = "PANIC_SPEC_AGE_LIMIT",
    [PANIC_COMPANION_ORPHAN]       = "PANIC_COMPANION_ORPHAN",
    [PANIC_COMPANION_HERESY]       = "PANIC_COMPANION_HERESY",

    [PANIC_BUDGET_EXHAUSTION]      = "PANIC_BUDGET_EXHAUSTION",
    [PANIC_STACK_GRAVITY_OVERFLOW] = "PANIC_STACK_GRAVITY_OVERFLOW",
    [PANIC_AFFINE_USE_AFTER_MOVE]  = "PANIC_AFFINE_USE_AFTER_MOVE",
    [PANIC_TYPE_MISMATCH]          = "PANIC_TYPE_MISMATCH",
    [PANIC_STACK_UNDERFLOW]        = "PANIC_STACK_UNDERFLOW",
    [PANIC_MEM_OUT_OF_BOUNDS]      = "PANIC_MEM_OUT_OF_BOUNDS",
    [PANIC_DECODE_COLLAPSE]        = "PANIC_DECODE_COLLAPSE",
    [PANIC_MUTATION_COLLAPSE]      = "PANIC_MUTATION_COLLAPSE",
    [PANIC_GENOME_DIVERGENCE]      = "PANIC_GENOME_DIVERGENCE",
    [PANIC_EXECUTION_COLLAPSE]     = "PANIC_EXECUTION_COLLAPSE",
};

static const char *const PANIC_DEFAULTS[PANIC_CODE_COUNT] = {
    [PANIC_IO_ERROR]               = "I/O operation failed or inaccessible file stream",
    [PANIC_SYNTAX_ERROR]           = "Malformed source token or syntax boundary violation",
    [PANIC_DIVZERO]                = "Integer division by zero",
    [PANIC_ARITHMETIC_OVERFLOW]    = "Arithmetic integer overflow encountered",
    [PANIC_BAD_INPUT]              = "Malformed numeric input format on stdin",
    [PANIC_GBSV_SYNTAX]            = "Malformed GBSV terminator format :[<B64>|<GF>|<TUX>];<OP>}",
    [PANIC_GBSV_B64]               = "Base64 projection syndrome mismatch",
    [PANIC_GBSV_GF]                = "Galois field F_{2^8} AES syndrome mismatch",
    [PANIC_GBSV_TUX]               = "p-adic weighted valuation factorization mismatch",
    [PANIC_GBSV_ZETA]              = "Hamming collapse quantum operator mismatch",

    [PANIC_SPEC_FILENAME_MISMATCH] = "Filename does not match exact source bit length (<bits>.tux)",
    [PANIC_SPEC_MISSING_LIBRARY]   = "Required opcode library import declaration missing",
    [PANIC_SPEC_LINE_CYCLE]        = "Dynamic line length cycle (1-2-3-4-5) sequence violation",
    [PANIC_SPEC_CHECKSUM]          = "Line checksum suffix token mismatch",
    [PANIC_SPEC_WHITESPACE_PARITY] = "Invisible whitespace parity invariant violation",
    [PANIC_SPEC_BAD_JUMP]          = "Control flow target address out of instruction bounds",
    [PANIC_SPEC_NO_HISTORY]        = "Attempted state rollback (OP_UNDO) without reversible snapshot buffer",
    [PANIC_SPEC_DORMANT_EXEC]      = "Attempted execution of dormant memory cell without genome resonance",
    [PANIC_SPEC_PARADOX]           = "Reversible history buffer capacity exhausted during state capture",
    [PANIC_SPEC_AGE_LIMIT]         = "Instruction cell generation limit (255) exceeded",
    [PANIC_COMPANION_ORPHAN]       = "Required companion metadata container (.tu) not found",
    [PANIC_COMPANION_HERESY]       = "Companion container signature or source integrity hash mismatch (TUX2)",

    [PANIC_BUDGET_EXHAUSTION]      = "Operational metabolic gas budget exhausted (gas counter <= 0)",
    [PANIC_STACK_GRAVITY_OVERFLOW] = "Stack structural limit exceeded (>7 consecutive push operations)",
    [PANIC_AFFINE_USE_AFTER_MOVE]  = "Affine type violation: use-after-move on transferred slot",
    [PANIC_TYPE_MISMATCH]          = "Strict type system tag mismatch: unaligned power-of-two arithmetic",
    [PANIC_STACK_UNDERFLOW]        = "Evaluation stack underflow: pop from empty stack",
    [PANIC_MEM_OUT_OF_BOUNDS]      = "Memory address out of accessible 64K bounds",
    [PANIC_DECODE_COLLAPSE]        = "Instruction decoding collapse: unrecognizable cell bytecode",
    [PANIC_MUTATION_COLLAPSE]      = "Active executable cell limit (16384) exceeded in memory",
    [PANIC_GENOME_DIVERGENCE]      = "Program chromosome entropy divergence limit exceeded",
    [PANIC_EXECUTION_COLLAPSE]     = "Adversarial dual-context scheduler terminal deadlock",
};

int vm_panic_get_exit_code(VmPanicCode code) {
    if (code >= PANIC_BUDGET_EXHAUSTION) {
        return 3;
    }
    if (code >= PANIC_SPEC_FILENAME_MISMATCH) {
        return 2;
    }
    return 1;
}

const char *vm_panic_code_name(VmPanicCode code) {
    if (code < 0 || code >= PANIC_CODE_COUNT) {
        return "PANIC_UNKNOWN";
    }
    return PANIC_NAMES[code];
}

void vm_panic(VmPanicCode code, const char *fmt, ...) {
    if (code < 0 || code >= PANIC_CODE_COUNT) {
        code = PANIC_SYNTAX_ERROR;
    }

    const char *cat_name = PANIC_NAMES[code];
    char detail[1024];

    if (fmt && fmt[0]) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(detail, sizeof(detail), fmt, args);
        va_end(args);
    } else {
        snprintf(detail, sizeof(detail), "%s", PANIC_DEFAULTS[code]);
    }

    fprintf(stderr, "[VM_PANIC] %s: %s\n", cat_name, detail);
    exit(vm_panic_get_exit_code(code));
}

void vm_diag_debug(const char *msg) {
    fprintf(stderr, "[VM_DEBUG] %s\n", msg);
}
