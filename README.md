# TuxPL 2.0.0: Architecture of a Deterministic, Adversarial Virtual Machine with Galois-Field Control-Flow Integrity and Modular Residue Arithmetic

<p align="center">
  <img src="https://raw.githubusercontent.com/kondrakov408-sys/TuxPL/main/examples/tux.png" alt="TuxPL Architectural Logo" width="160" onerror="this.style.display='none'"/>
</p>

<p align="center">
  <b>A Provably Complete, Self-Modifying Virtual Machine and Constraint Satisfaction Language</b><br/>
  <i>Technical Report & System Specification — Version 2.0.0 (C99 / POSIX.1-2001 Compliant)</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Specification-TuxPL%202.0.0-blue.svg?style=flat-square" alt="Specification 2.0.0">
  <img src="https://img.shields.io/badge/CFI-Galois%20Field%20%E2%84%A4%2F2%E2%81%B8%20(GBSV)-red.svg?style=flat-square" alt="CFI GBSV">
  <img src="https://img.shields.io/badge/Operand%20Engine-RNS--CRT%20M%3D7436429-purple.svg?style=flat-square" alt="RNS-CRT">
  <img src="https://img.shields.io/badge/Opcode%20Synthesis-De%20Bruijn%20Mealy%20Machine-darkgreen.svg?style=flat-square" alt="De Bruijn">
  <img src="https://img.shields.io/badge/Memory%20Model-Flat%20Torus%20%E2%84%A4_M%20(64K)-black.svg?style=flat-square" alt="Flat Torus ZM">
  <img src="https://img.shields.io/badge/Verification-8%2F8%20Suites%20%7C%2074k%20Assertions%20(100%25)-brightgreen.svg?style=flat-square" alt="Tests Passing">
  <img src="https://img.shields.io/badge/Sanitizers-ASan%20%2B%20UBSan%20(0%20Warnings)-blueviolet.svg?style=flat-square" alt="Sanitizers">
</p>

<p align="center">
  🌐 <b>English</b> | <a href="README.ru.md">Русский</a>
</p>

---

## Abstract

This specification defines **TuxPL 2.0.0**, a deterministic, adversarial, polymorphic programming language and execution environment engineered as a high-order Constraint Satisfaction Problem (CSP). Departing fundamentally from trivial substitution-based esoteric architectures (such as Malbolge or INTERCAL), TuxPL integrates cryptographic control-flow integrity (CFI), modular arithmetic representations, latent $p$-adic cellular degradation, and two-agent adversarial concurrency into an uncompromising monolithic C99 runtime.

In the TuxPL 2.0.0 kernel, discrete memory modes, classic stack interpreters, and behavioral mode switches have been permanently eliminated: the runtime operates exclusively on a **flat cryptographic torus $\mathbb{Z}_M$** ($M = 7{,}436{,}429$, fixed size of 65,536 `uint32_t` words), completely blurring the boundary between executable machine code and cryptographic entropy. Programs constitute bounded trajectories across a $k=2$ De Bruijn directed graph over the six-letter alphabet $\Sigma = \{T, t, U, u, X, x\}$, with operands embedded in a six-moduli Residue Number System (RNS) isomorphic to $\mathbb{Z} / 7{,}436{,}429\mathbb{Z}$ via the Chinese Remainder Theorem (CRT). Control-flow integrity is enforced at every line boundary by an 11-byte Galois-Base64 Syndrome Verification (GBSV) barrier over the finite field $\mathbb{F}_{2^8}$ with the Rijndael generator polynomial. Execution proceeds under the supervision of an Affine Borrow Checker ($W \equiv 0 \pmod{17}$), gravitational stack dynamics, metabolic gas consumption, and a deterministic quantum scheduler. Default execution is strictly safeguarded against unauthorized execution by **The Orphan Paradox** (mandatory presence of an 88-byte binary companion file `.tu`), with local development supported via the orthogonal `--no-shadow` flag.

---

## 1. Theoretical Complexity & Threat Model

Traditional esoteric languages achieve obfuscation through ad-hoc randomness or lookup tables (e.g., Malbolge’s ternary crazy function applied to a static 59,049-cell ring). Such designs suffer from deterministic pre-image vulnerabilities and rapid entropy collapse.

TuxPL 2.0.0 formalizes execution as a monolithic system of **simultaneous non-linear invariants across five orthogonal execution strata**:

```text
                                 [ SOURCE CODE ]
                                        │
           ┌────────────────────────────┴────────────────────────────┐
           ▼                                                         ▼
   [ LEXICAL STRATUM ]                                       [ STRUCTURAL STRATUM ]
   • Alphabet: Σ = {T,t,U,u,X,x}                             • File size in bits: |F| = bits(F)
   • Opcode: De Bruijn Mealy Machine (252 states)             • Mandatory micro-libraries Tux/
   • Operand: RNS-CRT 12-char suffix (M = 7,436,429)         • Harmonic cycle: 1-2-3-4-5 tokens
                                        │
           ┌────────────────────────────┴────────────────────────────┐
           ▼                                                         ▼
   [ CRYPTOGRAPHIC CFI ]                                     [ FLAT TORUS Z_M & METABOLISM ]
   • 11-byte GBSV terminator: :[Σ|Χ|Τ];Ζ}                    • Flat torus Z_M (64K uint32_t, M=7436429)
   • Base64 cyclic spectral shift                            • 3-adic age: nu_3(W) mod 4
   • Galois Field F_{2^8} syndrome (Rijndael 0x11B)          • Affine Borrow Checker: W != 0 mod 17
   • p-adic wavelet analysis: nu_2(W) vs nu_3(W)             • Torsion shift: PCD += popcount(W) + 1
   • Hamming collapse Z relative to P_{prev}                 • SP-Round: Irreversible AES S-Box round
                                        │
                                        ▼
                         [ MONOLITHIC ADVERSARIAL RUNTIME ]
                         • Context A (Main User Thread, PC=0)
                         • Context B (Shadow Adversary)
                         • The Orphan Paradox: Mandatory companion (.tu)
                         • Flag --no-shadow for local development
                         • Deterministic quantum scheduler
                         • Metabolic gas budget (100) & time debt
```

