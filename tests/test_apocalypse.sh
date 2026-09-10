#!/bin/sh
set -e
cd "$(dirname "$0")/.."

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

expect_troll_msg() {
    name=$1 pattern=$2
    shift 2
    out=$("$@" 2>&1 || true)
    if echo "$out" | grep -i -E -q "$pattern"; then
        pass=$((pass + 1))
        echo "  [PASS] $name (успешный троллинг)"
    else
        fail=$((fail + 1))
        echo "  [FAIL] $name: вывод не содержит [$pattern]. Получено: [$out]"
    fi
}

expect_exit_code() {
    name=$1 expected_code=$2
    shift 2
    set +e
    "$@" >/dev/null 2>&1
    actual_code=$?
    set -e
    if [ "$actual_code" -eq "$expected_code" ]; then
        pass=$((pass + 1))
        echo "  [PASS] $name (код возврата: $actual_code)"
    else
        fail=$((fail + 1))
        echo "  [FAIL] $name: ждали код $expected_code, получили $actual_code"
    fi
}

echo "=== Запуск тестов TuxPL 2.0.0 Apocalypse & Advanced VM ==="

TMP_DIR=$(mktemp -d -p "$(pwd)")
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

# 1. Проверка взаимоисключения основных режимов
expect_exit_code mode_exclusivity 2 ./tuxpl --CLASSIC --APOCALYPSE examples/hi.tux

# 2. Тест полиморфного анти-дизассемблера (--DISASM)
expect_troll_msg disasm_output "Polymorphic Anti-Disassembler" ./tuxpl --DISASM examples/self_modify.tux

# 3. Companion файл: сирота (TR_ORPHAN)
expect_troll_msg companion_orphan "companion|\.tu" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --APOCALYPSE --companion "$TMP_DIR/missing.tu" examples/apocalypse.tux

# 4. Companion файл: ересь / подделка контрольной суммы (TR_HERESY)
head -c 88 /dev/urandom > "$TMP_DIR/bad.tu"
expect_troll_msg companion_heresy "companion|спутник|ерес|TUX2" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --APOCALYPSE --companion "$TMP_DIR/bad.tu" examples/apocalypse.tux

# 5. Purgatory Self-Modifying Memory Execution
check self_modify "42" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --PURGATORY examples/self_modify.tux

# 6. Purgatory Dual PC & Code-as-Data
check code_as_data "0" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --PURGATORY examples/code_as_data.tux

# 7. Reversible Mode & OP_UNDO (успешный откат состояния памяти)
check reversible_undo "39112908095360031" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --PURGATORY --REVERSIBLE examples/undo.tux

# 8. OP_UNDO без флага --REVERSIBLE должен падать с TR_NO_HISTORY
expect_troll_msg undo_without_flag "истори|откат|времени" env TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1 ./tuxpl --PURGATORY examples/undo.tux

# 9. Детерминированная воспроизводимость 100% при фиксированном окружении
RUN1="$TMP_DIR/run1.log"
RUN2="$TMP_DIR/run2.log"
ENV_FIXED="TUX_TIME=1700000000 TUX_ENTROPY_SEED=42 TUX_GENOME=12345 TUX_CPU_TEMP=45.0 TUX_LUCKY=1 TUX_FORCE_ARCH=1 TUX_WAKE_UP=1"
env $ENV_FIXED ./tuxpl --APOCALYPSE --TRACE-STATE examples/apocalypse.tux > "$RUN1" 2>&1 || true
env $ENV_FIXED ./tuxpl --APOCALYPSE --TRACE-STATE examples/apocalypse.tux > "$RUN2" 2>&1 || true

if diff -u "$RUN1" "$RUN2" >/dev/null 2>&1; then
    pass=$((pass + 1))
    echo "  [PASS] apocalypse_deterministic_reproducibility (diff идентичен байт-в-байт)"
else
    fail=$((fail + 1))
    echo "  [FAIL] apocalypse_deterministic_reproducibility: трассы выполнения отличаются!"
fi

echo "======================================================="
echo "Apocalypse тесты: OK: $pass, FAIL: $fail"
if [ "$fail" -gt 0 ]; then
    exit 1
fi
