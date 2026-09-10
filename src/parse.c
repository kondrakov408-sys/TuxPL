#include "tuxpl.h"
#include "gbsv.h"
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

static void prog_push(Program *p, Opcode op, int64_t arg, int nU) {
    if (p->len == p->cap) {
        size_t cap = p->cap ? p->cap * 2 : 64;
        Cmd *cmds = realloc(p->cmds, cap * sizeof(Cmd));
        if (!cmds) oom();
        p->cmds = cmds;
        p->cap = cap;
    }
    p->cmds[p->len].op = op;
    p->cmds[p->len].arg = arg;
    int tag = 0;
    if (arg >= -128 && arg <= 127) tag = 0;
    else if (arg >= -32768 && arg <= 32767) tag = 1;
    else if (arg >= -2147483648LL && arg <= 2147483647LL) tag = 2;
    else tag = 3;
    p->cmds[p->len].type_tag = tag;
    p->cmds[p->len].initial_width = (uint8_t)(nU >= 4 ? 4 : (nU <= 1 ? 1 : nU));
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

    if (nU > 2) {
        if (w.op == OP_LOADIND) {
            w.op = OP_REGGET;
        } else if (w.op == OP_STOREIND) {
            w.op = OP_REGSET;
        } else if (w.op == OP_LISTNEW && nU >= 7) {
            if (w.arg == 100) w.op = OP_FISH;
            else if (w.arg == 101) w.op = OP_CRAZY;
            else if (w.arg == 102) w.op = OP_DIR;
            else if (w.arg >= 103 && w.arg <= 106) {
                int cast_tag = (int)(w.arg - 103);
                w.op = OP_CAST;
                w.arg = cast_tag;
            }
            else if (w.arg == 107) w.op = OP_PUSH_PC;
            else if (w.arg == 108) w.op = OP_SET_PC;
            else if (w.arg == 109) w.op = OP_SWAP_PC;
            else if (w.arg == 110) w.op = OP_ADD_PC;
            else if (w.arg == 111) w.op = OP_XOR_PC;
            else if (w.arg == 112) w.op = OP_CLONE;
            else if (w.arg == 113) w.op = OP_DECAY;
            else if (w.arg == 114) w.op = OP_WAKE;
            else if (w.arg == 115) w.op = OP_REINTERPRET;
            else if (w.arg == 116) w.op = OP_UNDO;
            else if (w.arg == 117) w.op = OP_PAY_TIME;
            else if (w.arg == 118) w.op = OP_NOP;
        }
    }

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

static void validate_jumps(const Program *prog) {
    for (size_t c = 0; c < prog->len; c++) {
        Opcode op = prog->cmds[c].op;
        if (op == OP_JMP || op == OP_JZ || op == OP_JNZ) {
            int64_t t = prog->cmds[c].arg;
            if (t < 0 || (uint64_t)t > prog->len) troll_die(TR_BADJUMP);
        }
    }
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
    case OP_REGGET:    return "TuUX";
    case OP_REGSET:    return "TUux";
    case OP_FISH:      return "tuux";
    case OP_CRAZY:     return "tuux";
    case OP_CAST:      return "tuux";
    case OP_DIR:       return "tuux";
    case OP_PUSH_PC:   return "tuux";
    case OP_SET_PC:    return "tuux";
    case OP_SWAP_PC:   return "tuux";
    case OP_ADD_PC:    return "tuux";
    case OP_XOR_PC:    return "tuux";
    case OP_CLONE:     return "tuux";
    case OP_DECAY:     return "tuux";
    case OP_WAKE:      return "tuux";
    case OP_REINTERPRET: return "tuux";
    case OP_UNDO:      return "tuux";
    case OP_PAY_TIME:  return "tuux";
    case OP_NOP:       return "tuux";
    default:           return "TuX";
    }
}