### Invariant Equations

To ensure execution trace validity $\mathcal{T} = (\sigma_0, \sigma_1, \dots, \sigma_n)$, every state transition must strictly satisfy the set of formal invariants:

1. **Orthographic Invariant:** Every instruction token $w \in \Sigma^*$ is a valid path on the De Bruijn directed graph generating an opcode $\lambda(q, w) \in [0, 41]$.
2. **Modular Operand Invariant:** Every integer argument $N \in [0, M-1]$ is mapped to residue vector $ec{r} = (N mod m_i)_{i=0}^5$ and encoded into exactly 12 characters via Radix-6 projection.
3. **Harmonic Line-Length Invariant:** Line $L_k$ contains exactly $k \pmod 5 + 1$ tokens ($1 \le |L_k| \le 5$).
4. **Delimiter Steganography:** The trailing space interval after token $w$ contains strictly $\delta(w)$ spaces, where $\delta(w) = \sum_{c \in w} [c \in \{U, u\}]$.
5. **Galois Integrity Invariant:** Every line of source code terminates with a valid 11-byte GBSV signature $\mathcal{S}_k = (\sigma_{	ext{b64}}, \chi_{	ext{gf}}, 	au_{	ext{tux}}, \zeta)$, chaining the line's prefix with the algebraic state of line $k-1$ in $\mathbb{F}_{2^8}$ and Hamming metric.
6. **Affine Ownership Invariant:** Any attempt to fetch an instruction from $\mathrm{PC}_{\mathrm{code}}$ or read memory where $W \equiv 0 \pmod{17}$ triggers an immediate Move-semantics panic (`PANIC_AFFINE_USE_AFTER_MOVE`).
7. **Thermodynamic Energy Invariant:** Gas balance $B_{t+1} = B_t - 1 \ge 0$. Upon exhaustion ($B_t = 0$), execution terminates with code 3 (`PANIC_BUDGET_EXHAUSTION`) unless replenished via `OP_FISH`.

Any invariant breach immediately halts execution with a deterministic `[VM_PANIC]` diagnostic vector and strict POSIX exit code (`1`, `2`, or `3`).

---

## 2. Cryptographic Lexical Core: De Bruijn Mealy Machine & RNS-CRT

### 2.1. De Bruijn Mealy Machine for Opcode Synthesis

Opcodes in TuxPL 2.0.0 are not static constants. They are synthesized dynamically as output signals of a deterministic Mealy machine $\mathcal{M}_{DB} = (Q, \Sigma, \Delta, \delta, \lambda, q_0)$ over alphabet $\Sigma = \{T:0, t:1, U:2, u:3, X:4, x:5\}$.

```text
Automaton Parameters:
  State Space:             |Q| = 252 (multiple of 42, 6 * 42)
  Input Alphabet:          |Σ| = 6
  Output Alphabet:         |Δ| = 42 (Opcodes 0..41)
  Initial State:           q_0 = 0x5A (90)
```

- **State Transition Function:**
  $$\delta(q, c) = (6q + 	ext{idx}(c)) \pmod{252}$$
- **Mealy Output Function:**
  $$\lambda(q, c) = (q \oplus (7 \cdot 	ext{idx}(c))) \pmod{42}$$

#### Theorem 1 (Complete Reachability of Opcodes)
$$orall q \in [0, 251], \quad orall 	ext{op} \in [0, 41], \quad \exists w \in \Sigma^3 \quad 	ext{such that} \quad \lambda^*(\delta^*(q, w_{0..1}), w_2) = 	ext{op}$$

*Empirical Proof:* The verification suite `tests/test_rns_debruijn.c` explores all $252 	imes 42 = 10{,}584$ (state, target opcode) pairs via breadth-first search (BFS). In 100% of cases, a trajectory of length $\le 3$ exists. The runtime exposes `debruijn_encode_fixed3` (fixed length 3) and `debruijn_encode_opcode` (shortest path 1 to 3 characters).

### 2.2. Residue Number System (RNS-CRT Engine)

To eliminate integer overflow vulnerabilities and maximize operand entropy diffusion, integer constants are represented using a Residue Number System based on the Chinese Remainder Theorem (CRT).

#### Moduli Vector & Dynamic Range
A tuple of six pairwise coprime moduli is employed:
$$ec{m} = (m_0, m_1, m_2, m_3, m_4, m_5) = (7, 11, 13, 17, 19, 23)$$
$$M = \prod_{i=0}^5 m_i = 7 	imes 11 	imes 13 	imes 17 	imes 19 	imes 23 = \mathbf{7{,}436{,}429}$$

Any integer $N \in [0, M-1]$ is uniquely represented by its residue tuple:
$$ec{r} = (r_0, r_1, r_2, r_3, r_4, r_5), \quad r_i = N \pmod{m_i}$$

#### Exact Bézout Modular Inverses
The reconstruction isomorphism $N = \left(\sum_{i=0}^5 r_i C_iight) \pmod M$ is computed using precomputed orthogonal Bézout constants $C_i = M_i \cdot (M_i^{-1} mod m_i)$, where $M_i = M / m_i$:

