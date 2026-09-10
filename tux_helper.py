#!/usr/bin/env python3
"""
TuxPL Helper — интерактивный переводчик и генератор кода для TuxPL.
Поддерживает генерацию классического кода и Абсолютного Адского TuxPL (Cursed Mode)!
"""
import sys
import os

BANK_A = {
    "ADD": ("TuX", "Сложить два верхних числа"),
    "SUB": ("Tux", "Вычесть b из a (a - b)"),
    "MUL": ("TUx", "Умножить a * b"),
    "DIV": ("TUX", "Разделить a / b"),
    "DUP": ("tux", "Продублировать верхнее число"),
    "SWAP": ("tuX", "Поменять два верхних местами"),
    "POP": ("tUx", "Выкинуть верхнее число"),
    "PRINT_CHAR": ("tUX", "Напечатать символ (по ASCII коду)"),
}

BANK_B = {
    "PUSH": ("Tuu", "X", "Положить число N на стек"),
    "LOAD": ("Tuu", "x", "Взять значение из vars[N] на стек"),
    "STORE": ("TuU", "x", "Сохранить вершину стека в vars[N]"),
    "LOAD_IND": ("TuU", "X", "Прочитать из mem[адрес]"),
    "STORE_IND": ("TUu", "x", "Записать в mem[адрес] = значение"),
    "JMP": ("TUu", "X", "Прыгнуть на команду №N"),
    "JZ": ("TUU", "x", "Прыгнуть на №N, если на стеке 0"),
    "JNZ": ("TUU", "X", "Прыгнуть на №N, если на стеке НЕ 0"),
    "CMP": ("tuu", "X", "Сравнить a и b (-1, 0, 1)"),
    "PRINT_NUM": ("tUU", "x", "Напечатать число"),
    "INPUT_NUM": ("tUU", "X", "Считать число со stdin в vars[N]"),
}

def to_bits(n):
    if n == 0:
        return ""
    return bin(n)[2:].replace("0", "u").replace("1", "U")

OPCODE_EXT = {
    "FISH": 100,
    "CRAZY": 101,
    "DIR": 102,
    "PUSH_PC": 107,
    "SET_PC": 108,
    "SWAP_PC": 109,
    "ADD_PC": 110,
    "XOR_PC": 111,
    "CLONE": 112,
    "DECAY": 113,
    "WAKE": 114,
    "REINTERPRET": 115,
    "UNDO": 116,
    "PAY_TIME": 117,
    "NOP": 118,
}

def word(op, arg=0):
    op = op.upper()
    if op in BANK_A:
        return BANK_A[op][0]
    if op == "REGGET":
        b = f"{arg & 3:02b}".replace("0", "u").replace("1", "U")
        return "TuU" + b + "X"
    if op == "REGSET":
        b = f"{arg & 3:02b}".replace("0", "u").replace("1", "U")
        return "TUu" + b + "x"
    if op in OPCODE_EXT:
        code = OPCODE_EXT[op]
        b = bin(code)[2:].replace("0", "u").replace("1", "U")
        return "tuu" + b + "x"
    if op == "CAST":
        code = 103 + (arg & 3)
        b = bin(code)[2:].replace("0", "u").replace("1", "U")
        return "tuu" + b + "x"
    if op in BANK_B:
        prefix, suffix, _ = BANK_B[op]
        return prefix + to_bits(arg) + suffix
    raise ValueError(f"Неизвестная команда: {op}")

def count_u(w):
    return sum(1 for c in w if c in "Uu")

def sep(w):
    u = count_u(w)
    if u == 1: return " "
    if u == 2: return "  "
    if u == 3: return "   "
    return "\t"

def calc_tux_checksum(bit_len):
    p2 = 1
    temp = bit_len
    while temp > 0 and temp % 2 == 0:
        p2 *= 2
        temp //= 2
    p3 = 1
    temp = bit_len
    while temp > 0 and temp % 3 == 0:
        p3 *= 3
        temp //= 3
    if p3 > p2: return 'X'
    if p2 > p3: return 'U'
    return 'T'

def compile_commands(cmds):
    """Классическая компиляция (по 5 команд на строку)."""
    words = []
    for item in cmds:
        if isinstance(item, tuple) or isinstance(item, list):
            op, arg = item[0], item[1]
        else:
            op, arg = item, 0
        words.append(word(op, arg))

    lines = []
    for i in range(0, len(words), 5):
        chunk = words[i:i+5]
        line = "{: "
        for idx, w in enumerate(chunk):
            line += w
            if idx < len(chunk) - 1:
                line += sep(w)
            else:
                line += ";"
        lines.append(line)
    return "\n".join(lines)

