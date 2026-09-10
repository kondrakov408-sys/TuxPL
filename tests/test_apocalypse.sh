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

echo "=== Запуск тестов TuxPL 2.0.0 Monolithic Adversarial Core ==="

TMP_DIR=$(mktemp -d -p "$(pwd)")
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

# 1. Проверка отвержения легаси-флагов (--CLASSIC / --APOCALYPSE / --PURGATORY)
expect_exit_code legacy_flag_classic 2 ./tuxpl --CLASSIC examples/2088.tux
expect_exit_code legacy_flag_purgatory 2 ./tuxpl --PURGATORY examples/2088.tux
expect_exit_code legacy_flag_apocalypse 2 ./tuxpl --APOCALYPSE examples/2088.tux

# 2. Тест полиморфного анти-дизассемблера (--DISASM)
expect_troll_msg disasm_output "Polymorphic Anti-Disassembler" ./tuxpl --DISASM examples/2088.tux

# 3. Companion файл: сирота (PANIC_COMPANION_ORPHAN)
expect_troll_msg companion_orphan "PANIC_COMPANION_ORPHAN|companion" ./tuxpl --companion "$TMP_DIR/missing.tu" examples/2088.tux

# 4. Companion файл: ересь / подделка контрольной суммы (PANIC_COMPANION_HERESY)
head -c 88 /dev/urandom > "$TMP_DIR/bad.tu"
expect_troll_msg companion_heresy "PANIC_COMPANION_HERESY|TUX2" ./tuxpl --companion "$TMP_DIR/bad.tu" examples/2088.tux

# 5. Исполнение Cyber-Reactor (141248.tux) по умолчанию без флагов (Zero-Flag Monolithic Torus)
expect_troll_msg cyber_reactor_zero_flag "CORE STATUS: OPERATIONAL|STABILIZED" ./tuxpl examples/141248.tux

# 6. Исполнение Cyber-Reactor с флагом --no-shadow
expect_troll_msg cyber_reactor_no_shadow "CORE STATUS: OPERATIONAL|STABILIZED" ./tuxpl --no-shadow examples/141248.tux

# 7. Генерация тестовой программы для обратимого исполнения (OP_UNDO)
python3 -c "
import os
from tux_helper import compile_cursed, generate_companion_data
cmds_undo = [
    ('PUSH', 100),
    ('PUSH', 777),
    ('STORE_IND', 0),
    ('UNDO', 0),
    ('PUSH', 100),
    ('LOAD_IND', 0),
    ('PRINT_NUM', 0),
]
fn, code = compile_cursed(cmds_undo, is_purgatory=True)
tmp_fn = os.path.join('$TMP_DIR', fn)
with open(tmp_fn, 'w') as f: f.write(code)
tu = generate_companion_data(code.encode('utf-8'))
with open(tmp_fn[:-4] + '.tu', 'wb') as f: f.write(tu)
with open(os.path.join('$TMP_DIR', 'undo_file.txt'), 'w') as f: f.write(fn)
"
UNDO_FILE=$(cat "$TMP_DIR/undo_file.txt")

# 8. Reversible Mode & OP_UNDO (успешный откат состояния памяти до 872857)
check reversible_undo "872857" ./tuxpl --no-shadow --reversible "$TMP_DIR/$UNDO_FILE"

# 9. OP_UNDO без флага --reversible должен падать с PANIC_SPEC_NO_HISTORY
expect_troll_msg undo_without_flag "PANIC_SPEC_NO_HISTORY|reversible snapshot" ./tuxpl "$TMP_DIR/$UNDO_FILE"

# 10. Детерминированная воспроизводимость 100% при фиксированном окружении
RUN1="$TMP_DIR/run1.log"
RUN2="$TMP_DIR/run2.log"
ENV_FIXED="TUX_ENTROPY_SEED=42 TUX_GENOME=12345"
env $ENV_FIXED ./tuxpl --trace-state examples/141248.tux > "$RUN1" 2>&1 || true
env $ENV_FIXED ./tuxpl --trace-state examples/141248.tux > "$RUN2" 2>&1 || true

if diff -u "$RUN1" "$RUN2" >/dev/null 2>&1; then
    pass=$((pass + 1))
    echo "  [PASS] deterministic_reproducibility (diff идентичен байт-в-байт)"
else
    fail=$((fail + 1))
    echo "  [FAIL] deterministic_reproducibility: трассы выполнения отличаются!"
fi

echo "======================================================="
echo "Monolithic Core тесты: OK: $pass, FAIL: $fail"
if [ "$fail" -gt 0 ]; then
    exit 1
fi