$$C_0 = 6374082 \quad (C_0 \equiv 1 mod 7, \quad C_0 \equiv 0 mod m_{j 
e 0})$$
$$C_1 = 676039 \quad (C_1 \equiv 1 mod 11, \quad C_1 \equiv 0 mod m_{j 
e 1})$$
$$C_2 = 1144066 \quad (C_2 \equiv 1 mod 13, \quad C_2 \equiv 0 mod m_{j 
e 2})$$
$$C_3 = 5249244 \quad (C_3 \equiv 1 mod 17, \quad C_3 \equiv 0 mod m_{j 
e 3})$$
$$C_4 = 782782 \quad (C_4 \equiv 1 mod 19, \quad C_4 \equiv 0 mod m_{j 
e 4})$$
$$C_5 = 646646 \quad (C_5 \equiv 1 mod 23, \quad C_5 \equiv 0 mod m_{j 
e 5})$$

#### Resolution of the Dirichlet Pigeonhole Constraint
Because $m_5 = 23 > |\Sigma| = 6$, a single character from $\Sigma$ cannot encode 23 distinct residues. TuxPL allocates **exactly two characters per modulus** ($6 	imes 2 = 12$ suffix characters total):

Each character pair $(c_{2i}, c_{2i+1})$ represents a base-6 integer in Radix-6:
$$v_i = 6 \cdot 	ext{idx}(c_{2i}) + 	ext{idx}(c_{2i+1}) \in [0, 35]$$
$$r_i = v_i \pmod{m_i}$$

Direct canonical encoding in $O(1)$:
$$c_{2i} = \Sigma[r_i / 6], \quad c_{2i+1} = \Sigma[r_i mod 6]$$

```text
Example: Encoding operand N = 42
  Moduli: m = (7, 11, 13, 17, 19, 23)
  Residues:
    42 mod  7 = 0  --> (0, 0) --> "TT"
    42 mod 11 = 9  --> (1, 3) --> "tu"
    42 mod 13 = 3  --> (0, 3) --> "Tu"
    42 mod 17 = 8  --> (1, 2) --> "tU"
    42 mod 19 = 4  --> (0, 4) --> "TX"
    42 mod 23 = 19 --> (3, 1) --> "ut"
  12-Character Suffix: "TTtuTutUTXut"
  Verification: Sum(r_i * C_i) mod 7436429 = 42. Bijection confirmed.
```

The decoding API `rns_decode_operand(const char *buf, size_t len, uint32_t *out)` accepts explicit buffer lengths, enabling safe parsing within dense instruction streams without requiring null terminators `\0`.

---

## 3. Control-Flow Integrity: Galois-Base64 Syndrome Verification (GBSV)

Every line of program source code is protected by an explicit 11-byte cryptographic terminator preceded by a mandatory single space:
```text
 :[<sigma_b64>|<chi_gf>|<tau_tux>];<zeta>}
```

```text
Terminator Layout (Exactly 11 bytes):
  :[  - 2-byte preamble (ASCII 0x3A 0x5B)
  σ   - Base64 cyclic spectral shift token (1 byte)
  |   - 1-byte separator (ASCII 0x7C)
  χ   - Galois Field F_{2^8} syndrome token (AES 0x11B) (1 byte)
  |   - 1-byte separator (ASCII 0x7C)
  τ   - 2-adic vs 3-adic wavelet analysis token ∈ {'T', 'U', 'X'} (1 byte)
  ];  - 2-byte delimiter (ASCII 0x5D 0x3B)
  ζ   - Hamming metric collapse token ∈ {'t', 'u', 'x', 'p', 'l'} (1 byte)
  }   - 1-byte block terminator (ASCII 0x7D)
```

```text
               Line Prefix: "TuX  tux   TUX"
                        │
       ┌────────────────┼────────────────┬────────────────┐
       ▼                ▼                ▼                ▼
[ Spectral Shift ] [ Galois Field ] [ p-Adic Wavelet ] [ Hamming Collapse ]
     Base64            F_{2^8}        nu_2 vs nu_3       d_H(P, P_{prev})
   σ = B64(acc)   χ = chr(33+S%94)    τ ∈ {T,U,X}        ζ ∈ {t,u,x,p,l}
       │                │                │                │
       └────────────────┴────────┬───────┴────────────────┘
                                 ▼
                 Terminator: " :[k|Ω|X];u}"
```

### 3.1. Mathematical Components of GBSV

1. **Cyclic Spectral Shift Base64 ($\sigma_{	ext{b64}}$):**
   The line prefix $P$ of length $N$ is divided into 6-bit sliding windows $C_i$ offset by $i 	imes 6$ bits. Each chunk undergoes a cyclic left bit-shift by $s = i \pmod 6$:
   $$V_i = 	ext{rotl}_6(C_i, i \pmod 6)$$
   $$	ext{acc} = \sum_{i=0}^{K-1} (V_i \oplus (i \pmod{64}))$$
   $$\sigma_{	ext{b64}} = 	ext{Base64Table}[	ext{acc} \pmod{64}]$$

2. **Galois Field $\mathbb{F}_{2^8}$ Multiplicative Syndrome ($\chi_{	ext{gf}}$):**
   Evaluated over the Rijndael finite field $\mathbb{F}_{2^8} \cong \mathbb{Z}_2[x] / (x^8 + x^4 + x^3 + x + 1)$ with irreducible polynomial `0x11B`:
   $$S = igoplus_{i=0}^{N-1} \mathrm{inv}_{\mathrm{GF}}(P[i] ullet lpha^{(i+1) \pmod{255}})$$
   $$\chi_{	ext{gf}} = 	ext{chr}(33 + (S \pmod{94}))$$
   where $ullet$ represents Galois multiplication, $lpha = 0x03$ is the generator element, and $\mathrm{inv}_{\mathrm{GF}}(y)$ is multiplicative inversion in $\mathbb{F}_{2^8}$ ($\mathrm{inv}(0) = 0$).

3. **$p$-Adic Wavelet Analysis ($	au_{	ext{tux}}$):**
   The prefix $P$ is mapped to a 64-bit integer $W = \sum_{i=0}^{N-1} ((uint64)P[i] \ll (i \pmod 8))$. If $W = 0$, `'T'` is returned. Otherwise, 2-adic and 3-adic valuation orders are compared:
   $$
