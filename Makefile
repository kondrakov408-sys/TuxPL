CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2

SRC = src/main.c src/parse.c src/vm.c src/troll.c
HDR = src/tuxpl.h

tuxpl: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test: tuxpl
	sh tests/run.sh

clean:
	rm -f tuxpl

.PHONY: test clean
