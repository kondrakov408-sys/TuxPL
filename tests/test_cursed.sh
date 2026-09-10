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
    else
        fail=$((fail + 1))
        echo "FAIL $name: got [$got] want [$expected]"
    fi
}

expect_troll_cursed() {
    name=$1
    shift
    if "$@" >/dev/null 2>&1; then
        fail=$((fail + 1))
        echo "FAIL $name: ждали троллинг, но команда прошла"
    else
        pass=$((pass + 1))
    fi
}

echo "=== Запуск адских тестов TuxPL Cursed Mode ==="

# 1. Запуск эталонной адской программы
check cursed_hi "Hi" ./tuxpl examples/1840.tux

# 2. Тест: имя файла не равно числу его бит
TMP_DIR=$(mktemp -d)
cp examples/1840.tux "$TMP_DIR/wrong_name.tux"
expect_troll_cursed name_mismatch ./tuxpl "$TMP_DIR/wrong_name.tux"

# 3. Тест: отказ при нарушении спецификации GBSV в строгом режиме (--gbsv)
expect_troll_cursed gbsv_spec_failure ./tuxpl --gbsv examples/1840.tux

# 4. Тест: нарушение синтаксиса скобок (убрали тильду)
sed 's/~"TUX"/"TUX"/g' examples/1840.tux > "$TMP_DIR/bad_syntax.tux"
# rename to match its bits
BS_SZ=$(wc -c < "$TMP_DIR/bad_syntax.tux")
BS_BITS=$((BS_SZ * 8))
mv "$TMP_DIR/bad_syntax.tux" "$TMP_DIR/${BS_BITS}.tux"
expect_troll_cursed bad_syntax ./tuxpl "$TMP_DIR/${BS_BITS}.tux"

# 5. Тест: неверная контрольная буква T/U/X
sed 's/:U;!?}/:X;!?}/g' examples/1840.tux > "$TMP_DIR/1840.tux"
expect_troll_cursed bad_checksum ./tuxpl "$TMP_DIR/1840.tux"

# 6. Тест: пропущенная библиотека
sed '/TuuX/d' examples/1840.tux > "$TMP_DIR/no_lib.tux"
NL_SZ=$(wc -c < "$TMP_DIR/no_lib.tux")
NL_BITS=$((NL_SZ * 8))
mv "$TMP_DIR/no_lib.tux" "$TMP_DIR/${NL_BITS}.tux"
expect_troll_cursed missing_lib ./tuxpl "$TMP_DIR/${NL_BITS}.tux"

# 7. Тест: нарушение цикла строк 1-2-3-4-5
cat << 'EOF' > "$TMP_DIR/cycle.tux"
<~"TUX"/['TuuuX']~>!
<~"TUX"/['TuuuuX']~>?
<~"TUX"/['tUX']~>?
~"TUX"('tux')/[TUX](){:
{:[~'Tux'~] (tUX) (tUX) :U;!?}
:}}//?!~;
EOF
CYC_SZ=$(wc -c < "$TMP_DIR/cycle.tux")
CYC_BITS=$((CYC_SZ * 8))
mv "$TMP_DIR/cycle.tux" "$TMP_DIR/${CYC_BITS}.tux"
expect_troll_cursed line_cycle ./tuxpl "$TMP_DIR/${CYC_BITS}.tux"

# 8. Тест безопасного отказа: деление на ноль завершается с кодом 1 без удаления файла
printf '{: Tuux  TuuUx   TUX;\n' > "$TMP_DIR/victim.tux"
if ./tuxpl --CLASSIC "$TMP_DIR/victim.tux" >/dev/null 2>&1; then
    fail=$((fail + 1))
    echo "FAIL safe_failure: ожидался сбой деления на ноль"
else
    if [ -f "$TMP_DIR/victim.tux" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL safe_failure: файл был удален!"
    fi
fi

rm -rf "$TMP_DIR"

echo "Cursed Tests: OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