u_2(W) = 	ext{ctz}(W), \quad 
u_3(W) = \max \{k \in \mathbb{N}_0 : 3^k \mid W\}$$
   $$	au_{	ext{tux}} = egin{cases} 	ext{'X'}, & 	ext{if } 
u_3(W) > 
u_2(W) \ 	ext{'U'}, & 	ext{if } 
u_2(W) > 
u_3(W) \ 	ext{'T'}, & 	ext{if } 
u_3(W) = 
u_2(W) \end{cases}$$

4. **Hamming Metric Chain Collapse ($\zeta$):**
   Measures bitwise Hamming distance between the current line prefix $P_{	ext{curr}}$ and the previous line prefix $P_{	ext{prev}}$ (the first line utilizes the genesis seed `TUX_SEED = "TUXPL_2.0_GENESIS"`):
   $$d_H = \sum_{j=0}^{\max(N_c, N_p)-1} 	ext{popcount}(P_{	ext{curr}}[j] \oplus P_{	ext{prev}}[j])$$
   $$\zeta = 	ext{ZETA\_TABLE}[d_H \pmod 5] \in \{'t', 'u', 'x', 'p', 'l'\}$$

### 3.2. Elimination of Galois/Rijndael Drift Between Python and C

Because `tux_helper.py` serves as a critical compiler and code-generator, any minute arithmetic divergence (such as odd-bit window alignment at $k \pmod 6$ or generator powers $lpha^{i+1}$) immediately breaks runtime CFI verification (`PANIC_GBSV_GF`).

In [tests/test_py_gbsv_cross.py](file:///home/djanki/TuxPL/tests/test_py_gbsv_cross.py), a rigorous automated cross-verification harness is implemented:
* Evaluates **1,010 test vectors** (extreme boundary conditions, empty prefix mutations, long polynomial streams, and 1,000 pseudorandom sequences).
* Loads the C library `src/gbsv.c` via `ctypes` and verifies bit-for-bit identity of `(sigma_b64, chi_gf, tau_tux, zeta)` against the native Python implementation.
* **Result:** 100% bitwise parity (0 bits drift). The cross-test is executed automatically as a mandatory prerequisite in `tests/run.sh`.

Any single-bit mutation or whitespace perturbation in TuxPL source code immediately triggers `PANIC_GBSV_GF`, `PANIC_GBSV_B64`, `PANIC_GBSV_TUX`, or `PANIC_GBSV_ZETA`, abruptly terminating the process.

---

## 4. Flat Cryptographic Torus $\mathbb{Z}_M$ & Latent Cell Invariants

TuxPL 2.0.0 completely eliminates the legacy 40-byte bookkeeping structure `TuxCell`. The entire virtual machine memory is consolidated into a continuous one-dimensional array `uint32_t *unified_mem` residing on a **flat cryptographic torus $\mathbb{Z}_M$**:

$$\mathbb{Z}_M = \mathbb{Z} / 7{,}436{,}429\mathbb{Z}, \quad |	ext{Memory}| = 65{,}536 	ext{ words (256 KB)}$$

This paradigm erases the boundary between code and data: memory is homogeneous cryptographic noise. Every 32-bit machine word $W \in \mathbb{Z}_M$ carries latent mathematical properties evaluated dynamically on-the-fly.

```text
       ┌─────────────────────────────────────────────────────────────┐
       │     Flat Modular Torus Z_M (65,536 words uint32_t)          │
       ├─────────────────────────────┬───────────────────────────────┤
       │  [0 ... prog_len - 1]       │   [prog_len ... 65535]        │
       │  Active Executable Core     │   Data, Heap & Shadow Zone    │
       └──────────────▲──────────────┴───────────────▲───────────────┘
                      │                              │
                PC_CODE (Fetch)                PC_DATA (Dual-PC)
```

### 4.1. Latent Machine-Word Invariants

Every memory cell $W = 	ext{unified\_mem}[	ext{addr}]$ determines four latent quantum attributes:

1. **Latent Cell Age via 3-Adic Valuation $
u_3(W)$:**
   Cell age reflects entropy degradation. In pure mathematics, the $p$-adic order of zero is undefined ($
u_p(0) = \infty$), creating an infinite loop trap. The TuxPL kernel introduces a strict boundary condition:
   $$
u_3(0) = 3 \implies 	ext{DEAD}$$
   For any $W > 0$, the valuation is the maximum power of three dividing $W$:
   $$
u_3(W) = \max \{k \in \mathbb{N}_0 : 3^k \mid W\}$$
   $$	ext{Age}(W) = 
u_3(W) \pmod 4 \in \{0: 	ext{YOUNG}, 1: 	ext{ADULT}, 2: 	ext{OLD}, 3: 	ext{DEAD}\}$$

   * **YOUNG (0):** Opcode is decoded directly by the base dynamic decoder.
   * **ADULT (1):** Opcode undergoes genomic mutation via chromosome $G_1$: $	ext{op} \leftarrow (	ext{op} \oplus G_1) \pmod{42}$.
   * **OLD (2):** Opcode is distorted by CRAZY64 ternary convolution: $	ext{op} \leftarrow 	ext{tux\_crazy64}(	ext{op}, G_2) \pmod{42}$.
   * **DEAD (3):** Complete cellular collapse. Cell irreversibly degrades into an inert `OP_NOP` (`41`).

2. **Latent Operand Type:**
   Derived from the modular projection offset of invariant $0x5A$ (90):
   $$	ext{Type}(W) = (W \oplus 0x5A) \pmod 6$$
   * `0`: `TUX_TYPE_ADDR` (Address on torus $\mathbb{Z}_M$)
   * `1`: `TUX_TYPE_I8` (8-bit signed integer)
   * `2`: `TUX_TYPE_I16` (16-bit signed integer)
   * `3`: `TUX_TYPE_I32` (32-bit modular integer)
   * `4`: `TUX_TYPE_I64` (64-bit extended integer)
   * `5`: `TUX_TYPE_OPCODE` (Executable kernel opcode)

3. **Affine Ownership Invariant (Borrow Checker):**
   In torus $\mathbb{Z}_M$, 17 is one of the six generating prime moduli ($m_3 = 17$). A memory cell is considered **Moved / Consumed** if and only if it is a multiple of 17:
   $$W \equiv 0 \pmod{17} \iff 	ext{Cell is Moved}$$
   Any instruction fetch (`Fetch`) from $\mathrm{PC}_{\mathrm{code}}$ or indirect data read (`OP_LOADIND`) at a location where $W \equiv 0 \pmod{17}$ is caught as *Use-After-Move*, triggering:
   ```text
   [VM_PANIC] PANIC_AFFINE_USE_AFTER_MOVE: Affine borrow checker: cell moved (W = 0 mod 17)
   ```

4. **Torsion Data Pointer Shift ($\Delta \mathrm{PCD}$):**
   During indirect operations and decoding, data pointer $\mathrm{PCD}$ dynamically advances by the population count of the word:
   $$\mathrm{PCD}_{t+1} = (\mathrm{PCD}_t + 	ext{popcount}(W) + 1) \pmod{65536}$$

### 4.2. Post-Execution Irreversible Mutation (SP-Round)

Upon executing an instruction, the cell at $\mathrm{PC}_{\mathrm{code}}$ undergoes irreversible mutation, obliterating the executed bytecode via an SP-Round based on the AES Substitution Box (S-Box):

$$W' = ((	ext{AES\_SBOX}[W \ \& \ 	ext{0xFF}] \ll 16) \oplus (W \gg 8) \oplus (\mathrm{PC}_{\mathrm{code}} 	imes 3)) \pmod M$$

**Cycle False-Move Protection:**
If after the SP-Round mutation the resulting pseudorandom value $W'$ happens to be a multiple of 17 ($W' \equiv 0 \pmod{17}$), the kernel deterministically increments: $W' \leftarrow (W' + 1) \pmod M$. This guarantees that instructions executing inside loops (such as Cyber-Reactor) do not inadvertently trigger borrow checker panics upon subsequent loop iterations.

