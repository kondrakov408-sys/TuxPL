CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2

SRC = src/main.c src/parse.c src/vm.c src/diag.c src/genome.c src/state.c src/decoder.c src/mutation.c src/scheduler.c src/gbsv.c src/debruijn.c src/rns.c
HDR = src/tuxpl.h src/gbsv.h src/diag.h src/debruijn.h src/rns.h

tuxpl: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test: tuxpl test-gbsv test-rns test-flat-core
	sh tests/run.sh
	sh tests/test_cursed.sh
	sh tests/test_hardcore.sh
	sh tests/test_golden_vectors.sh
	sh tests/test_apocalypse.sh

test-flat-core:
	$(CC) $(CFLAGS) -Isrc -o tests/test_flat_core tests/test_flat_core.c src/parse.c src/vm.c src/diag.c src/genome.c src/state.c src/decoder.c src/mutation.c src/scheduler.c src/gbsv.c src/debruijn.c src/rns.c
	./tests/test_flat_core
	rm -f tests/test_flat_core

test-rns:
	$(CC) $(CFLAGS) -Isrc -o tests/test_rns_debruijn tests/test_rns_debruijn.c src/debruijn.c src/rns.c src/diag.c
	./tests/test_rns_debruijn
	rm -f tests/test_rns_debruijn

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
	rm -f tuxpl tests/test_gbsv tests/test_rns_debruijn tests/test_flat_core

.PHONY: test test-flat-core test-rns test-gbsv test-hardcore test-apocalypse test-golden clean
