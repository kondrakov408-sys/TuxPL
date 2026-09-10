#include "tuxpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Слово: T + U+ + x. Регистр значим.
  Банк A (1 U):  опкод по регистрам T, U, x.
  Банк B (2+ U): опкод по T, первой U, второй U, x.
                 Операнд = хвост после второй U: u=0, U=1, старший бит первым.
*/

typedef struct {
    int nU;
    Opcode op;
    int64_t arg;
} Word;

static void oom(void) {
    fprintf(stderr, "TuxPL: у Тукса кончилась память. Буквально.\n");
    exit(1);
}

static void prog_push(Program *p, Opcode op, int64_t arg) {
    if (p->len == p->cap) {
        size_t cap = p->cap ? p->cap * 2 : 64;
        Cmd *cmds = realloc(p->cmds, cap * sizeof(Cmd));
        if (!cmds) oom();
        p->cmds = cmds;
        p->cap = cap;
    }
    p->cmds[p->len].op = op;
    p->cmds[p->len].arg = arg;
    p->len++;
}

static Word parse_word(const char *s, size_t n, size_t *i) {
    Word w = {0, OP_ADD, 0};
    size_t j = *i;

    if (s[j] != 'T' && s[j] != 't') troll_die(TR_GARBAGE);
    int bT = (s[j] == 'T');
    j++;

    int bits[2] = {0, 0};
    int nU = 0;
    while (j < n && (s[j] == 'U' || s[j] == 'u')) {
        if (nU < 2) bits[nU] = (s[j] == 'U');
        nU++;
        j++;
    }
    if (nU == 0 || j >= n || (s[j] != 'X' && s[j] != 'x')) troll_die(TR_GARBAGE);
    int bx = (s[j] == 'X');
    j++;

    static const Opcode bankA[8] = {
        OP_DUP, OP_SWAP, OP_POP, OP_PRINTCHAR,
        OP_SUB, OP_ADD, OP_MUL, OP_DIV
    };
    static const Opcode bankB[16] = {
        OP_LISTNEW, OP_CMP, OP_LISTPUSH, OP_LISTGET,
        OP_LISTSET, OP_LISTLEN, OP_PRINTNUM, OP_INPUTNUM,
        OP_LOAD, OP_PUSH, OP_STORE, OP_LOADIND,
        OP_STOREIND, OP_JMP, OP_JZ, OP_JNZ
    };

    if (nU == 1) {
        w.op = bankA[(bT << 2) | (bits[0] << 1) | bx];
    } else {
        w.op = bankB[(bT << 3) | (bits[0] << 2) | (bits[1] << 1) | bx];
    }

    /* операнд: хвост U после первых двух (после одной для банка A) */
    size_t k = *i + 1;
    int skip = (nU == 1) ? 1 : 2;
    while (skip > 0) { k++; skip--; }
    int over = 0;
    for (; k + 1 < j; k++) {
        int bit = (s[k] == 'U');
        if (w.arg > (INT64_MAX - bit) / 2) {
            over = 1;
        } else {
            w.arg = w.arg * 2 + bit;
        }
    }
    if (over) troll_die(TR_OVERFLOW);

    w.nU = nU;
    *i = j;
    return w;
}

static void check_sep(int nU, const char *s, size_t n, size_t *i) {
    size_t j = *i;
    int ok;
    if (nU == 1) {
        ok = (j < n && s[j] == ' ') &&
             !(j + 1 < n && (s[j + 1] == ' ' || s[j + 1] == '\t'));
    } else if (nU == 2) {
        ok = (j + 1 < n && s[j] == ' ' && s[j + 1] == ' ') &&
             !(j + 2 < n && (s[j + 2] == ' ' || s[j + 2] == '\t'));
    } else if (nU == 3) {
        ok = (j + 2 < n && s[j] == ' ' && s[j + 1] == ' ' && s[j + 2] == ' ') &&
             !(j + 3 < n && (s[j + 3] == ' ' || s[j + 3] == '\t'));
    } else {
        ok = (j < n && s[j] == '\t') && !(j + 1 < n && s[j + 1] == '\t');
    }
    if (!ok) troll_die(TR_SEP);
    *i = j + (nU >= 4 ? 1 : (size_t)nU);
}