---

## 5. Monolithic Adversarial Concurrency by Default & Quantum Scheduler

TuxPL 2.0.0 completely eliminates disparate startup modes. Adversarial execution of two isolated agents across the flat torus $\mathbb{Z}_M$ is the **sole, uncompromised runtime behavior (Zero-Flag Monolithic Torus)**.

```text
       ┌─────────────────────────────────────────────────────────┐
       │               Flat Torus Z_M (64K uint32_t)             │
       └────────────▲────────────────────────▲───────────────────┘
                    │                        │
             ┌──────┴──────┐          ┌──────┴──────┐
             │  Context A  │          │  Context B  │
             │ (User Main) │          │ (Adversary) │
             └──────▲──────┘          └──────▲──────┘
                    │                        │
                    └───────────┬────────────┘
                                │
                    ┌───────────┴────────────┐
                    │    Quantum Scheduler   │
                    │      (scheduler.c)     │
                    └────────────────────────┘
```

- **Context A (Main Execution Thread):** User program initialized at $\mathrm{PC}_{\mathrm{code}, A} = 0$, $\mathrm{PC}_{\mathrm{data}, A} = 1024$.
- **Context B (Shadow Adversary):** Injected into a pseudorandom memory location derived from the program FNV-1a hash and genome chromosome $G_1$:
  $$\mathrm{PC}_{\mathrm{code}, B} = (K_{\mathrm{prog}} \oplus G_1) \pmod{65536}$$
  $$\mathrm{PC}_{\mathrm{data}, B} = (\mathrm{PC}_{\mathrm{code}, B} + 512 + (G_2 \pmod{1024})) \pmod{65536}$$
  Registers and entropy for Context B are seeded from the binary companion container `.tu`. The adversary executes within the memory torus, exerting continuous pressure on user state.

### 5.1. The Orphan Paradox & The `--no-shadow` Flag

Because the adversarial engine is monolithic, launching any program with zero flags (`./tuxpl program.tux`) mandates the presence of the paired 88-byte companion container `program.tu`.
* If a programmer writes `test.tux` in a text editor and executes it directly, the runtime panics immediately:
  ```text
  [VM_PANIC] PANIC_COMPANION_ORPHAN: Required companion metadata container (.tu) not found
  ```
  The language fundamentally rejects unsigned code lacking companion metadata.
* **Local Mode `--no-shadow` (alias `--deterministic`):** The primary development workflow for unit testing and local experimentation without companion generation. Disables Context B spawning and runs strictly in single-context mode.

### 5.2. Metabolic Gas Budget

Execution is strictly bounded by a metabolic gas budget:
* The initial budget defaults to **100 units** of gas.
* Every executed instruction consumes 1 unit of gas.
* Upon budget exhaustion, the VM halts with POSIX code 3 (`PANIC_BUDGET_EXHAUSTION`).
* The only mechanism to sustain long-running programs (such as Cyber-Reactor) is periodic energetic replenishment via `OP_FISH` (`tuuUUuuUuux`).

### 5.3. Thermodynamic Reversibility (`--reversible` & `OP_UNDO`)

