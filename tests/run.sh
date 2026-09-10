#!/bin/sh
set -e
cd "$(dirname "$0")/.."

export TUX_MATH_KEY=auto

pass=0
fail=0

check() {
    name=$1 expected=$2
    shift 2
    got=$("$@" 2>/dev/null)
    if [ "$got" = "$expected" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name: got [$got] want [$expected]"
    fi
}

expect_troll() {
    name=$1 file=$2
    if ./tuxpl --PLS "$file" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "FAIL $name: ждали троллинг, программа прошла"
    else
        pass=$((pass + 1))
    fi
}

# Тест: запуск классического файла без --PLS должен отвергаться (режим по умолчанию — Cursed)
if ./tuxpl examples/hi.tux >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "FAIL cursed_default: ждали отказ без --PLS"
else
    pass=$((pass + 1))
fi

check hi "Hi" ./tuxpl --PLS examples/hi.tux
check loop "54321" ./tuxpl --PLS examples/loop.tux
check sum "7" sh -c 'echo "3 4" | ./tuxpl --PLS examples/sum.tux'

T=$(mktemp -d)
printf '{: TuX TuX TuX TuX TuX TuX;\n' > "$T/six.tux"
printf '{: tux  tux;\n' > "$T/sep.tux"
printf '{: tux\n' > "$T/nosemi.tux"
printf 'tux;\n' > "$T/noopen.tux"
printf '{: tuQx;\n' > "$T/garbage.tux"
printf '{:;\n' > "$T/emptyline.tux"
printf '{: tux;\n{:;\n' > "$T/second.tux"
: > "$T/emptyfile.tux"
printf '{: tux;\n' > "$T/underflow.tux"
printf '{: Tuux  TuuUx   TUX;\n' > "$T/divzero.tux"
printf '{: TUuUUuuuUUX;\n' > "$T/badjump.tux"

expect_troll six "$T/six.tux"
expect_troll sep "$T/sep.tux"
expect_troll nosemi "$T/nosemi.tux"
expect_troll noopen "$T/noopen.tux"
expect_troll garbage "$T/garbage.tux"
expect_troll emptyline "$T/emptyline.tux"
expect_troll emptyline2 "$T/second.tux"
expect_troll emptyfile "$T/emptyfile.tux"
expect_troll underflow "$T/underflow.tux"
expect_troll divzero "$T/divzero.tux"
expect_troll badjump "$T/badjump.tux"

rm -rf "$T"
echo "OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