def compile_cursed(cmds, is_purgatory=False):
    """
    Адская компиляция (Cursed Mode / Purgatory Mode):
    - Подключение библиотек из Tux/
    - Обязательный импорт TuuuuuuuuX для Purgatory
    - Сакральная C-образная обвязка со всеми символами
    - Динамический цикл 1-2-3-4-5
    - Контрольная буква T/U/X в конце каждой строки
    - Валидация Whitespace Parity (в Purgatory Mode)
    """
    words = []
    needed_libs = {"TuuuX", "TuuuuX"}
    if is_purgatory:
        needed_libs.add("TuuuuuuuuX")

    for item in cmds:
        if isinstance(item, tuple) or isinstance(item, list):
            op_name, arg = item[0], item[1]
        else:
            op_name, arg = item, 0
        w = word(op_name, arg)
        words.append(w)
        # Identify base opcode lib
        op_up = op_name.upper()
        if op_up in BANK_A:
            needed_libs.add(BANK_A[op_up][0])
        elif op_up == "REGGET":
            needed_libs.add("TuUX")
        elif op_up == "REGSET":
            needed_libs.add("TUux")
        elif op_up in ("FISH", "CRAZY", "DIR", "CAST") or op_up in OPCODE_EXT:
            needed_libs.add("tuux")
        elif op_up in BANK_B:
            needed_libs.add(BANK_B[op_up][0] + "X" if BANK_B[op_up][1] == "X" else BANK_B[op_up][0] + "x")

    # Pad commands to complete cycle if needed
    lines_body = []
    cmd_idx = 0
    line_idx = 1
    while cmd_idx < len(words):
        target = ((line_idx - 1) % 5) + 1
        chunk = words[cmd_idx:cmd_idx + target]
        cmd_idx += target
        
        # If last line has fewer commands than required by cycle, pad with PUSH 0 + POP (noop)
        while len(chunk) < target:
            chunk.append("TuuX") # PUSH 0
            needed_libs.add("TuuX")
            if len(chunk) < target:
                chunk.append("tUx") # POP
                needed_libs.add("tUx")

        pfx = "{:[~'Tux'~] "
        body = ""
        for i, w in enumerate(chunk):
            body += f"({w})"
            if i < len(chunk) - 1:
                body += sep(w)
            else:
                body += " "
        full_pfx = pfx + body
        chk = calc_tux_checksum(len(full_pfx.encode('utf-8')) * 8)
        line = f"{full_pfx}:{chk};!?}}"
        if is_purgatory:
            ws_count = sum(1 for c in line if c in " \t")
            if (ws_count % 2) != (line_idx % 2):
                line += " "
        lines_body.append(line)
        line_idx += 1

    imports = []
    for lib in sorted(list(needed_libs)):
        imports.append(f"<~\"TUX\"/['{lib}']~>!")

    header = "~\"TUX\"('tux')/[TUX](){:"
    closing = ":}}//?!~;"
    full_code = "\n".join(imports + [header] + lines_body + [closing]) + "\n"
    
    sz = len(full_code.encode('utf-8'))
    bits = sz * 8
    filename = f"{bits}.tux"
    return filename, full_code

def fnv1a_64(data: bytes) -> int:
    h = 0xCBF29CE484222325
    for b in data:
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h

def generate_companion_data(tux_content: bytes, chromosomes=None, regs=None) -> bytes:
    import struct
    if chromosomes is None:
        chromosomes = [
            0x123456789ABCDEF0,
            0x0FEDCBA987654321,
            0xCAFEBABE01234567,
            0x1337C0DED00DF00D
        ]
    if regs is None:
        regs = [0, 0, 0, 0]

    src_hash = fnv1a_64(tux_content)
    header = struct.pack("<4sHHQ", b"TUX2", 0x0200, 0, src_hash)
    chrom_bytes = struct.pack("<4Q", *chromosomes)
    regs_bytes = struct.pack("<4Q", *regs)
    payload = header + chrom_bytes + regs_bytes
    assert len(payload) == 0x50
    chk = fnv1a_64(payload)
    companion = payload + struct.pack("<Q", chk)
    assert len(companion) == 0x58
    return companion

def make_text(text):
    cmds = []
    for char in text:
        cmds.append(("PUSH", ord(char)))
        cmds.append(("PRINT_CHAR", 0))
    return compile_commands(cmds)

def make_cursed_text(text):
    cmds = []
    for char in text:
        cmds.append(("PUSH", ord(char)))
        cmds.append(("PRINT_CHAR", 0))
    return compile_cursed(cmds)