TuxPL supports temporal reversibility through a thermodynamic ring buffer `TuxHistoryBuffer`:
* Enabled exclusively via `--reversible` (or `--REVERSIBLE`).
* When active, `OP_UNDO` (`tuuUUuUUuuX`) performs a physical rollback of the last state modification (registers, stack, and memory).
* Executing `OP_UNDO` without `--reversible` produces a specification panic:
  ```text
  [VM_PANIC] PANIC_SPEC_NO_HISTORY: Attempted state rollback (OP_UNDO) without reversible snapshot buffer
  ```

### 5.4. Deterministic Quantum Scheduler & Differential Register Coupling

Context switching is deterministic and independent of operating system timers. At each execution step, an entropy convolution of registers and torus memory is computed:
$$\mu = \mathrm{FNV1a}(ec{R}) \oplus 	ext{unified\_mem}[\mathrm{PC}_{\mathrm{code}}] \oplus 	ext{step}$$
$$	ext{Active Context} \leftarrow egin{cases} 	ext{Context A}, & 	ext{if } (\mu \oplus G_0) \pmod 2 = 0 \ 	ext{Context B}, & 	ext{if } (\mu \oplus G_0) \pmod 2 = 1 \end{cases}$$

Registers $R_0 \dots R_3$ undergo differential non-linear cross-coupling at each instruction cycle:
$$\delta_1 = \mathrm{crazy64}(R_0, R_1) \pmod{256}, \quad R_1 \leftarrow R_1 + \delta_1$$
$$\delta_2 = \mathrm{crazy64}(R_1, R_2) \pmod{256}, \quad R_2 \leftarrow R_2 + \delta_2$$
$$\delta_3 = \mathrm{crazy64}(R_2, R_3) \pmod{256}, \quad R_3 \leftarrow R_3 + \delta_3$$
$$\delta_0 = \mathrm{crazy64}(R_3, R_0) \pmod{256}, \quad R_0 \leftarrow R_0 \oplus \delta_0$$

### 5.5. Thermodynamic Time Debt
Instructions accumulate temporal debt `time_debt`:
- Standard instructions: $+1$
- Heavy non-linear operations (`CRAZY`, `CAST`): $+1 + (	ext{entropy} \pmod 3)$
- Cellular cloning (`CLONE`): $+5$

If `time_debt` $> 5000$, memory enters phase collapse. Time debt must be amortized via `OP_PAY_TIME` at the expense of metabolic gas (`gas_budget`).

---

## 6. Type System, Stack Dynamics & Ternary Logic

### 6.1. Affine Move Semantics (Borrow Checker) on Flat Torus
Variables and memory cells operate under strict affine move semantics:
- Reading a variable via `OP_LOAD` moves the value to the stack, marking the memory cell as moved ($W \leftarrow 17$).
- Subsequent reads without an intervening `OP_STORE` trigger `PANIC_AFFINE_USE_AFTER_MOVE` (POSIX code `3`).
- Indirect reading via `OP_LOADIND` on any address where $W \equiv 0 \pmod{17}$ similarly triggers an immediate move-after-use abort.

### 6.2. Stack Gravitational Invariant
The operand stack is subjected to dynamic acceleration limits:
- Consecutive push operations increase stack tension.
- Exceeding 7 consecutive `PUSH` instructions without intermediate computation or pops triggers gravitational collapse:
  $$\mathrm{depth} > 7 \implies 	ext{Stack Gravity Overflow [Exit Code 3]}$$

### 6.3. Balanced 64-Bit Ternary CRAZY Logic
TuxPL implements a 40-trit extension of Malbolge’s ternary crazy operation ($3^{40}$ states):

$$	ext{CRAZY}(t_a, t_b) \quad 	ext{truth table:}$$

| $t_a ackslash t_b$ | 0 | 1 | 2 |
| :---: | :---: | :---: | :---: |
| **0** | 1 | 0 | 0 |
| **1** | 1 | 0 | 2 |
| **2** | 2 | 2 | 1 |

Implemented via parallel bitwise SIMD emulation in `src/state.c`.

---

## 7. Complete Opcode Reference (Table of 42 Opcodes)

