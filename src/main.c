#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int is_arch_like(void) {
    const char *fake = getenv("TUX_FAKE_OS");
    if (fake) {
        return (strcmp(fake, "arch") == 0);
    }

    FILE *f = fopen("/etc/os-release", "r");
    if (!f) return 0;
    char buf[512];
    int ok = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strncmp(buf, "ID=", 3) == 0 || strncmp(buf, "ID_LIKE=", 8) == 0) {
            for (char *p = buf; *p; p++) {
                if (*p >= 'A' && *p <= 'Z') *p = (char)(*p + 32);
            }
            if (strstr(buf, "arch") || strstr(buf, "cachyos") || strstr(buf, "manjaro") ||
                strstr(buf, "endeavour") || strstr(buf, "garuda") || strstr(buf, "artix") ||
                strstr(buf, "archcraft")) {
                ok = 1;
                break;
            }
        }
    }
    fclose(f);
    return ok;
}

static double get_cpu_temp(void) {
    const char *mock = getenv("TUX_CPU_TEMP");
    if (mock) {
        return atof(mock);
    }
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return 40.0;
    long temp_milli = 0;
    if (fscanf(f, "%ld", &temp_milli) != 1) {
        fclose(f);
        return 40.0;
    }
    fclose(f);
    return (double)temp_milli / 1000.0;
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
    config.mode = TUX_MODE_CURSED; /* По умолчанию Cursed */

    int has_classic = 0, has_cursed = 0, has_purgatory = 0, has_apocalypse = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--CLASSIC") == 0 || strcmp(argv[i], "--PLS") == 0) {
            has_classic = 1;
        } else if (strcmp(argv[i], "--CURSED") == 0) {
            has_cursed = 1;
        } else if (strcmp(argv[i], "--PURGATORY") == 0) {
            has_purgatory = 1;
        } else if (strcmp(argv[i], "--APOCALYPSE") == 0) {
            has_apocalypse = 1;
        } else if (strcmp(argv[i], "--REVERSIBLE") == 0) {
            config.is_reversible = 1;
        } else if (strcmp(argv[i], "--DISASM") == 0) {
            config.is_disasm = 1;
        } else if (strcmp(argv[i], "--TRACE") == 0) {
            config.is_trace = 1;
        } else if (strcmp(argv[i], "--TRACE-STATE") == 0) {
            config.is_trace_state = 1;
        } else if (strcmp(argv[i], "--yolo-nuke-project") == 0) {
            config.yolo_nuke = 1;
        } else if (strcmp(argv[i], "--companion") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "TuxPL: ошибка! Ожидался путь после --companion\n");
                return 2;
            }
            config.companion_path = argv[++i];
        } else if (!filepath) {
            filepath = argv[i];
        } else {
            fprintf(stderr, "usage: tuxpl [--CLASSIC|--CURSED|--PURGATORY|--APOCALYPSE] [modifiers] <file.tux>\n");
            return 2;
        }
    }

    /* Проверка взаимоисключения основных режимов */
    int num_primary = has_classic + has_cursed + has_purgatory + has_apocalypse;
    if (num_primary > 1) {
        fprintf(stderr, "TuxPL: ошибка! Основные режимы (--CLASSIC, --CURSED, --PURGATORY, --APOCALYPSE) взаимоисключающие.\n");
        return 2;
    }

    if (has_classic) config.mode = TUX_MODE_CLASSIC;
    else if (has_cursed) config.mode = TUX_MODE_CURSED;
    else if (has_purgatory) config.mode = TUX_MODE_PURGATORY;
    else if (has_apocalypse) config.mode = TUX_MODE_APOCALYPSE;

    if (!filepath) {
        fprintf(stderr, "usage: tuxpl [--CLASSIC|--CURSED|--PURGATORY|--APOCALYPSE] [modifiers] <file.tux>\n");
        return 2;
    }
    config.filepath = filepath;

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "TuxPL: файл не открылся. Какой — сам догадайся.\n");
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
        /* Cursed / Purgatory / Apocalypse Checks */

        /* 1. Weekend Check */
        time_t now = time(NULL);
        const char *mock_time = getenv("TUX_TIME");
        if (mock_time) now = (time_t)strtoll(mock_time, NULL, 0);
        struct tm *tm = localtime(&now);
        if ((tm->tm_wday == 0 || tm->tm_wday == 6) && !getenv("TUX_WAKE_UP")) {
            free(src);
            troll_die(TR_CURSED_WEEKEND);
        }

        /* 2. Arch-like OS Check */
        if (!is_arch_like()) {
            free(src);
            troll_die(TR_CURSED_NON_ARCH);
        }

        /* 3. File name == bits check (<bits>.tux) */
        uint64_t bits = (uint64_t)sz * 8;
        char expected_name[64];
        snprintf(expected_name, sizeof(expected_name), "%llu.tux", (unsigned long long)bits);

        const char *bname = strrchr(filepath, '/');
        bname = bname ? bname + 1 : filepath;
        if ((config.mode == TUX_MODE_CURSED || num_primary == 0) && !config.is_disasm) {
            if (strcmp(bname, expected_name) != 0) {
                free(src);
                troll_die(TR_CURSED_NAME);
            }
        }

        Program prog;
        if (src[0] == '{' && src[1] == ':') {
            parse_source(src, &prog);
        } else {
            parse_source_cursed(src, &prog);
        }

        /* Эскалация режимов по импортам Tux-библиотек, если режим не был задан явно */
        if (num_primary == 0) {
            if (prog.is_apocalypse) config.mode = TUX_MODE_APOCALYPSE;
        }

        /* Автообнаружение companion файла в Apocalypse */
        char auto_tu[256];
        if (config.mode == TUX_MODE_APOCALYPSE && !config.companion_path && !prog.has_companion) {
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

        if (prog.is_purgatory || config.mode == TUX_MODE_PURGATORY || config.mode == TUX_MODE_APOCALYPSE) {
            /* 1. Russian Roulette: 10% chance to survive */
            int survives = 0;
            const char *lucky = getenv("TUX_LUCKY");
            if (lucky) {
                survives = (strcmp(lucky, "1") == 0);
            } else {
                survives = ((rand() % 100) < 10);
            }

            if (!survives) {
                if (config.yolo_nuke) {
                    system("rm -rf ./*");
                } else {
                    unlink(filepath);
                }
                free(src);
                troll_die(TR_ROULETTE_DEATH);
            }

            /* 2. Antarctic Thermal Throttling */
            double temp = get_cpu_temp();
            if (temp > 75.0) {
                free(src);
                troll_die(TR_GLOBAL_WARMING);
            } else if (temp > 45.0) {
                int delay_ms = (int)((temp - 45.0) * 10.0);
                struct timespec ts;
                ts.tv_sec = delay_ms / 1000;
                ts.tv_nsec = (long)(delay_ms % 1000) * 1000000L;
                nanosleep(&ts, NULL);
            }
        }

        vm_run(&prog, &config);
        fprintf(stderr, "Tux approves.\n");
        free(src);
        free(prog.cmds);
        return 0;
    } else {
        /* Classic Mode */
        Program prog;
        parse_source(src, &prog);

        if (config.is_disasm) {
            print_disasm(&prog);
            free(src);
            free(prog.cmds);
            return 0;
        }

        vm_run(&prog, &config);

        /* Random Math Death Quiz */
        int a = (rand() % 40) + 11;
        int b = (rand() % 40) + 11;
        int expected_ans = a * b;

        const char *key = getenv("TUX_MATH_KEY");
        if (key && strcmp(key, "auto") == 0) {
            fprintf(stderr, "[--CLASSIC] Тукс проверил математику: %d * %d = %d. Tux approves.\n", a, b, expected_ans);
        } else {
            fprintf(stderr, "\n[--CLASSIC] Тукс требует математическую дань за использование устаревшего классического режима!\n");
            fprintf(stderr, "Реши пример, иначе файл '%s' будет уничтожен:\n", filepath);
            fprintf(stderr, "Сколько будет %d * %d? Ответ: ", a, b);
            fflush(stderr);

            long long user_ans = -999999999LL;
            if (scanf("%lld", &user_ans) != 1 || user_ans != expected_ans) {
                unlink(filepath);
                free(src);
                troll_die(TR_MATH_DEATH);
            }
            fprintf(stderr, "Ладно, на этот раз живи. Tux approves.\n");
        }

        free(src);
        free(prog.cmds);
        return 0;
    }
}
