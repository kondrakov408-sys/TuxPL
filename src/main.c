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

int main(int argc, char **argv) {
    int is_pls = 0;
    int yolo_nuke = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--PLS") == 0) {
            is_pls = 1;
        } else if (strcmp(argv[i], "--yolo-nuke-project") == 0) {
            yolo_nuke = 1;
        } else if (!filepath) {
            filepath = argv[i];
        } else {
            fprintf(stderr, "usage: tuxpl [--PLS] [--yolo-nuke-project] <file.tux>\n");
            return 2;
        }
    }

    if (!filepath) {
        fprintf(stderr, "usage: tuxpl [--PLS] [--yolo-nuke-project] <file.tux>\n");
        return 2;
    }

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

    if (!is_pls) {
        /* Cursed Mode Checks */

        /* 1. Weekend Check */
        time_t now = time(NULL);
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
        if (strcmp(bname, expected_name) != 0) {
            free(src);
            troll_die(TR_CURSED_NAME);
        }

        Program prog;
        parse_source_cursed(src, &prog);

        if (prog.is_purgatory) {
            /* 1. Russian Roulette: 10% chance to survive */
            int survives = 0;
            const char *lucky = getenv("TUX_LUCKY");
            if (lucky) {
                survives = (strcmp(lucky, "1") == 0);
            } else {
                survives = ((rand() % 100) < 10);
            }

            if (!survives) {
                if (yolo_nuke) {
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

        vm_run(&prog);
        fprintf(stderr, "Tux approves.\n");
        free(src);
        return 0;
    } else {
        /* Classic Mode (--PLS) with Math Quiz */
        Program prog;
        parse_source(src, &prog);
        vm_run(&prog);

        /* Random Math Death Quiz */
        int a = (rand() % 40) + 11;
        int b = (rand() % 40) + 11;
        int expected_ans = a * b;

        const char *key = getenv("TUX_MATH_KEY");
        if (key && strcmp(key, "auto") == 0) {
            fprintf(stderr, "[--PLS] Тукс проверил математику: %d * %d = %d. Tux approves.\n", a, b, expected_ans);
        } else {
            fprintf(stderr, "\n[--PLS] Тукс требует математическую дань за использование устаревшего классического режима!\n");
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
        return 0;
    }
}
