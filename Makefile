CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2

SRC = src/main.c src/parse.c src/vm.c src/diag.c src/genome.c src/state.c src/decoder.c src/mutation.c src/scheduler.c src/gbsv.c
HDR = src/tuxpl.h src/gbsv.h src/diag.h

tuxpl: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test: tuxpl test-gbsv
	sh tests/run.sh
	sh tests/test_cursed.sh
	sh tests/test_hardcore.sh
	sh tests/test_golden_vectors.sh
	sh tests/test_apocalypse.sh

test-gbsv:
	$(CC) $(CFLAGS) -o tests/test_gbsv tests/test_gbsv.c src/gbsv.c src/diag.c
	./tests/test_gbsv
	rm -f tests/test_gbsv

test-hardcore: tuxpl
	sh tests/test_hardcore.sh

test-apocalypse: tuxpl
	sh tests/test_apocalypse.sh

test-golden: tuxpl
	sh tests/test_golden_vectors.sh

clean:
	rm -f tuxpl tests/test_gbsv

.PHONY: test test-gbsv test-hardcore test-apocalypse test-golden clean
