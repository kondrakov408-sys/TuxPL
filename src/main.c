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
        "usage: %s [options] <file.tux>\n\n"
        "TuxPL 2.0.0 Monolithic Adversarial Kernel on Modular Torus Z_M\n\n"
        "Options:\n"
        "  --no-shadow, --deterministic Disable shadow Context B execution\n"
        "  --companion <path>           Provide companion metadata container (.tu)\n"
        "  --reversible                 Enable reversible execution with state snapshots\n"
        "  --disasm                     Display anti-disassembler static instruction stream\n"
        "  --trace                      Trace runtime instruction execution\n"
        "  --trace-state                Trace full system state (PC, registers, genome)\n"
        "  --version, -v                Show version information\n"
        "  --help, -h                   Show this help message\n",
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

    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-shadow") == 0 || strcmp(argv[i], "--deterministic") == 0) {
            config.is_no_shadow = 1;
        } else if (strcmp(argv[i], "--reversible") == 0 || strcmp(argv[i], "--REVERSIBLE") == 0) {
            config.is_reversible = 1;
        } else if (strcmp(argv[i], "--disasm") == 0 || strcmp(argv[i], "--DISASM") == 0) {
            config.is_disasm = 1;
        } else if (strcmp(argv[i], "--trace") == 0 || strcmp(argv[i], "--TRACE") == 0) {
            config.is_trace = 1;
        } else if (strcmp(argv[i], "--trace-state") == 0 || strcmp(argv[i], "--TRACE-STATE") == 0) {
            config.is_trace_state = 1;
        } else if (strcmp(argv[i], "--companion") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "TuxPL: error! Expected path following --companion\n");
                return 2;
            }
            config.companion_path = argv[++i];
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("TuxPL 2.0.0 (Adversarial ZM Torus Monolithic Core)\n");
            return 0;
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

    /* Structural specification check: Filename must equal bit count (<bits>.tux) */
    uint64_t bits = (uint64_t)sz * 8;
    char expected_name[64];
    snprintf(expected_name, sizeof(expected_name), "%llu.tux", (unsigned long long)bits);

    const char *bname = strrchr(filepath, '/');
    bname = bname ? bname + 1 : filepath;
    if (!config.is_disasm) {
        if (strcmp(bname, expected_name) != 0) {
            free(src);
            vm_panic(PANIC_SPEC_FILENAME_MISMATCH,
                     "Filename '%s' does not match source bit size (%llu bits, expected '%s')",
                     bname, (unsigned long long)bits, expected_name);
        }
    }

    Program prog;
    memset(&prog, 0, sizeof(prog));
    parse_source(src, &prog);

    /* Automatic discovery of companion file (.tu) if not explicitly provided */
    static char auto_tu[256];
    if (!config.companion_path) {
        if (prog.has_companion) {
            /* If companion specified in directive, resolve relative to filepath directory */
            const char *slash = strrchr(filepath, '/');
            if (slash) {
                size_t dirlen = (size_t)(slash - filepath + 1);
                snprintf(auto_tu, sizeof(auto_tu), "%.*s%s", (int)dirlen, filepath, prog.companion_name);
            } else {
                snprintf(auto_tu, sizeof(auto_tu), "%s", prog.companion_name);
            }
            config.companion_path = auto_tu;
        } else {
            /* Try replacing .tux with .tu */
            snprintf(auto_tu, sizeof(auto_tu), "%s", filepath);
            char *dot = strrchr(auto_tu, '.');
            if (dot) {
                strcpy(dot, ".tu");
                if (access(auto_tu, F_OK) == 0) {
                    config.companion_path = auto_tu;
                }
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
}