static void parse_line(const char *s, size_t n, Program *prog) {
    if (n < 2 || s[0] != '{' || s[1] != ':') troll_die(TR_OPEN);
    size_t i = 2;
    if (i >= n || s[i] != ' ') troll_die(TR_SPACE1);
    i++;
    if (i < n && s[i] == ';') troll_die(TR_EMPTYLINE);

    int count = 0;
    for (;;) {
        Word w = parse_word(s, n, &i);
        prog_push(prog, w.op, w.arg);
        count++;
        if (count > 5) troll_die(TR_SIX);

        if (i >= n) troll_die(TR_END);
        if (s[i] == ';') {
            if (i + 1 != n) troll_die(TR_END);
            return;
        }

        check_sep(w.nU, s, n, &i);
        if (i >= n) troll_die(TR_END);
        if (s[i] == ';') troll_die(TR_END);
    }
}

static void validate_jumps(const Program *prog) {
    for (size_t c = 0; c < prog->len; c++) {
        Opcode op = prog->cmds[c].op;
        if (op == OP_JMP || op == OP_JZ || op == OP_JNZ) {
            int64_t t = prog->cmds[c].arg;
            if (t < 0 || (uint64_t)t > prog->len) troll_die(TR_BADJUMP);
        }
    }
}

void parse_source(const char *src, Program *prog) {
    prog->cmds = NULL;
    prog->len = 0;
    prog->cap = 0;
    if (*src == '\0') troll_die(TR_EMPTYFILE);

    const char *p = src;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t llen = eol ? (size_t)(eol - p) : strlen(p);
        while (llen > 0 && p[llen - 1] == '\r') llen--;
        parse_line(p, llen, prog);
        p = eol ? eol + 1 : p + llen;
    }
    if (prog->len == 0) troll_die(TR_EMPTYFILE);
    validate_jumps(prog);
}

char calc_tux_checksum(const char *line, size_t len) {
    (void)line;
    uint64_t bit_len = (uint64_t)len * 8;
    uint64_t p2 = 1, t2 = bit_len;
    while (t2 > 0 && t2 % 2 == 0) { p2 *= 2; t2 /= 2; }
    uint64_t p3 = 1, t3 = bit_len;
    while (t3 > 0 && t3 % 3 == 0) { p3 *= 3; t3 /= 3; }
    if (p3 > p2) return 'X';
    if (p2 > p3) return 'U';
    return 'T';
}

static const char *op_to_tux_lib(Opcode op) {
    switch (op) {
    case OP_ADD:       return "TuX";
    case OP_SUB:       return "Tux";
    case OP_MUL:       return "TUx";
    case OP_DIV:       return "TUX";
    case OP_DUP:       return "tux";
    case OP_SWAP:      return "tuX";
    case OP_POP:       return "tUx";
    case OP_PRINTCHAR: return "tUX";
    case OP_PUSH:      return "TuuX";
    case OP_LOAD:      return "Tuux";
    case OP_STORE:     return "TuUx";
    case OP_LOADIND:   return "TuUX";
    case OP_STOREIND:  return "TUux";
    case OP_JMP:       return "TUuX";
    case OP_JZ:        return "TUUx";
    case OP_JNZ:       return "TUUX";
    case OP_CMP:       return "tuuX";
    case OP_LISTNEW:   return "tuux";
    case OP_LISTPUSH:  return "tuUx";
    case OP_LISTGET:   return "tuUX";
    case OP_LISTSET:   return "tUux";
    case OP_LISTLEN:   return "tUuX";
    case OP_PRINTNUM:  return "tUUx";
    case OP_INPUTNUM:  return "tUUX";
    default:           return "TuX";
    }
}

