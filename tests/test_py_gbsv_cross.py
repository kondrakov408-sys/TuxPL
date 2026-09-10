#!/usr/bin/env python3
"""
TuxPL 2.0.0 — Cross-Verification of Galois/Rijndael & GBSV Engine (Python vs C)
Verifies bit-for-bit identity of GbsvSignature across hundreds of random & structured vectors.
"""
import sys
import os
import ctypes
import subprocess

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)

from tux_helper import gbsv_calculate

class GbsvSignatureC(ctypes.Structure):
    _fields_ = [
        ('b64', ctypes.c_char),
        ('gf', ctypes.c_char),
        ('tux', ctypes.c_char),
        ('zeta', ctypes.c_char)
    ]

def compile_libgbsv():
    so_path = os.path.join(ROOT_DIR, "tests", "libgbsv_cross.so")
    cmd = [
        "cc", "-std=c99", "-shared", "-fPIC", "-O2",
        "-o", so_path,
        os.path.join(ROOT_DIR, "src", "gbsv.c"),
        os.path.join(ROOT_DIR, "src", "diag.c")
    ]
    subprocess.check_call(cmd)
    return so_path

def main():
    print("=== Cross-testing GBSV Galois/Rijndael Arithmetic (Python vs C) ===")
    so_path = compile_libgbsv()
    try:
        lib = ctypes.CDLL(so_path)
        lib.gbsv_calculate.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t]
        lib.gbsv_calculate.restype = GbsvSignatureC

        import random
        random.seed(0x1337BEEF)

        # 1. Deterministic known vectors
        test_vectors = [
            b"{:[~'Tux'~] (tUX) ",
            b"{:[~'Tux'~] (tUX) (TuuUuuUUuX) ",
            b"{:[~'Tux'~] (TuuUUuuUuux)\t(tuuUUuuUuux)\t(tuuUUuuUuux) ",
            b"",
            b"A",
            b"Hello TuxPL 2.0.0 Monolithic Kernel Flat Torus!",
            bytes(range(256)),
            b"\x00" * 32,
            b"\xFF" * 32,
            b"\xAA\x55" * 16,
        ]

        prev_line = None
        passed = 0

        for idx, vec in enumerate(test_vectors):
            py_sig = gbsv_calculate(vec, prev_line)
            c_res = lib.gbsv_calculate(vec, len(vec), prev_line, len(prev_line) if prev_line else 0)
            c_sig = (
                c_res.b64.decode('latin1'),
                c_res.gf.decode('latin1'),
                c_res.tux.decode('latin1'),
                c_res.zeta.decode('latin1')
            )
            assert py_sig == c_sig, f"Deterministic vector {idx} mismatch: Python {py_sig} != C {c_sig}"
            passed += 1
            prev_line = vec

        # 2. 1000 Random stress vectors
        for i in range(1000):
            curr_len = random.randint(1, 150)
            curr = bytes(random.randint(0, 255) for _ in range(curr_len))
            py_sig = gbsv_calculate(curr, prev_line)
            c_res = lib.gbsv_calculate(curr, len(curr), prev_line, len(prev_line) if prev_line else 0)
            c_sig = (
                c_res.b64.decode('latin1'),
                c_res.gf.decode('latin1'),
                c_res.tux.decode('latin1'),
                c_res.zeta.decode('latin1')
            )
            if py_sig != c_sig:
                print(f"FAILED on iteration {i}!")
                print(f"  Vector: {curr.hex()}")
                print(f"  Python: {py_sig}")
                print(f"  C:      {c_sig}")
                sys.exit(1)
            passed += 1
            if random.random() < 0.8:
                prev_line = curr
            else:
                prev_line = None

        print(f"  [PASS] All {passed} vectors passed with bit-for-bit parity! (0 drift in F_2^8/popcount/p-adic)")
        print("=== Cross-Verification Successful! ===")
    finally:
        if os.path.exists(so_path):
            os.remove(so_path)

if __name__ == "__main__":
    main()