def make_purgatory_text(text):
    cmds = []
    for char in text:
        cmds.append(("PUSH", ord(char)))
        cmds.append(("PRINT_CHAR", 0))
    return compile_cursed(cmds, is_purgatory=True)

# ----------------------------------------------------------------------
# De Bruijn Mealy Machine & RNS-CRT Engine (TuxPL 2.0 Academic Core)
# ----------------------------------------------------------------------
DEBRUIJN_ALPHABET = "TtUuXx"
DEBRUIJN_NUM_STATES = 252
DEBRUIJN_DEFAULT_SEED = 0x5A
RNS_M = [7, 11, 13, 17, 19, 23]
RNS_C = [6374082, 676039, 1144066, 5249244, 782782, 646646]
RNS_MODULUS_M = 7436429

def debruijn_char_to_idx(c):
    return DEBRUIJN_ALPHABET.index(c) if c in DEBRUIJN_ALPHABET else -1

def debruijn_transition(q, c):
    idx = debruijn_char_to_idx(c)
    if idx < 0:
        return q
    return (q * 6 + idx) % DEBRUIJN_NUM_STATES

def debruijn_emit_opcode(q, c):
    idx = debruijn_char_to_idx(c)
    if idx < 0:
        return 0xFF
    return (q ^ (idx * 7)) % 42

def debruijn_encode_fixed3(start_state, target_opcode):
    if not (0 <= target_opcode < 42):
        raise ValueError(f"Target opcode must be 0..41, got {target_opcode}")
    for c0 in DEBRUIJN_ALPHABET:
        q1 = debruijn_transition(start_state, c0)
        for c1 in DEBRUIJN_ALPHABET:
            q2 = debruijn_transition(q1, c1)
            for c2 in DEBRUIJN_ALPHABET:
                if debruijn_emit_opcode(q2, c2) == target_opcode:
                    return c0 + c1 + c2
    raise RuntimeError("No 3-step path found (mathematically impossible)")

def debruijn_encode_shortest(start_state, target_opcode):
    if not (0 <= target_opcode < 42):
        raise ValueError(f"Target opcode must be 0..41, got {target_opcode}")
    for c0 in DEBRUIJN_ALPHABET:
        if debruijn_emit_opcode(start_state, c0) == target_opcode:
            return c0
    for c0 in DEBRUIJN_ALPHABET:
        q1 = debruijn_transition(start_state, c0)
        for c1 in DEBRUIJN_ALPHABET:
            if debruijn_emit_opcode(q1, c1) == target_opcode:
                return c0 + c1
    return debruijn_encode_fixed3(start_state, target_opcode)

def debruijn_resolve_sequence(start_state, seq):
    q = start_state
    op = 0xFF
    for c in seq:
        op = debruijn_emit_opcode(q, c)
        q = debruijn_transition(q, c)
    return op, q

def rns_decompose(val):
    if not (0 <= val < RNS_MODULUS_M):
        raise ValueError(f"Value must be 0..{RNS_MODULUS_M - 1}, got {val}")
    return [val % m for m in RNS_M]

def rns_reconstruct(residues):
    total = sum(r * c for r, c in zip(residues, RNS_C)) % RNS_MODULUS_M
    return total

