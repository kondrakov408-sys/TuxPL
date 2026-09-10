#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void print_usage(const char *prog_name) {
    fprintf(stderr,
        "usage: %s [--CLASSIC|--STRICT|--UNIFIED-VM|--ADVERSARIAL] [modifiers] <file.tux>\n\n"
        "Primary Execution Modes:\n"
        "  --CLASSIC            Classic TuxPL execution mode\n"
        "  --STRICT             Strict specification & structural verification\n"
        "  --UNIFIED-VM         Unified memory architecture with metabolic gas budget\n"
        "  --ADVERSARIAL        Adversarial dual-context scheduler execution\n\n"
        "Execution Modifiers:\n"
        "  --companion <path>   Provide companion metadata container (.tu)\n"
        "  --REVERSIBLE         Enable reversible execution with state snapshots\n"
        "  --gbsv               Enable Galois-Base64 Syndrome Verification\n"
        "  --DISASM             Display anti-disassembler static instruction stream\n"
        "  --TRACE              Trace runtime instruction execution\n"
        "  --TRACE-STATE        Trace full system state (PC, registers, genome)\n"
        "  --help, -h           Show this help message\n",
        prog_name);
}

static void print_disasm(const Program *prog) {
    printf("=== TuxPL 2.0.0 Polymorphic Anti-Disassembler ===\n");
    printf("Program bits: %llu | Source hash: 0x%016llX | Checksum: %c\n",
           (unsigned long long)prog->fp.bit_len,
           (unsigned long long)prog->fp.source_hash,
           prog->fp.tux_checksum);
    printf("--------------------------------------------------\n");
    for (size_t i = 0; i < prog->len; i++) {
        const Cmd *c = &prog->cmds[i];
        printf("[ADDR %04zu] ARG: %-5lld | WIDTH: %u | STATIC: AMBIGUOUS | RUNTIME: POLYMORPHIC (Base Op: %d)\n",
               i, (long long)c->arg, c->initial_width, c->op);
    }
    printf("==================================================\n");
}

int main(int argc, char **argv) {
    TuxVMConfig config;
    memset(&config, 0, sizeof(config));
    config.mode = TUX_MODE_STRICT; /* Default mode: Strict */

    int has_classic = 0, has_strict = 0, has_unified = 0, has_adversarial = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        /* Primary modes and hidden backward-compatible aliases */
        if (strcmp(argv[i], "--CLASSIC") == 0 || strcmp(argv[i], "--PLS") == 0) {
            has_classic = 1;
        } else if (strcmp(argv[i], "--STRICT") == 0 || strcmp(argv[i], "--SPEC") == 0 || strcmp(argv[i], "--CURSED") == 0) {
            has_strict = 1;
        } else if (strcmp(argv[i], "--UNIFIED-VM") == 0 || strcmp(argv[i], "--PURGATORY") == 0) {
            has_unified = 1;
        } else if (strcmp(argv[i], "--ADVERSARIAL") == 0 || strcmp(argv[i], "--APOCALYPSE") == 0) {
            has_adversarial = 1;
        } else if (strcmp(argv[i], "--REVERSIBLE") == 0) {
            config.is_reversible = 1;
        } else if (strcmp(argv[i], "--DISASM") == 0) {
            config.is_disasm = 1;
        } else if (strcmp(argv[i], "--TRACE") == 0) {
            config.is_trace = 1;
        } else if (strcmp(argv[i], "--TRACE-STATE") == 0) {
            config.is_trace_state = 1;
        } else if (strcmp(argv[i], "--gbsv") == 0) {
            config.is_gbsv = 1;
        } else if (strcmp(argv[i], "--companion") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "TuxPL: error! Expected path following --companion\n");
                return 2;
            }
            config.companion_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (!filepath && argv[i][0] != '-') {
            filepath = argv[i];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    /* Mutual exclusivity check for primary execution modes */
    int num_primary = has_classic + has_strict + has_unified + has_adversarial;
    if (num_primary > 1) {
        fprintf(stderr, "TuxPL: error! Primary modes (--CLASSIC, --STRICT, --UNIFIED-VM, --ADVERSARIAL) are mutually exclusive.\n");
        return 2;
    }

    if (has_classic) config.mode = TUX_MODE_CLASSIC;
    else if (has_strict) config.mode = TUX_MODE_STRICT;
    else if (has_unified) config.mode = TUX_MODE_UNIFIED_VM;
    else if (has_adversarial) config.mode = TUX_MODE_ADVERSARIAL;

    if (!filepath) {
        print_usage(argv[0]);
        return 2;
    }
    config.filepath = filepath;

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "TuxPL: error: unable to open source file '%s'\n", filepath);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 2; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 2; }
    rewind(f);
    char *src = malloc((size_t)sz + 1);
    if (!src) { fclose(f); return 2; }
    if (sz > 0 && fread(src, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        free(src);
        return 2;
    }
    src[sz] = '\0';
    fclose(f);

    srand((unsigned)time(NULL));

    if (config.mode != TUX_MODE_CLASSIC) {
        /* Structural specification check: Filename must equal bit count (<bits>.tux) */
        uint64_t bits = (uint64_t)sz * 8;
        char expected_name[64];
        snprintf(expected_name, sizeof(expected_name), "%llu.tux", (unsigned long long)bits);

        const char *bname = strrchr(filepath, '/');
        bname = bname ? bname + 1 : filepath;
        if ((config.mode == TUX_MODE_STRICT || num_primary == 0) && !config.is_disasm) {
            if (strcmp(bname, expected_name) != 0) {
                free(src);
                vm_panic(PANIC_SPEC_FILENAME_MISMATCH,
                         "Filename '%s' does not match source bit size (%llu bits, expected '%s')",
                         bname, (unsigned long long)bits, expected_name);
            }
        }

        Program prog;
        memset(&prog, 0, sizeof(prog));
        prog.is_gbsv = config.is_gbsv;
        if (src[0] == '{' && src[1] == ':') {
            parse_source(src, &prog);
        } else {
            parse_source_cursed(src, &prog);
        }

        /* Auto-escalate mode based on imported libraries if no explicit mode was passed */
        if (num_primary == 0) {
            if (prog.is_apocalypse) config.mode = TUX_MODE_ADVERSARIAL;
        }

        /* Automatic discovery of companion file (.tu) in adversarial mode */
        char auto_tu[256];
        if (config.mode == TUX_MODE_ADVERSARIAL && !config.companion_path && !prog.has_companion) {
            snprintf(auto_tu, sizeof(auto_tu), "%s", filepath);
            char *dot = strrchr(auto_tu, '.');
            if (dot) {
                strcpy(dot, ".tu");
                if (access(auto_tu, F_OK) == 0) {
                    config.companion_path = auto_tu;
                }
            }
        }

        if (config.is_disasm) {
            print_disasm(&prog);
            free(src);
            free(prog.cmds);
            return 0;
        }

        vm_run(&prog, &config);
        fprintf(stderr, "Execution terminated cleanly. State verified.\n");
        free(src);
        free(prog.cmds);
        return 0;
    } else {
        /* Classic Execution Mode */
        Program prog;
        parse_source(src, &prog);

        if (config.is_disasm) {
            print_disasm(&prog);
            free(src);
            free(prog.cmds);
            return 0;
        }

        vm_run(&prog, &config);
        fprintf(stderr, "Execution terminated cleanly. State verified.\n");
        free(src);
        free(prog.cmds);
        return 0;
    }
}
