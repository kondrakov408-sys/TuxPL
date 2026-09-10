CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2

SRC = src/main.c src/parse.c src/vm.c src/troll.c src/genome.c src/state.c src/decoder.c src/mutation.c src/scheduler.c
HDR = src/tuxpl.h

tuxpl: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test: tuxpl
	sh tests/run.sh
	sh tests/test_cursed.sh
	sh tests/test_hardcore.sh
	sh tests/test_golden_vectors.sh
	sh tests/test_apocalypse.sh

test-hardcore: tuxpl
	sh tests/test_hardcore.sh

test-apocalypse: tuxpl
	sh tests/test_apocalypse.sh

test-golden: tuxpl
	sh tests/test_golden_vectors.sh

clean:
	rm -f tuxpl

.PHONY: test test-hardcore clean
