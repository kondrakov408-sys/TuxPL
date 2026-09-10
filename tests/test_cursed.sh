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

expect_troll_cursed() {
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

echo "=== Запуск адских тестов TuxPL Cursed / GBSV Mode ==="

TMP_DIR=$(mktemp -d -p "$(pwd)")
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

# 1. Запуск эталонной адской программы с GBSV и companion (.tu)
check cursed_hi "Hi" ./tuxpl examples/2088.tux

# 2. Тест: имя файла не равно числу его бит
cp examples/2088.tux "$TMP_DIR/wrong_name.tux"
expect_troll_cursed name_mismatch ./tuxpl "$TMP_DIR/wrong_name.tux"

# 3. Тест: отказ при нарушении спецификации GBSV (старый терминатор :U;!?})
expect_troll_cursed gbsv_syntax_legacy ./tuxpl examples/1840.tux

# 4. Тест: нарушение синтаксиса скобок (убрали тильду)
sed 's/~"TUX"/"TUX"/g' examples/2088.tux > "$TMP_DIR/bad_syntax.tux"
BS_SZ=$(wc -c < "$TMP_DIR/bad_syntax.tux")
BS_BITS=$((BS_SZ * 8))
mv "$TMP_DIR/bad_syntax.tux" "$TMP_DIR/${BS_BITS}.tux"
expect_troll_cursed bad_syntax ./tuxpl "$TMP_DIR/${BS_BITS}.tux"

# 5. Тест: неверная GBSV сигнатура
cp examples/2088.tux "$TMP_DIR/2088.tux"
cp examples/2088.tu "$TMP_DIR/2088.tu"
sed -i 's/:\[f|_|U\];&/:[A|_|U\];&/g' "$TMP_DIR/2088.tux"
expect_troll_cursed bad_checksum ./tuxpl "$TMP_DIR/2088.tux"

# 6. Тест: пропущенная библиотека
sed '/TuuX/d' examples/2088.tux > "$TMP_DIR/no_lib.tux"
NL_SZ=$(wc -c < "$TMP_DIR/no_lib.tux")
NL_BITS=$((NL_SZ * 8))
mv "$TMP_DIR/no_lib.tux" "$TMP_DIR/${NL_BITS}.tux"
expect_troll_cursed missing_lib ./tuxpl "$TMP_DIR/${NL_BITS}.tux"

# 7. Тест: нарушение цикла строк 1-2-3-4-5
python3 -c "
from tux_helper import compile_cursed
_, code = compile_cursed([('PUSH', 1), ('PUSH', 2), ('PUSH', 3)])
lines = code.splitlines(keepends=True)
# Break cycle: duplicate line 1
for i, l in enumerate(lines):
    if l.startswith('{:[~'):
        lines.insert(i+1, l)
        break
bad_c = ''.join(lines)
b_sz = len(bad_c.encode('utf-8')) * 8
with open('$TMP_DIR/' + str(b_sz) + '.tux', 'w') as f:
    f.write(bad_c)
with open('$TMP_DIR/cyc_bits.txt', 'w') as f:
    f.write(str(b_sz))
"
CYC_BITS=$(cat "$TMP_DIR/cyc_bits.txt")
expect_troll_cursed line_cycle ./tuxpl "$TMP_DIR/${CYC_BITS}.tux"

# 8. Тест безопасного отказа: деление на ноль завершается с кодом 1 без удаления файла
python3 -c "
from tux_helper import compile_cursed, generate_companion_data
f_div, c_div = compile_cursed([('PUSH', 10), ('PUSH', 0), ('DIV', 0)])
with open('$TMP_DIR/' + f_div, 'w') as f: f.write(c_div)
tu = generate_companion_data(c_div.encode('utf-8'))
with open('$TMP_DIR/' + f_div[:-4] + '.tu', 'wb') as f: f.write(tu)
with open('$TMP_DIR/div_file.txt', 'w') as f: f.write(f_div)
"
DIV_FILE=$(cat "$TMP_DIR/div_file.txt")
set +e
./tuxpl "$TMP_DIR/$DIV_FILE" >/dev/null 2>&1
div_rc=$?
set -e
if [ "$div_rc" -eq 1 ]; then
    if [ -f "$TMP_DIR/$DIV_FILE" ]; then
        pass=$((pass + 1))
        echo "  [PASS] safe_divzero_failure (код 1, файл сохранен)"
    else
        fail=$((fail + 1))
        echo "  [FAIL] safe_divzero_failure: файл был удален!"
    fi
else
    fail=$((fail + 1))
    echo "  [FAIL] safe_divzero_failure: ожидали код 1, получили $div_rc"
fi

echo "======================================================="
echo "Cursed / GBSV Tests: OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