void parse_source_cursed(const char *src, Program *prog) {
    prog->cmds = NULL;
    prog->len = 0;
    prog->cap = 0;
    if (*src == '\0') troll_die(TR_EMPTYFILE);

    static char licensed[512][64];
    int num_licensed = 0;

    enum { CS_IMPORTS, CS_HEADER, CS_BODY, CS_END } state = CS_IMPORTS;
    int line_idx = 1;

    const char *p = src;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t llen = eol ? (size_t)(eol - p) : strlen(p);
        while (llen > 0 && p[llen - 1] == '\r') llen--;

        if (llen == 0) {
            p = eol ? eol + 1 : p + llen;
            continue;
        }

        if (state == CS_IMPORTS) {
            /* Format: <~"TUX"/['<TuxWord>']~>! or ? */
            if (llen > 14 && strncmp(p, "<~\"TUX\"/['", 10) == 0) {
                const char *end_bracket = strstr(p + 10, "']~>");
                if (!end_bracket) troll_die(TR_CURSED_SYNTAX);
                char term = end_bracket[4];
                if (term != '!' && term != '?') troll_die(TR_CURSED_SYNTAX);
                if ((size_t)(end_bracket + 5 - p) != llen) troll_die(TR_CURSED_SYNTAX);

                size_t wlen = (size_t)(end_bracket - (p + 10));
                if (wlen == 0 || wlen >= 63) troll_die(TR_CURSED_SYNTAX);
                char lib_name[64];
                memcpy(lib_name, p + 10, wlen);
                lib_name[wlen] = '\0';

                /* Verify file Tux/<lib_name>.tux exists */
                char fpath[128];
                snprintf(fpath, sizeof(fpath), "Tux/%s.tux", lib_name);
                FILE *tf = fopen(fpath, "rb");
                if (!tf) troll_die(TR_CURSED_NO_LIB);
                fclose(tf);

                if (num_licensed < 512) {
                    memcpy(licensed[num_licensed], lib_name, 63);
                    licensed[num_licensed][63] = '\0';
                    num_licensed++;
                }
                p = eol ? eol + 1 : p + llen;
                continue;
            } else {
                state = CS_HEADER;
            }
        }

        if (state == CS_HEADER) {
            /* Check mandatory licenses */
            int has_line = 0, has_bracket = 0;
            for (int k = 0; k < num_licensed; k++) {
                if (strcmp(licensed[k], "TuuuX") == 0) has_line = 1;
                if (strcmp(licensed[k], "TuuuuX") == 0) has_bracket = 1;
            }
            if (!has_line || !has_bracket) troll_die(TR_CURSED_NO_LIB);

            /* Must match exactly: ~"TUX"('tux')/[TUX](){: */
            static const char expected_header[] = "~\"TUX\"('tux')/[TUX](){:";
            if (llen != strlen(expected_header) || strncmp(p, expected_header, llen) != 0) {
                troll_die(TR_CURSED_SYNTAX);
            }
            state = CS_BODY;
            p = eol ? eol + 1 : p + llen;
            continue;
        }

        if (state == CS_BODY) {
            /* Check for closing line: :}}//?!~; */
            static const char expected_close[] = ":}}//?!~;";
            if (llen == strlen(expected_close) && strncmp(p, expected_close, llen) == 0) {
                state = CS_END;
                p = eol ? eol + 1 : p + llen;
                continue;
            }

            /* Body line format: {:[~'Tux'~] (cmd1)  (cmd2) ... :C;!?} */
            int target_count = ((line_idx - 1) % 5) + 1; /* cycle 1, 2, 3, 4, 5 */

            static const char pfx[] = "{:[~'Tux'~] ";
            size_t pfx_len = strlen(pfx);
            if (llen < pfx_len + 8 || strncmp(p, pfx, pfx_len) != 0) {
                troll_die(TR_CURSED_SYNTAX);
            }

            size_t i = pfx_len;
            int count = 0;

            for (;;) {
                if (i >= llen || p[i] != '(') troll_die(TR_CURSED_SYNTAX);
                i++;
                Word w = parse_word(p, llen, &i);
                if (i >= llen || p[i] != ')') troll_die(TR_CURSED_SYNTAX);
                i++;

                /* Check license for opcode */
                const char *req_lib = op_to_tux_lib(w.op);
                int ok_lib = 0;
                for (int k = 0; k < num_licensed; k++) {
                    if (strcmp(licensed[k], req_lib) == 0) { ok_lib = 1; break; }
                }
                if (!ok_lib) troll_die(TR_CURSED_NO_LIB);

                prog_push(prog, w.op, w.arg);
                count++;

                if (count < target_count) {
                    check_sep(w.nU, p, llen, &i);
                } else {
                    /* Must have exactly one space before :C;!?} */
                    if (i >= llen || p[i] != ' ') troll_die(TR_CURSED_SYNTAX);
                    i++;

                    /* i is at ':' of :C;!?} */
                    size_t prefix_len = i;
                    char expected_c = calc_tux_checksum(p, prefix_len);

                    if (i >= llen || p[i] != ':') troll_die(TR_CURSED_SYNTAX);
                    i++;
                    if (i >= llen) troll_die(TR_CURSED_SYNTAX);
                    char actual_c = p[i];
                    if (actual_c != expected_c) troll_die(TR_CURSED_CHECKSUM);
                    i++;
                    if (i + 4 != llen || strncmp(p + i, ";!?}", 4) != 0) {
                        troll_die(TR_CURSED_SYNTAX);
                    }
                    break;
                }
            }

            if (count != target_count) troll_die(TR_CURSED_LINE_CYCLE);
            line_idx++;
            p = eol ? eol + 1 : p + llen;
            continue;
        }

        if (state == CS_END) {
            troll_die(TR_CURSED_SYNTAX);
        }
    }

    if (state != CS_END) troll_die(TR_CURSED_SYNTAX);
    if (prog->len == 0) troll_die(TR_EMPTYFILE);
    validate_jumps(prog);
}
