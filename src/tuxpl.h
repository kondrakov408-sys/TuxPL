#ifndef TUXPL_H
#define TUXPL_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_DUP, OP_SWAP, OP_POP, OP_PRINTCHAR,
    OP_PUSH, OP_LOAD, OP_STORE, OP_LOADIND, OP_STOREIND,
    OP_JMP, OP_JZ, OP_JNZ, OP_CMP,
    OP_LISTNEW, OP_LISTPUSH, OP_LISTGET, OP_LISTSET, OP_LISTLEN,
    OP_PRINTNUM, OP_INPUTNUM
} Opcode;

typedef struct {
    Opcode op;
    int64_t arg;
} Cmd;

typedef struct {
    Cmd *cmds;
    size_t len;
    size_t cap;
} Program;

/* troll.c — категории ошибок. Каждая — отдельный пул сообщений. */
enum {
    TR_GARBAGE,    /* символы/слова вне алфавита */
    TR_OPEN,       /* строка не начинается с {: */
    TR_SPACE1,     /* после {: нет ровно одного пробела */
    TR_SEP,        /* неправильный разделитель между командами */
    TR_END,        /* нет ; или ; не прижата к команде */
    TR_SIX,        /* больше 5 команд в строке */
    TR_EMPTYLINE,  /* строка без команд */
    TR_EMPTYFILE,  /* пустой файл */
    TR_OVERFLOW,   /* операнд не влезает в int64 */
    TR_STACK,      /* стек пуст */
    TR_DIVZERO,    /* деление на ноль */
    TR_MEMRANGE,   /* плохой адрес памяти */
    TR_LISTRANGE,  /* индекс вне списка */
    TR_BADJUMP,    /* прыжок в никуда */
    TR_BADINPUT,   /* на входе не число */
    TR_CURSED_NAME,       /* имя файла != число бит */
    TR_CURSED_WEEKEND,    /* выходной день: Тукс спит */
    TR_CURSED_NON_ARCH,   /* не Arch Linux система */
    TR_CURSED_NO_LIB,     /* операция без библиотеки Tux */
    TR_CURSED_LINE_CYCLE, /* строка нарушила цикл 1-2-3-4-5 */
    TR_CURSED_SYNTAX,     /* нарушение синтаксиса скобок/символов */
    TR_CURSED_CHECKSUM,   /* неверная контрольная буква T/U/X */
    TR_MATH_DEATH,        /* провал математического теста при --PLS */
    TR_CAT_COUNT
};

void troll_die(int cat);

/* parse.c — читает исходник, строит список команд. Ошибки — через troll_die. */
void parse_source(const char *src, Program *prog);
void parse_source_cursed(const char *src, Program *prog);
char calc_tux_checksum(const char *line, size_t len);

/* vm.c — исполнение. Ошибки — через troll_die. */
void vm_run(const Program *prog);

#endif