void parse_source(const char *src, Program *prog) {
    prog->cmds = NULL;
    prog->len = 0;
    prog->cap = 0;
    prog->has_companion = 0;
    prog->companion_name[0] = '\0';
    if (*src == '\0') troll_die(TR_EMPTYFILE);

    prog->fp.source_hash = tux_source_hash(src, strlen(src));
    prog->fp.bit_len = strlen(src) * 8;
    prog->fp.tux_checksum = 'T';

    static char licensed[512][64];
    int num_licensed = 0;
    int is_purgatory = 0;

    enum { CS_IMPORTS, CS_HEADER, CS_BODY, CS_END } state = CS_IMPORTS;
    int line_idx = 1;
    const char *prev_line = NULL;
    size_t prev_len = 0;

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
            /* Поддержка companion директивы: <~"COMPANION"/['<filename>.tu']~>! */
            if (llen > 19 && strncmp(p, "<~\"COMPANION\"/['", 16) == 0) {
                const char *end_bracket = strstr(p + 16, "']~>");
                if (!end_bracket) troll_die(TR_CURSED_SYNTAX);
                char term = end_bracket[4];
                if (term != '!' && term != '?') troll_die(TR_CURSED_SYNTAX);
                if ((size_t)(end_bracket + 5 - p) != llen) troll_die(TR_CURSED_SYNTAX);

                size_t cnamelen = (size_t)(end_bracket - (p + 16));
                if (cnamelen == 0 || cnamelen >= sizeof(prog->companion_name)) troll_die(TR_CURSED_SYNTAX);
                memcpy(prog->companion_name, p + 16, cnamelen);
                prog->companion_name[cnamelen] = '\0';
                prog->has_companion = 1;
                p = eol ? eol + 1 : p + llen;
                continue;
            }

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
                if (strcmp(lib_name, "TuuuuuuuuX") == 0 || strcmp(lib_name, "TuuuuuuuuuX") == 0) {
                    is_purgatory = 1;
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

            if (is_purgatory) {
                size_t ws_count = 0;
                for (size_t wi = 0; wi < llen; wi++) {
                    if (p[wi] == ' ' || p[wi] == '\t') ws_count++;
                }
                if ((ws_count % 2) != (size_t)(line_idx % 2)) {
                    troll_die(TR_WHITESPACE_TAMPERED);
                }
            }

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

                prog_push(prog, w.op, w.arg, w.nU);
                count++;

                if (count < target_count) {
                    check_sep(w.nU, p, llen, &i);
                } else {
                    /* Must have exactly one space before terminator */
                    if (i >= llen || p[i] != ' ') troll_die(TR_CURSED_SYNTAX);
                    i++;

                    /* Mandatory GBSV verification on every instruction line */
                    gbsv_verify_line_or_die(p, llen, prev_line, prev_len);
                    i += 11;
                    if (is_purgatory) {
                        while (i < llen && (p[i] == ' ' || p[i] == '\t')) i++;
                    }
                    if (i != llen) {
                        troll_die(TR_GBSV_SYNTAX);
                    }
                    break;
                }
            }

            if (count != target_count) troll_die(TR_CURSED_LINE_CYCLE);
            prev_line = p;
            prev_len = llen;
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

int load_companion_file(const char *path, uint64_t expected_hash, uint64_t *out_genome, uint64_t *out_regs) {
    FILE *f = fopen(path, "rb");
    if (!f) troll_die(TR_ORPHAN);
    uint8_t buf[0x58];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf)) {
        fclose(f);
        troll_die(TR_HERESY);
    }
    fclose(f);

    /* Magic "TUX2" */
    if (buf[0] != 'T' || buf[1] != 'U' || buf[2] != 'X' || buf[3] != '2') {
        troll_die(TR_HERESY);
    }
    /* Version 0x0200 */
    uint16_t ver = (uint16_t)buf[4] | ((uint16_t)buf[5] << 8);
    if (ver != 0x0200) {
        troll_die(TR_HERESY);
    }
    /* Source hash check */
    uint64_t file_src_hash = 0;
    for (int i = 0; i < 8; i++) file_src_hash |= ((uint64_t)buf[8 + i] << (i * 8));
    if (file_src_hash != 0 && expected_hash != 0 && file_src_hash != expected_hash) {
        troll_die(TR_HERESY);
    }
    /* Checksum over first 0x50 bytes */
    uint64_t calc_h = tux_source_hash((const char *)buf, 0x50);
    uint64_t stored_h = 0;
    for (int i = 0; i < 8; i++) stored_h |= ((uint64_t)buf[0x50 + i] << (i * 8));
    if (calc_h != stored_h) {
        troll_die(TR_HERESY);
    }

    if (out_genome) {
        for (int c = 0; c < 4; c++) {
            uint64_t v = 0;
            for (int i = 0; i < 8; i++) v |= ((uint64_t)buf[0x10 + c * 8 + i] << (i * 8));
            out_genome[c] = v;
        }
    }
    if (out_regs) {
        for (int r = 0; r < 4; r++) {
            uint64_t v = 0;
            for (int i = 0; i < 8; i++) v |= ((uint64_t)buf[0x30 + r * 8 + i] << (i * 8));
            out_regs[r] = v;
        }
    }
    return 1;
}