def rns_encode_operand(val):
    if not (0 <= val < RNS_MODULUS_M):
        raise ValueError(f"Value must be 0..{RNS_MODULUS_M - 1}, got {val}")
    res = []
    for m in RNS_M:
        r = val % m
        res.append(DEBRUIJN_ALPHABET[r // 6])
        res.append(DEBRUIJN_ALPHABET[r % 6])
    return "".join(res)

def rns_decode_operand(seq):
    if len(seq) < 12:
        raise ValueError(f"RNS operand sequence must be at least 12 chars, got {len(seq)}")
    residues = []
    for i in range(6):
        d0 = debruijn_char_to_idx(seq[2 * i])
        d1 = debruijn_char_to_idx(seq[2 * i + 1])
        if d0 < 0 or d1 < 0:
            raise ValueError(f"Invalid characters at residue {i}: {seq[2*i:2*i+2]}")
        residues.append((d0 * 6 + d1) % RNS_M[i])
    return rns_reconstruct(residues)

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "purgatory":
        msg = " ".join(sys.argv[2:]) if len(sys.argv) > 2 else "Hi"
        fname, code = make_purgatory_text(msg)
        print(f"# Сгенерирован Purgatory TuxPL файл: {fname}")
        with open(fname, "w") as f:
            f.write(code)
        print(f"# Сохранено в {fname} (размер: {len(code)} байт = {len(code)*8} бит)")
        print(code)
    elif len(sys.argv) > 1 and sys.argv[1] == "cursed":
        msg = " ".join(sys.argv[2:]) if len(sys.argv) > 2 else "Hi"
        fname, code = make_cursed_text(msg)
        print(f"# Сгенерирован Cursed TuxPL файл: {fname}")
        with open(fname, "w") as f:
            f.write(code)
        print(f"# Сохранено в {fname} (размер: {len(code)} байт = {len(code)*8} бит)")
        print(code)
    elif len(sys.argv) > 1 and sys.argv[1] == "companion":
        if len(sys.argv) < 3:
            print("Укажите путь к .tux файлу: python3 tux_helper.py companion <file.tux>")
            sys.exit(1)
        src_path = sys.argv[2]
        with open(src_path, "rb") as f:
            content = f.read()
        tu_data = generate_companion_data(content)
        tu_path = src_path[:-4] + ".tu" if src_path.endswith(".tux") else src_path + ".tu"
        with open(tu_path, "wb") as f:
            f.write(tu_data)
        print(f"# Сгенерирован валидный Companion файл TuxPL 2.0: {tu_path} ({len(tu_data)} байт)")
    elif len(sys.argv) > 1 and sys.argv[1] == "debruijn":
        op = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        q0 = int(sys.argv[3], 0) if len(sys.argv) > 3 else DEBRUIJN_DEFAULT_SEED
        seq = debruijn_encode_fixed3(q0, op)
        shortest = debruijn_encode_shortest(q0, op)
        res_op, end_q = debruijn_resolve_sequence(q0, seq)
        print(f"De Bruijn Opcode {op} (start q0={hex(q0)}):")
        print(f"  Fixed-3:   {seq}  --> resolves to opcode {res_op} (end q={hex(end_q)})")
        print(f"  Shortest:  {shortest}")
    elif len(sys.argv) > 1 and sys.argv[1] == "rns-enc":
        val = int(sys.argv[2]) if len(sys.argv) > 2 else 42
        enc = rns_encode_operand(val)
        print(f"RNS-CRT Encode {val} (M={RNS_MODULUS_M}):")
        print(f"  12-char suffix: {enc}")
        print(f"  Decoded back:   {rns_decode_operand(enc)}")
    elif len(sys.argv) > 1 and sys.argv[1] == "rns-dec":
        seq = sys.argv[2] if len(sys.argv) > 2 else "TTTTTTTTTTTT"
        dec = rns_decode_operand(seq)
        print(f"RNS-CRT Decode {seq}:")
        print(f"  Decoded value: {dec}")
    elif len(sys.argv) > 1 and sys.argv[1] == "text":
        msg = " ".join(sys.argv[2:]) if len(sys.argv) > 2 else "Hi!"
        print(f"# Классический код TuxPL для текста: {msg}")
        print(make_text(msg))
    elif len(sys.argv) > 1 and sys.argv[1] == "word":
        op = sys.argv[2]
        arg = int(sys.argv[3]) if len(sys.argv) > 3 else 0
        w = word(op, arg)
        print(f"{op} {arg}  -->  {w}  (U-букв: {count_u(w)}, разделитель: {repr(sep(w))})")
    elif len(sys.argv) > 1 and sys.argv[1] == "opcodes":
        print("Поддерживаемые опкоды TuxPL 2.0:")
        all_ops = list(BANK_A.keys()) + list(BANK_B.keys()) + list(OPCODE_EXT.keys()) + ["REGGET", "REGSET", "CAST"]
        for o in sorted(all_ops):
            print(f"  {o}")
    else:
        print("Использование:")
        print("  python3 tux_helper.py purgatory \"Hi\"      # создать файл режима Purgatory")
        print("  python3 tux_helper.py cursed \"Hi\"         # создать адский <bits>.tux файл")
        print("  python3 tux_helper.py companion <f.tux>   # создать .tu companion файл")
        print("  python3 tux_helper.py debruijn <op> [q0]  # синтез опкода через автомат де Брейна")
        print("  python3 tux_helper.py rns-enc <number>    # кодирование операнда в RNS-12")
        print("  python3 tux_helper.py rns-dec <12-chars>  # декодирование RNS-12 операнда")
        print("  python3 tux_helper.py word PUSH 42        # узнать слово TuxPL для PUSH 42")
        print("  python3 tux_helper.py text \"Hello!\"       # классический код для --PLS")
        print("  python3 tux_helper.py opcodes             # список всех опкодов TuxPL 2.0")