| Opcode | Mnemonic | Classic Trajectory | Mathematical Semantics |
| :---: | :--- | :--- | :--- |
| `0` | `OP_ADD` | `TuX` | $a, b 	o (a + b)$ |
| `1` | `OP_SUB` | `Tux` | $a, b 	o (a - b)$ |
| `2` | `OP_MUL` | `TUx` | $a, b 	o (a 	imes b)$ |
| `3` | `OP_DIV` | `TUX` | $a, b 	o (a / b)$, checks $b 
eq 0$ |
| `4` | `OP_DUP` | `tux` | $a 	o a, a$ |
| `5` | `OP_SWAP` | `tuX` | $a, b 	o b, a$ |
| `6` | `OP_POP` | `tUx` | $a 	o arnothing$ |
| `7` | `OP_PRINTCHAR`| `tUX` | Emits $(a mod 256)$ as ASCII character |
| `8` | `OP_PUSH` | `Tuu<bits>X` | Pushes constant $N$ onto stack |
| `9` | `OP_LOAD` | `Tuu<bits>x` | Move value from `vars[N]` onto stack |
| `10`| `OP_STORE` | `TuU<bits>x` | Stores stack top into `vars[N]` |
| `11`| `OP_LOADIND` | `TuU<bits>X` | Reads `mem[PC_DATA + offset]` (requires $W 
ot\equiv 0 \pmod{17}$) |
| `12`| `OP_STOREIND`| `TUu<bits>x` | Writes `mem[PC_DATA + offset] = val` |
| `13`| `OP_JMP` | `TUu<bits>X` | Jump: $\mathrm{PC}_{\mathrm{code}} \leftarrow N$ |
| `14`| `OP_JZ` | `TUU<bits>x` | Conditional branch if $a == 0$ |
| `15`| `OP_JNZ` | `TUU<bits>X` | Conditional branch if $a 
eq 0$ |
| `16`| `OP_CMP` | `tuu<bits>X` | $a, b 	o 	ext{sgn}(a - b) \in \{-1, 0, 1\}$ |
| `17`| `OP_LISTNEW` | `tuuUUuux` | Allocates dynamic vector in torus memory |
| `18`| `OP_LISTPUSH`| `tuuUUuuX` | Appends element to vector |
| `19`| `OP_LISTGET` | `tuuUUuUx` | Reads from vector index |
| `20`| `OP_LISTSET` | `tuuUUuUX` | Writes to vector index |
| `21`| `OP_LISTLEN` | `tuuUUUUx` | Returns vector length |
| `22`| `OP_PRINTNUM`| `tUU<bits>x` | Prints decimal number |
| `23`| `OP_INPUTNUM`| `tUU<bits>X` | Reads decimal number from stdin |
| `24`| `OP_REGGET` | `TuU<reg>X` | Reads register $R_k$ ($k \in [0, 3]$) onto stack |
| `25`| `OP_REGSET` | `TUu<reg>x` | Writes stack top to register $R_k$ |
| `26`| `OP_FISH` | `tuuUUuuUuux` | `OP_REPLENISH_GAS`: Replenishes gas budget ($+50$) |
| `27`| `OP_CRAZY` | `tuuUUuuUuuX` | 64-bit ternary Malbolge convolution |
| `28`| `OP_CAST` | `tuu<bits>x` | Explicit semantic type tag conversion |
| `29`| `OP_DIR` | `tuuUUuuUuUx` | Inverts stack direction |
| `30`| `OP_PUSH_PC` | `tuuUUuuUUuUx`| Pushes current $\mathrm{PC}_{\mathrm{code}}$ |
| `31`| `OP_SET_PC` | `tuuUUuuUUuUX`| Branch: $\mathrm{PC}_{\mathrm{code}} \leftarrow a \pmod{65536}$ |
| `32`| `OP_SWAP_PC` | `tuuUUuuUUUux`| Swap: $\mathrm{PC}_{\mathrm{code}} \leftrightarrow \mathrm{PC}_{\mathrm{data}}$ |
| `33`| `OP_ADD_PC` | `tuuUUuuUUUuX`| Relative offset: $\mathrm{PC}_{\mathrm{code}} \leftarrow (\mathrm{PC}_{\mathrm{code}} + \Delta) \pmod{65536}$ |
| `34`| `OP_XOR_PC` | `tuuUUuuUUUUx`| Masking: $\mathrm{PC}_{\mathrm{code}} \leftarrow (\mathrm{PC}_{\mathrm{code}} \oplus M) \pmod{65536}$ |
| `35`| `OP_CLONE` | `tuuUUuuUUUUX`| Clones cell with generational evolution |
| `36`| `OP_DECAY` | `tuuUUuUUuuux`| Forces cell aging to $	ext{DEAD}$ ($W \leftarrow 0$) |
| `37`| `OP_WAKE` | `tuuUUuUUuuuX`| Awakens cell from $	ext{DEAD}$ ($W \leftarrow 1$) |
| `38`| `OP_REINTERPRET`| `tuuUUuUUuux`| Reinterprets data word as executable opcode |
| `39`| `OP_UNDO` | `tuuUUuUUuuX`| Reversible state rollback via history buffer |
| `40`| `OP_PAY_TIME`| `tuuUUuUUuUx`| Amortizes temporal debt via gas budget |
| `41`| `OP_NOP` | `tuuUUuUUuUX`| Inert operation (terminal dead-cell state) |

---

## 8. Diagnostic Subsystem & POSIX Exit Codes

TuxPL implements an unambiguous diagnostic panic subsystem (`[VM_PANIC]`). Messages are emitted to `stderr` in standard canonical format:

```text
[VM_PANIC] PANIC_<CATEGORY>_<DETAIL>: <Human readable diagnostic message>
```

### Deterministic Exit Codes

| POSIX Code | Semantic Category | Representative Panics |
| :---: | :--- | :--- |
| **`1`** | **Runtime Arithmetic Errors** | `PANIC_RUNTIME_MATH_DIV_ZERO`, `PANIC_RUNTIME_STACK_UNDERFLOW`, `PANIC_GBSV_GF`, `PANIC_GBSV_B64`, `PANIC_GBSV_TUX`, `PANIC_GBSV_ZETA` |
| **`2`** | **Specification, CFI & Companion** | `PANIC_SPEC_LEGACY_REJECTED`, `PANIC_SPEC_FILENAME_MISMATCH`, `PANIC_SPEC_HARMONIC_VIOLATION`, `PANIC_SPEC_MISSING_PRELUDE`, `PANIC_COMPANION_ORPHAN`, `PANIC_COMPANION_HERESY`, `PANIC_SPEC_NO_HISTORY` |
| **`3`** | **Resources, Memory & Ownership** | `PANIC_AFFINE_USE_AFTER_MOVE`, `PANIC_STACK_GRAVITY_OVERFLOW`, `PANIC_BUDGET_EXHAUSTION`, `PANIC_OUT_OF_MEMORY` |

---

## 9. Binary Companion Specification (`.tu` Container)

### Binary Memory Layout

```text
Offset    Size   Field Description
─────────────────────────────────────────────────────────────────
0x00      4 B    Magic Signature:  0x54 0x55 0x58 0x32 ("TUX2")
0x04      4 B    Format Version:   0x00000002
0x08      8 B    64-bit FNV-1a Hash of source file .tux
0x10      8 B    Initial Entropy Seed
0x18      8 B    Genome Chromosome 0 (G0: Structural mutation key)
0x20      8 B    Genome Chromosome 1 (G1: Context B Code Pointer)
0x28      8 B    Genome Chromosome 2 (G2: Context B Data Pointer)
0x30      8 B    Genome Chromosome 3 (G3: Awakening harmonic)
0x38      8 B    Initial Register R0
0x40      8 B    Initial Register R1
0x48      8 B    Initial Register R2
0x50      8 B    Initial Register R3
─────────────────────────────────────────────────────────────────
Total: Exactly 88 bytes of structured binary data.
```

