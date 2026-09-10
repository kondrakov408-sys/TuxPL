#!/bin/sh
set -e
cd "$(dirname "$0")/.."

echo "=== Rule 1: Running GBSV Galois/Rijndael Cross-Test (Python vs C) ==="
python3 tests/test_py_gbsv_cross.py

echo "=== Running TuxPL 2.0.0 Monolithic Core Basic Test Suite ==="

pass=0
fail=0

check() {
    name=$1 expected=$2
    shift 2
    got=$("$@" 2>/dev/null)
    if [ "$got" = "$expected" ]; then
        pass=$((pass + 1))
        echo "  [PASS] $name"
    else
        fail=$((fail + 1))
        echo "  [FAIL] $name: got [$got] want [$expected]"
    fi
}

expect_troll() {
    name=$1 file=$2
    if ./tuxpl --no-shadow "$file" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "  [FAIL] $name: ждали троллинг, программа прошла"
    else
        pass=$((pass + 1))
        echo "  [PASS] $name (успешный троллинг)"
    fi
}

# 1. Тест: запуск классического файла без заголовков/GBSV должен отвергаться
if ./tuxpl examples/hi.tux >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "  [FAIL] legacy_rejection: ждали отказ на классическом файле"
else
    pass=$((pass + 1))
    echo "  [PASS] legacy_rejection (классический формат отвергнут)"
fi

# 2. Позитивные тесты исполнения
check hi_zero_flag "Hi" ./tuxpl examples/2088.tux
check hi_no_shadow "Hi" ./tuxpl --no-shadow examples/2088.tux

# 3. Синтаксические и структурные проверки отказов
T=$(mktemp -d -p "$(pwd)")
cleanup() {
    rm -rf "$T"
}
trap cleanup EXIT INT TERM

# Тест сироты: без .tu файла рантайм паникует PANIC_COMPANION_ORPHAN
cp examples/2088.tux "$T/2088.tux"
if ./tuxpl "$T/2088.tux" >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "  [FAIL] orphan_check: ждали PANIC_COMPANION_ORPHAN"
else
    pass=$((pass + 1))
    echo "  [PASS] orphan_check (сирота отвергнута без companion)"
fi

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

echo "======================================================="
echo "Basic Tests: OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
