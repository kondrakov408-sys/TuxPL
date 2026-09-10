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

expect_troll() {
    name=$1
    shift
    if "$@" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "  [FAIL] $name: ждали троллинг, но команда прошла"
    else
        pass=$((pass + 1))
        echo "  [PASS] $name (успешный троллинг)"
    fi
}

echo "=== Запуск хардкорных тестов TuxPL Purgatory Mode ==="

TMP_DIR=$(mktemp -d -p "$(pwd)")
ln -s "$(pwd)/Tux" "$TMP_DIR/Tux"

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

# 1. Генерация тестовых файлов через tux_helper.py в $TMP_DIR
python3 -c "
import os
from tux_helper import compile_cursed, make_purgatory_text

tmp = '$TMP_DIR'

# 1. Baseline 'Hi'
f_hi, c_hi = make_purgatory_text('Hi')
with open(os.path.join(tmp, f_hi), 'w') as f: f.write(c_hi)

# 2. Avalanche (>7 consecutive pushes)
f_ava, c_ava = compile_cursed([('PUSH', i) for i in range(1, 9)], is_purgatory=True)
with open(os.path.join(tmp, f_ava), 'w') as f: f.write(c_ava)

# 3. Borrow Checker (Move Semantics: load without store)
f_bor, c_bor = compile_cursed([('PUSH', 42), ('STORE', 0), ('LOAD', 0), ('LOAD', 0)], is_purgatory=True)
with open(os.path.join(tmp, f_bor), 'w') as f: f.write(c_bor)

# 4. Hardware Registers (Tu RAX accumulator)
f_reg, c_reg = compile_cursed([('PUSH', 20), ('PUSH', 22), ('ADD', 0), ('REGGET', 0), ('PRINT_NUM', 0)], is_purgatory=True)
with open(os.path.join(tmp, f_reg), 'w') as f: f.write(c_reg)

# 5. CRAZY operation (Malbolge balanced ternary)
f_crz, c_crz = compile_cursed([('PUSH', 1), ('PUSH', 1), ('CRAZY', 0), ('PRINT_NUM', 0)], is_purgatory=True)
with open(os.path.join(tmp, f_crz), 'w') as f: f.write(c_crz)

# 6. Type Mismatch (i8 + i16)
f_tbad, c_tbad = compile_cursed([('PUSH', 10), ('PUSH', 1000), ('ADD', 0)], is_purgatory=True)
with open(os.path.join(tmp, f_tbad), 'w') as f: f.write(c_tbad)

# 7. Type Cast (i8 -> cast i16 + i16)
f_tgood, c_tgood = compile_cursed([('PUSH', 10), ('CAST', 1), ('PUSH', 1000), ('ADD', 0), ('PRINT_NUM', 0)], is_purgatory=True)
with open(os.path.join(tmp, f_tgood), 'w') as f: f.write(c_tgood)

# 8. Fish Starvation (>100 ops without food)
cmds_starve = []
for _ in range(60):
    cmds_starve.extend([('PUSH', 0), ('POP', 0)])
f_starve, c_starve = compile_cursed(cmds_starve, is_purgatory=True)
with open(os.path.join(tmp, f_starve), 'w') as f: f.write(c_starve)

# 9. Fish Replenished
cmds_repl = [('FISH', 500)] + cmds_starve
f_repl, c_repl = compile_cursed(cmds_repl, is_purgatory=True)
with open(os.path.join(tmp, f_repl), 'w') as f: f.write(c_repl)

# 10. Whitespace Tampered
lines = c_tgood.splitlines(keepends=True)
for i, l in enumerate(lines):
    if '{:[~' in l:
        lines[i] = l.replace('} \n', '}\n')
        break
c_tampered = ''.join(lines)
f_tampered = str(len(c_tampered.encode('utf-8')) * 8) + '.tux'
with open(os.path.join(tmp, f_tampered), 'w') as f: f.write(c_tampered)

# Save filenames for shell script
with open(os.path.join(tmp, 'manifest.sh'), 'w') as mf:
    mf.write(f'F_HI={f_hi}\n')
    mf.write(f'F_AVA={f_ava}\n')
    mf.write(f'F_BOR={f_bor}\n')
    mf.write(f'F_REG={f_reg}\n')
    mf.write(f'F_CRZ={f_crz}\n')
    mf.write(f'F_TBAD={f_tbad}\n')
    mf.write(f'F_TGOOD={f_tgood}\n')
    mf.write(f'F_STARVE={f_starve}\n')
    mf.write(f'F_REPL={f_repl}\n')
    mf.write(f'F_TAMP={f_tampered}\n')
"

. "$TMP_DIR/manifest.sh"

# Тест 1: Базовый запуск программы в режиме Unified VM
check purgatory_hi "Hi" ./tuxpl "$TMP_DIR/$F_HI"

# Тест 2: Гарантированное падение по PANIC_BUDGET_EXHAUSTION при нулевом газе
set +e
env TUX_GAS_BUDGET=0 ./tuxpl "$TMP_DIR/$F_HI" >/dev/null 2>&1
rc=$?
set -e
if [ "$rc" -eq 3 ]; then
    pass=$((pass + 1))
    echo "  [PASS] zero_gas_budget_exhaustion (код возврата: 3, PANIC_BUDGET_EXHAUSTION)"
else
    fail=$((fail + 1))
    echo "  [FAIL] zero_gas_budget_exhaustion: ожидали код 3, получили $rc"
fi

# Тест 3: Запуск через академический первичный флаг --UNIFIED-VM
check unified_vm_mode "Hi" ./tuxpl --UNIFIED-VM "$TMP_DIR/$F_HI"

# Тест 4: Обрушение стека от гравитации (>7 pushes)
expect_troll stack_avalanche ./tuxpl "$TMP_DIR/$F_AVA"

# Тест 5: Borrow Checker (Use-after-move)
expect_troll use_after_move ./tuxpl "$TMP_DIR/$F_BOR"

# Тест 6: Аппаратные регистры (Tu = RAX аккумулятор)
check hw_registers "42" ./tuxpl "$TMP_DIR/$F_REG"

# Тест 7: Троичная CRAZY операция (Malbolge)
check crazy_op "29523" ./tuxpl "$TMP_DIR/$F_CRZ"

# Тест 8: Несовпадение строгих типов
expect_troll type_mismatch ./tuxpl "$TMP_DIR/$F_TBAD"

# Тест 9: Явный кастинг типа (CAST)
check type_cast "1010" ./tuxpl "$TMP_DIR/$F_TGOOD"

# Тест 10: Исчерпание метаболического бюджета газа
expect_troll gas_starvation ./tuxpl "$TMP_DIR/$F_STARVE"

# Тест 11: Восполнение метаболического газа (OP_FISH / OP_REPLENISH_GAS)
check gas_replenished "" ./tuxpl "$TMP_DIR/$F_REPL"

# Тест 12: Нарушение невидимой Whitespace-четности
expect_troll whitespace_tampered ./tuxpl "$TMP_DIR/$F_TAMP"

echo "======================================================="
echo "Хардкорные тесты: OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