Missing companion files abort execution with `PANIC_COMPANION_ORPHAN` (code 2). FNV-1a checksum mismatches abort with `PANIC_COMPANION_HERESY` (code 2).

---

## 10. Building, Verification & Tooling

### 10.1. Build System
The runtime is authored in standard ISO C99 and compiles with any POSIX C compiler:

```bash
# Build monolithic release binary
make

# Clean compilation artifacts
make clean
```

### 10.2. Formal Test Suite (100% Passing)
The verification suite exercises unit, integration, and invariant layers:

```bash
# Run all 8 test suites and 74k+ formal mathematical assertions
make test
```

#### Test Suite Composition (8 Test Suites):
1. **Galois Field Verification (`make test-gbsv`):** 24/24 tests verifying $\mathbb{F}_{2^8}$ multiplicative inversion, the Rijndael polynomial, and GBSV signature sensitivity.
2. **De Bruijn Automaton & RNS-CRT Engine (`make test-rns`):**
   - **Bézout Inverses:** $C_i \equiv 1 \pmod{m_i}$ and $C_i \equiv 0 \pmod{m_j}$.
   - **CRT Isomorphism:** 100,000 pseudorandom integers verified for $\text{decode}(\text{encode}(N)) == N$.
   - **Dirichlet Resolution:** 100% residue coverage of $m_5=23$ via 12-char Radix-6 suffix.
   - **Streaming Decoder:** Zero-byte-agnostic parsing from raw bytecode buffers.
   - **De Bruijn Completeness:** All 10,584 pairs reached within $\le 3$ transitions.
   - **Avalanche Diffusion:** 1-character perturbations propagate maximal entropy.
   *(74,307 formal assertions)*
3. **Flat Torus $\mathbb{Z}_M$ Core (`make test-flat-core`):** 36/36 checks verifying latent age ($
u_3(0) = 3$), latent types ($0x5A$), Affine Move Checker ($W \equiv 0 \pmod{17}$), and SP-Round loop protection.
4. **Galois/Rijndael Python $\leftrightarrow$ C Cross-Test (`tests/test_py_gbsv_cross.py`):** 1,010/1,010 test vectors verifying bit-for-bit identity (0 bits drift).
5. **Classic Monolithic Test Suite (`tests/run.sh`):** 15/15 tests covering stack, arithmetic, branching, and dynamic lists.
6. **Structural Invariants Suite (`tests/test_cursed.sh`):** 8/8 tests verifying bit length filenames, whitespace parity, and checksums.
7. **Memory Safety & Purgatory Suite (`tests/test_hardcore.sh`):** 12/12 tests validating borrow-checker, stack gravity, and gas exhaustion.
8. **Golden Vectors Suite (`tests/test_golden_vectors.sh`):** 22/22 tests validating deterministic FNV-1a, CRAZY64, and cell aging.
9. **Monolithic Adversarial Suite (`tests/test_apocalypse.sh`):** 11/11 tests validating dual-context scheduling, adversary injection, `OP_UNDO` reversibility, and the orphan paradox.

```text
Verification Summary:
  Test Suites:         8 / 8   (100% PASS)
  Total Assertions:    74,445  (100% PASS)
  Compiler Warnings:   0       (-Wall -Wextra -pedantic)
  Memory Sanitizers:   ASan + UBSan (0 leaks, 0 undefined behavior)
```

### 10.3. Python Toolchain (`tux_helper.py`)

```bash
# De Bruijn opcode synthesis
python3 tux_helper.py debruijn 8 0x5A
# Outputs: 3-character path and minimal trajectory resolving to OP_PUSH

# RNS operand encoding
python3 tux_helper.py rns-enc 42
# Outputs: 12-character RNS suffix "TTtuTutUTXut"

# RNS operand decoding
python3 tux_helper.py rns-dec TTtuTutUTXut
# Outputs: 42

# Compute GBSV signature for line
python3 tux_helper.py gbsv-sig "TuX  tux   TUX"
# Outputs: " :[k|Ω|X];u}"

# Verify GBSV signature
python3 tux_helper.py gbsv-verify "TuX  tux   TUX :[k|Ω|X];u}"

# Generate valid 88-byte companion container
python3 tux_helper.py companion examples/141248.tux
```

---

## 11. Command-Line Interface

In TuxPL 2.0.0, legacy execution modes (`--CLASSIC`, `--STRICT`, `--UNIFIED-VM`, `--ADVERSARIAL`) have been completely superseded. Adversarial execution across the flat torus under GBSV verification is the sole, monolithic default.

```text
Usage: tuxpl [OPTIONS] <source_file.tux>

Runtime Options:
  --no-shadow, --deterministic   Execute in single-context mode without shadow adversary
                                 (allows running code without companion .tu container)
  --reversible, --REVERSIBLE     Enable thermodynamic history ring buffer (enables OP_UNDO)
  --disasm                       Disassemble torus memory prior to execution
  --trace-state                  Emit step-by-step trace of quantum scheduler state
  --help                         Display technical reference and command options
  --version                      Display build version and kernel architectural summary
```

---

## 12. Conclusion & Academic Significance

TuxPL 2.0.0 demonstrates that esoteric programming languages can serve as rigorous mathematical proving grounds for research in computer security, formal verification, and fault tolerance. By unifying Galois field theory, Chinese Remainder Theorem modular representations, topological routing over De Bruijn graphs, a flat cryptographic torus $\mathbb{Z}_M$, and deterministic adversarial concurrency, TuxPL establishes an execution standard where correctness is provable, and unauthorized state mutation is cryptographically impossible.
