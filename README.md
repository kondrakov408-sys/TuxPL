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
  <img src="https://img.shields.io/badge/Memory%20Model-Unified%2064K%20Cells-black.svg?style=flat-square" alt="Unified Memory">
  <img src="https://img.shields.io/badge/Verification-90%2F90%20Suites%20%7C%2074k%20Assertions%20(100%25)-brightgreen.svg?style=flat-square" alt="Tests Passing">
</p>

---

## Abstract

This specification defines **TuxPL 2.0.0**, a deterministic, adversarial, polymorphic programming language and execution environment engineered as a high-order Constraint Satisfaction Problem (CSP). Departing fundamentally from trivial substitution-based esoteric architectures (such as Malbolge or INTERCAL), TuxPL integrates cryptographic control-flow integrity, modular arithmetic representations, biological cellular memory degradation, and two-agent adversarial concurrency into a formally verified C99 runtime.

Programs in TuxPL are not sequences of unconstrained mnemonics; they constitute bounded trajectories across a $k=2$ De Bruijn directed graph over the six-letter alphabet $\Sigma = \{T, t, U, u, X, x\}$, with operands embedded in a six-moduli Residue Number System (RNS) isomorphic to $\mathbb{Z} / 7{,}436{,}429\mathbb{Z}$ via the Chinese Remainder Theorem. Control-flow integrity is enforced at every line boundary by an 11-byte Galois-Base64 Syndrome Verification (GBSV) barrier evaluated over the finite field $\mathbb{F}_{2^8}$ with the Rijndael generator polynomial. Execution is hosted within a Von Neumann unified memory of 65,536 self-mutating cellular units governed by affine move-semantics, gravitational stack dynamics, and an adversarial quantum scheduler arbitrating between user execution and a concurrent shadow agent.

---

## 1. Theoretical Complexity & Threat Model

Traditional esoteric languages achieve obfuscation through ad-hoc randomness or lookup tables (e.g., Malbolge’s ternary crazy function applied to a static 59,049-cell ring). Such designs suffer from deterministic pre-image vulnerabilities and rapid entropy collapse.

TuxPL 2.0.0 formalizes execution as a system of **simultaneous non-linear invariants across five orthogonal execution strata**:

```text
                                 [ SOURCE CODE ]
                                        │
           ┌────────────────────────────┴────────────────────────────┐
           ▼                                                         ▼
   [ LEXICAL STRATUM ]                                       [ STRUCTURAL STRATUM ]
   • Alphabet: Σ = {T,t,U,u,X,x}                             • File size in bits: |F| = bits(F)
   • Opcode: De Bruijn Mealy Machine (252 states)             • Micro-library declarations
   • Operand: RNS-CRT 12-char suffix (M = 7,436,429)         • Harmonic cycle: 1-2-3-4-5 tokens
                                        │
           ┌────────────────────────────┴────────────────────────────┐
           ▼                                                         ▼
   [ CRYPTOGRAPHIC CFI ]                                     [ METABOLIC / MEMORY ]
   • GBSV 11-byte barrier: :[Σ|Χ;Ζ]                          • Unified 64K cellular memory
   • Base64 cyclic spectral shift                            • Dual PC: PC_CODE ↔ PC_DATA
   • Galois Field F_{2^8} syndrome analysis                  • Affine borrow-checker (Move semantics)
   • Hamming collapse with ~"TUX" seed                       • Cellular degradation: YOUNG→DEAD
                                        │
                                        ▼
                         [ ADVERSARIAL RUNTIME ENGINE ]
                         • Context A (User, PC=0)
                         • Context B (Shadow Adversary)
                         • Deterministic Quantum Scheduler
                         • Differential Register Coupling
```

### Invariant Equations

For an execution trace $\mathcal{T} = (\sigma_0, \sigma_1, \dots, \sigma_n)$ to remain valid, every transition $\sigma_t \to \sigma_{t+1}$ must satisfy:

1. **Orthographic Invariant:** Every instruction token $w \in \Sigma^*$ is a valid walk on the De Bruijn automaton producing opcode $\lambda(q, w) \in [0, 41]$.
2. **Modular Operand Invariant:** Every integer argument $N \in [0, M-1]$ is mapped to residues $\vec{r} = (N \bmod m_i)_{i=0}^5$ and encoded into 12 characters via Radix-6 projection.
3. **Harmonic Length Invariant:** Line $L_k$ contains exactly $k \pmod 5 + 1$ tokens ($1 \le |L_k| \le 5$).
4. **Whitespace Steganography:** The delimiter following token $w$ contains $\delta(w)$ space characters, where $\delta(w) = \sum_{c \in w} [c \in \{U, u\}]$.
5. **Galois CFI Invariant:** Every line terminates with a valid GBSV signature $\mathcal{S}_k = (\Sigma_k, \mathrm{X}_k, Z_k)$ binding the line prefix to the algebraic state of line $k-1$ in $\mathbb{F}_{2^8}$.
6. **Thermodynamic Gas Invariant:** $B_{t+1} = B_t - \text{cost}(\text{op}_t) \ge 0$. If $B_t = 0$, execution halts with panic code 3 (`PANIC_BUDGET_EXHAUSTION`).

Violation of any invariant terminates the virtual machine with an unrecoverable `[VM_PANIC]` diagnostics vector and an academic POSIX exit code (`1`, `2`, or `3`).

---

## 2. Cryptographic Lexical Core: De Bruijn Mealy Machine & RNS-CRT

### 2.1. De Bruijn Mealy Machine for Opcode Synthesis

Opcodes in TuxPL 2.0.0 are not static byte constants. They represent state-transition outputs emitted by a deterministic Mealy automaton $\mathcal{M}_{DB} = (Q, \Sigma, \Delta, \delta, \lambda, q_0)$ operating over the alphabet $\Sigma = \{T:0, t:1, U:2, u:3, X:4, x:5\}$.

```text
Automaton Characteristics:
  States:           |Q| = 252 (divisible by 42, 6 * 42)
  Alphabet:         |Σ| = 6
  Output Alphabet:  |Δ| = 42 (TuxPL Opcodes 0..41)
  Initial Seed:     q_0 = 0x5A (90)
```

- **State Transition Function:**
  $$\delta(q, c) = (6q + \text{idx}(c)) \pmod{252}$$
- **Mealy Output Function:**
  $$\lambda(q, c) = (q \oplus (7 \cdot \text{idx}(c))) \pmod{42}$$

#### Theorem 1 (Complete Reachability of Opcodes)
$$\forall q \in [0, 251], \quad \forall \text{op} \in [0, 41], \quad \exists w \in \Sigma^3 \quad \text{such that} \quad \lambda^*(\delta^*(q, w_{0..1}), w_2) = \text{op}$$

*Empirical Proof:* The test suite `tests/test_rns_debruijn.c` evaluates all $252 \times 42 = 10{,}584$ state-opcode pairs via breadth-first search (BFS). In 100% of cases, an exact path of length $\le 3$ exists. The runtime exposes `debruijn_encode_fixed3` (constant 3-character path) and `debruijn_encode_opcode` (minimal 1..3 character path).

### 2.2. Residue Number System (RNS-CRT Engine)

To eliminate integer-overflow vulnerabilities and enforce high-entropy operand dispersion, immediate integer operands are processed in a non-positional Residue Number System based on the Chinese Remainder Theorem (CRT).

#### Moduli Vector & Dynamic Range
The system uses six pairwise coprime moduli:
$$\vec{m} = (m_0, m_1, m_2, m_3, m_4, m_5) = (7, 11, 13, 17, 19, 23)$$
$$M = \prod_{i=0}^5 m_i = 7 \times 11 \times 13 \times 17 \times 19 \times 23 = \mathbf{7{,}436{,}429}$$

Any integer $N \in [0, M-1]$ is uniquely represented by its residue tuple:
$$\vec{r} = (r_0, r_1, r_2, r_3, r_4, r_5), \quad r_i = N \pmod{m_i}$$

#### Exact Bézout Modular Inverses
The reconstruction isomorphism $N = \left(\sum_{i=0}^5 r_i C_i\right) \pmod M$ utilizes precomputed Bézout constants $C_i = M_i \cdot (M_i^{-1} \bmod m_i)$, where $M_i = M / m_i$:

$$C_0 = 6374082 \quad (C_0 \equiv 1 \bmod 7, \quad C_0 \equiv 0 \bmod m_{j \ne 0})$$
$$C_1 = 676039 \quad (C_1 \equiv 1 \bmod 11, \quad C_1 \equiv 0 \bmod m_{j \ne 1})$$
$$C_2 = 1144066 \quad (C_2 \equiv 1 \bmod 13, \quad C_2 \equiv 0 \bmod m_{j \ne 2})$$
$$C_3 = 5249244 \quad (C_3 \equiv 1 \bmod 17, \quad C_3 \equiv 0 \bmod m_{j \ne 3})$$
$$C_4 = 782782 \quad (C_4 \equiv 1 \bmod 19, \quad C_4 \equiv 0 \bmod m_{j \ne 4})$$
$$C_5 = 646646 \quad (C_5 \equiv 1 \bmod 23, \quad C_5 \equiv 0 \bmod m_{j \ne 5})$$

#### Resolution of the Dirichlet Pigeonhole Constraint
Because $m_5 = 23 > |\Sigma| = 6$, a single 6-ary character cannot represent residues up to 22. TuxPL resolves this by allocating **exactly 2 characters per modulus** ($6 \times 2 = 12$ characters total):

Each character pair $(c_{2i}, c_{2i+1})$ represents a Radix-6 integer:
$$v_i = 6 \cdot \text{idx}(c_{2i}) + \text{idx}(c_{2i+1}) \in [0, 35]$$
$$r_i = v_i \pmod{m_i}$$

Canonical encoding maps $r_i$ deterministically in $O(1)$:
$$c_{2i} = \Sigma[r_i / 6], \quad c_{2i+1} = \Sigma[r_i \bmod 6]$$

```text
Example: Encoding Operand N = 42
  m = (7, 11, 13, 17, 19, 23)
  Residues:
    42 mod  7 = 0  --> (0, 0) --> "TT"
    42 mod 11 = 9  --> (1, 3) --> "tu"
    42 mod 13 = 3  --> (0, 3) --> "Tu"
    42 mod 17 = 8  --> (1, 2) --> "tU"
    42 mod 19 = 4  --> (0, 4) --> "TX"
    42 mod 23 = 19 --> (3, 1) --> "ut"
  12-Character Suffix: "TTtuTutUTXut"
  Verification: Sum(r_i * C_i) mod 7436429 = 42. Exactly reversible.
```

The decoder `rns_decode_operand(const char *buf, size_t len, uint32_t *out)` accepts explicit length parameters, permitting streaming execution without embedded null terminators.

---

## 3. Control-Flow Integrity: Galois-Base64 Syndrome Verification (GBSV)

Every line of TuxPL source code is protected by an 11-byte cryptographically bound terminator:
```text
:[Σ|Χ;Ζ]
```

```text
Format Breakdown:
  :[  - 2-byte signature preamble (ASCII 0x3A 0x5B)
  Σ   - Base64 Spectral Character (Line harmonic projection)
  |   - 1-byte delimiter
  Χ   - Galois Field F_{2^8} Syndrome Character (AES irreducible poly)
  ;   - 1-byte delimiter
  Ζ   - Base64 Hamming Collapse Character (Weighted metric)
  ]   - 1-byte signature terminator (ASCII 0x5D)
```

```text
               Line Prefix: "TuX  tux   TUX"
                        │
       ┌────────────────┼────────────────┐
       ▼                ▼                ▼
[ Cyclic Shift ] [ Field F_{2^8} ] [ Hamming Collapse ]
  Σ = Base64(H)    Χ = Poly(S)       Ζ = Hamming(P, ~"TUX")
       │                │                │
       └────────────────┼────────────────┘
                        ▼
            Terminator: ":[k|Ω;7]"
```

### 3.1. Mathematical Components of GBSV

1. **Spectral Base64 Shift ($\Sigma$):**
   Computed over line index $L$, token count $K$, and prefix byte sum:
   $$H = \left(\sum_{j=0}^{|P|-1} P[j] \cdot (j+1) + (L \cdot 37) + (K \cdot 101)\right) \pmod{64}$$
   $$\Sigma = \text{Base64Table}[H]$$

2. **Galois Field $\mathbb{F}_{2^8}$ Multiplicative Syndrome ($\mathrm{X}$):**
   Evaluated in the Rijndael field $\mathbb{F}_{2^8} \cong \mathbb{Z}_2[x] / (x^8 + x^4 + x^3 + x + 1)$ (modular polynomial `0x11B`):
   $$S_0 = \text{seed}_{L-1}$$
   $$S_{j+1} = (S_j \bullet P[j]) \oplus P[j], \quad \text{where } \bullet \text{ denotes Galois field multiplication}$$
   $$\mathrm{X} = \text{gf\_inv}(S_{|P|}) \oplus 0\text{xA5}$$

3. **Hamming Metric Collapse ($Z$):**
   Measures bitwise divergence against the constant seed mask `~"TUX"` (`0xDF 0xAA 0xA7`):
   $$Z = \left(\sum_{j=0}^{|P|-1} \text{popcount}(P[j] \oplus \text{TUX\_SEED}[j \bmod 3]) \cdot 13 + L\right) \pmod{64}$$
   $$Z = \text{Base64Table}[Z]$$

A single bit flip in code or whitespace produces an immediate syndrome collapse, triggering `PANIC_GBSV_GF` or `PANIC_GBSV_B64` with POSIX exit code `1`.

---

## 4. Unified Memory 2.0 & Cellular Biological Lifecycle

TuxPL abandons traditional separated stack/heap architectures in favour of a **Von Neumann Unified Memory** of 65,536 cellular units (`TuxCell`):

```c
typedef struct {
    int64_t  val;          /* Canonical integer payload */
    uint16_t raw_code;     /* 16-bit encoded instruction image */
    uint8_t  type_tag;     /* Semantic type: i8, i16, i32, i64, TRIT, ADDR, OPCODE */
    uint8_t  age;          /* Cellular generation: YOUNG, ADULT, OLD, DEAD */
    uint8_t  gen;          /* Clone generation index (0..255) */
    uint8_t  flags;        /* Bitflags: EXECUTABLE, MUTATED, CLONED, DORMANT */
    uint32_t lineage;      /* Deterministic lineage identifier */
    uint32_t exec_count;   /* Execution frequency counter */
} TuxCell;
```

```text
    ┌─────────────────────────────────────────────────────────────┐
    │                 64K Unified Memory Space                    │
    ├─────────────────────────────┬───────────────────────────────┤
    │  [0 ... prog_len - 1]       │   [prog_len ... 65535]        │
    │  Active Executable Core     │   Dynamic Data, Heap & Shadow │
    └──────────────▲──────────────┴───────────────▲───────────────┘
                   │                              │
             PC_CODE (Fetch)                PC_DATA (Indirect)
```

### 4.1. Dual Program Counter Architecture
Execution state maintains two independent registers:
- `PC_CODE`: Execution pointer for instruction fetch and cellular aging.
- `PC_DATA`: Base address pointer for indirect cellular access (`OP_LOADIND`, `OP_STOREIND`).

Dedicated vector instructions arbitrate PC state:
- `OP_PUSH_PC` (`tuuUUuuUUuUx`): Pushes `PC_CODE` with tag `TUX_TYPE_ADDR`.
- `OP_SET_PC` (`tuuUUuuUUuUX`): Pops address $A$ and branches: $\text{PC\_CODE} \leftarrow A \pmod{65536}$.
- `OP_SWAP_PC` (`tuuUUuuUUUux`): Atomically exchanges $\text{PC\_CODE} \leftrightarrow \text{PC\_DATA}$.
- `OP_ADD_PC` (`tuuUUuuUUUuX`): Relative offset displacement: $\text{PC\_CODE} \leftarrow (\text{PC\_CODE} + \Delta) \pmod{65536}$.
- `OP_XOR_PC` (`tuuUUuuUUUUx`): Bitwise mask: $\text{PC\_CODE} \leftarrow (\text{PC\_CODE} \oplus M) \pmod{65536}$.

### 4.2. Biological Cellular Degradation

Every execution of a cell increments its `exec_count`. Cellular competence degrades across four non-reversible stages:

```text
  exec_count = 0        exec_count = 3        exec_count = 6        exec_count ≥ 9
  ┌────────────┐        ┌────────────┐        ┌────────────┐        ┌────────────┐
  │   YOUNG    │ ─────> │   ADULT    │ ─────> │    OLD     │ ─────> │    DEAD    │
  │ Baseline   │        │ Genomic G1 │        │ CRAZY64    │        │ Immutable  │
  │ Decoding   │        │ Distortion │        │ Convolution│        │ OP_NOP     │
  └────────────┘        └────────────┘        └────────────┘        └────────────┘
```

1. **YOUNG ($0 \le \text{exec\_count} < 3$):** Opcode resolves directly through primary decoding table.
2. **ADULT ($3 \le \text{exec\_count} < 6$):** Instruction decoded with genomic chromosome perturbation:
   $$\text{opcode} \leftarrow (\text{opcode} \oplus G_1) \pmod{42}$$
3. **OLD ($6 \le \text{exec\_count} < 9$):** Instruction subjected to ternary Malbolge convolution:
   $$\text{opcode} \leftarrow \text{tux\_crazy64}(\text{opcode}, G_2) \pmod{42}$$
4. **DEAD ($\text{exec\_count} \ge 9$):** The cell suffers terminal biological collapse. The cell permanently decodes as `OP_NOP` (`0x29`). The executable footprint cannot execute loops of length $> 8$ without cellular renewal via `OP_CLONE`.

### 4.3. Post-Execution Auto-Mutation
Upon completing an instruction, the cell value is mutated deterministically:
$$\Delta = \text{tux\_crazy64}(\text{result}, \text{entropy\_pool})$$
$$\text{cell.val} \leftarrow \text{cell.val} \oplus (\Delta \pmod{256})$$
$$\text{cell.raw\_code} \leftarrow \text{mutation\_encode}(\text{cell.val}, \text{program\_key}, G_0)$$
$$\text{cell.flags} \leftarrow \text{cell.flags} \mid \text{TUX\_FLAG\_MUTATED}$$

---

## 5. Adversarial Concurrency & Deterministic Quantum Scheduler

In Adversarial Mode (`--ADVERSARIAL`, formerly `--APOCALYPSE`), execution shifts to a **two-agent competitive runtime**:

```text
       ┌─────────────────────────────────────────────────────────┐
       │                   Unified Memory (64K)                  │
       └────────────▲────────────────────────▲───────────────────┘
                    │                        │
             ┌──────┴──────┐          ┌──────┴──────┐
             │  Context A  │          │  Context B  │
             │ (User, PC=0)│          │ (Shadow PC) │
             └──────▲──────┘          └──────▲──────┘
                    │                        │
                    └───────────┬────────────┘
                                │
                    ┌───────────┴────────────┐
                    │ Quantum Scheduler Step │
                    │      (scheduler.c)     │
                    └────────────────────────┘
```

- **Context A (Primary Agent):** Executes user program starting at $\text{PC\_CODE}_A = 0$, $\text{PC\_DATA}_A = 1024$.
- **Context B (Shadow Adversary):** Instantiated at a pseudo-random memory boundary derived from program FNV-1a key and genomic chromosome $G_1$:
  $$\text{PC\_CODE}_B = (\text{program\_key} \oplus G_1) \pmod{65536}$$
  $$\text{PC\_DATA}_B = (\text{PC\_CODE}_B + 512 + (G_2 \pmod{1024})) \pmod{65536}$$
  Context B registers and entropy are initialized from the companion `.tu` container. Context B executes concurrently within the shadow region ($prog\_len \dots 65535$), modifying background cells and altering registers.

### 5.1. Deterministic Scheduler Metric
Context switching does not rely on OS preemption. At each machine cycle, the scheduler computes an entropy metric over active memory and hardware registers:
$$\mu = \text{FNV-1a}(\text{active\_ctx.regs}) \oplus \text{mem}[\text{PC\_CODE}].\text{val} \oplus \text{step\_counter}$$
$$\text{Active Context} \leftarrow \begin{cases} \text{Context A}, & \text{if } (\mu \oplus G_0) \pmod 2 = 0 \\ \text{Context B}, & \text{if } (\mu \oplus G_0) \pmod 2 = 1 \end{cases}$$

### 5.2. Differential Register Coupling
Registers $R_0 \dots R_3$ are interconnected across a non-linear feedback loop evaluated at each clock cycle:
$$\delta_1 = \text{tux\_crazy64}(R_0, R_1) \pmod{256}, \quad R_1 \leftarrow R_1 + \delta_1$$
$$\delta_2 = \text{tux\_crazy64}(R_1, R_2) \pmod{256}, \quad R_2 \leftarrow R_2 + \delta_2$$
$$\delta_3 = \text{tux\_crazy64}(R_2, R_3) \pmod{256}, \quad R_3 \leftarrow R_3 + \delta_3$$
$$\delta_0 = \text{tux\_crazy64}(R_3, R_0) \pmod{256}, \quad R_0 \leftarrow R_0 \oplus \delta_0$$

### 5.3. Thermodynamic Time Debt
Instructions accumulate an entropy debt `time_debt`:
- Standard instruction: $+1$
- Complex non-linear instructions (`CRAZY`, `CAST`): $+1 + (\text{entropy} \pmod 3)$
- Cellular clone (`CLONE`): $+5$

If $\text{time\_debt} > 5000$, memory corruption occurs. The debt must be liquidated via `OP_PAY_TIME` at the cost of execution fuel (`gas_budget`).

---

## 6. Type System, Stack Dynamics & Ternary Logic

### 6.1. Affine Move Semantics (Borrow Checker)
Variables and cellular units adhere to strict affine typing:
- Reading a variable via `OP_LOAD` transfers ownership (`Move`).
- Attempting a secondary read without an intervening `OP_STORE` triggers `PANIC_AFFINE_USE_AFTER_MOVE` and terminates with POSIX exit code `3`.

### 6.2. Stack Gravitational Invariant
The operand evaluation stack enforces a dynamic gravity constraint:
- Consecutive push operations increase stack tension.
- If more than 7 push instructions occur without an intervening arithmetic, reduction, or pop operation, stack gravity collapses:
  $$\text{depth} > 7 \implies \text{PANIC\_STACK\_GRAVITY\_OVERFLOW (Exit Code 3)}$$

### 6.3. Balanced 64-Bit Ternary CRAZY Logic
TuxPL embeds a 64-bit extension of the Malbolge ternary operation across 40 trits ($3^{40}$ space):

$$\text{CRAZY}(t_a, t_b) \quad \text{truth table:}$$

| $t_a \backslash t_b$ | 0 | 1 | 2 |
| :---: | :---: | :---: | :---: |
| **0** | 1 | 0 | 0 |
| **1** | 1 | 0 | 2 |
| **2** | 2 | 2 | 1 |

Applied across all trit positions via parallel bitmask bit-slicing in `src/state.c`.

---

## 7. Complete Opcode Reference (Table of 42 Opcodes)

TuxPL 2.0.0 defines exactly 42 opcodes ($0 \dots 41$).

| Code | Mnemonic | Classical Representation | Mathematical Semantics |
| :---: | :--- | :--- | :--- |
| `0` | `OP_ADD` | `TuX` | $a, b \to (a + b)$ |
| `1` | `OP_SUB` | `Tux` | $a, b \to (a - b)$ |
| `2` | `OP_MUL` | `TUx` | $a, b \to (a \times b)$ |
| `3` | `OP_DIV` | `TUX` | $a, b \to (a / b)$, checks $b \neq 0$ |
| `4` | `OP_DUP` | `tux` | $a \to a, a$ |
| `5` | `OP_SWAP` | `tuX` | $a, b \to b, a$ |
| `6` | `OP_POP` | `tUx` | $a \to \varnothing$ |
| `7` | `OP_PRINTCHAR`| `tUX` | Emits $(a \bmod 256)$ as ASCII character |
| `8` | `OP_PUSH` | `Tuu<bits>X` | Pushes immediate integer $N$ onto stack |
| `9` | `OP_LOAD` | `Tuu<bits>x` | Transfers ownership from `vars[N]` (Move) |
| `10`| `OP_STORE` | `TuU<bits>x` | Stores top of stack into `vars[N]` |
| `11`| `OP_LOADIND` | `TuU<bits>X` | Reads `mem[PC_DATA + offset]` |
| `12`| `OP_STOREIND`| `TUu<bits>x` | Writes `mem[PC_DATA + offset] = val` |
| `13`| `OP_JMP` | `TUu<bits>X` | Sets $\text{PC\_CODE} \leftarrow N$ |
| `14`| `OP_JZ` | `TUU<bits>x` | Conditional branch if $a == 0$ |
| `15`| `OP_JNZ` | `TUU<bits>X` | Conditional branch if $a \neq 0$ |
| `16`| `OP_CMP` | `tuu<bits>X` | $a, b \to \text{sgn}(a - b) \in \{-1, 0, 1\}$ |
| `17`| `OP_LISTNEW` | `tuuUUuux` | Allocates dynamic vector in Unified Memory |
| `18`| `OP_LISTPUSH`| `tuuUUuuX` | Appends element to dynamic vector |
| `19`| `OP_LISTGET` | `tuuUUuUx` | Index read from dynamic vector |
| `20`| `OP_LISTSET` | `tuuUUuUX` | Index write to dynamic vector |
| `21`| `OP_LISTLEN` | `tuuUUUUx` | Queries dynamic vector length |
| `22`| `OP_PRINTNUM`| `tUU<bits>x` | Emits integer as decimal string |
| `23`| `OP_INPUTNUM`| `tUU<bits>X` | Reads signed decimal integer from stdin |
| `24`| `OP_REGGET` | `TuU<reg>X` | Pushes hardware register $R_k$ ($k \in [0, 3]$) |
| `25`| `OP_REGSET` | `TUu<reg>x` | Pops into hardware register $R_k$ |
| `26`| `OP_FISH` | `tuuUUuuUuux` | `OP_REPLENISH_GAS`: Adds fuel to `gas_budget` |
| `27`| `OP_CRAZY` | `tuuUUuuUuuX` | 64-bit balanced ternary Malbolge convolution |
| `28`| `OP_CAST` | `tuu<bits>x` | Explicit re-tagging of numeric type |
| `29`| `OP_DIR` | `tuuUUuuUuUx` | Reverses evaluation stack direction |
| `30`| `OP_PUSH_PC` | `tuuUUuuUUuUx`| Pushes current `PC_CODE` |
| `31`| `OP_SET_PC` | `tuuUUuuUUuUX`| $\text{PC\_CODE} \leftarrow a \pmod{65536}$ |
| `32`| `OP_SWAP_PC` | `tuuUUuuUUUux`| $\text{PC\_CODE} \leftrightarrow \text{PC\_DATA}$ |
| `33`| `OP_ADD_PC` | `tuuUUuuUUUuX`| $\text{PC\_CODE} \leftarrow (\text{PC\_CODE} + \Delta) \pmod{65536}$ |
| `34`| `OP_XOR_PC` | `tuuUUuuUUUUx`| $\text{PC\_CODE} \leftarrow (\text{PC\_CODE} \oplus M) \pmod{65536}$ |
| `35`| `OP_CLONE` | `tuuUUuuUUUUX`| Clones cell to address with generation increment |
| `36`| `OP_DECAY` | `tuuUUuUUuuux`| Manually ages cell by $+1$ generation |
| `37`| `OP_WAKE` | `tuuUUuUUuuuX`| Clears `TUX_FLAG_DORMANT` flag at target cell |
| `38`| `OP_REINTERPRET`| `tuuUUuUUuux`| Reinterprets numeric payload as raw instruction |
| `39`| `OP_UNDO` | `tuuUUuUUuuX`| Restores execution frame from reversible ring |
| `40`| `OP_PAY_TIME`| `tuuUUuUUuUx`| Liquidates accumulated time debt |
| `41`| `OP_NOP` | `tuuUUuUUuUX`| No operation (Terminal state of `DEAD` cells) |

---

## 8. Diagnostic Subsystem & POSIX Diagnostic Specification

TuxPL replaces troll routines with a strictly typed diagnostic architecture (`src/diag.c`, `src/diag.h`). Any fatal condition produces a formatted stderr vector and a deterministic exit status.

### Diagnostic Output Format
```text
[VM_PANIC] <CATEGORY_CODE>: <Exact descriptive failure reason>
  --> Location: line <num>
```

### Deterministic Exit Codes

| POSIX Code | Failure Class | Diagnostics Enumeration |
| :---: | :--- | :--- |
| **`1`** | **Mathematical & CFI Violations** | `PANIC_GBSV_SYNTAX`, `PANIC_GBSV_B64`, `PANIC_GBSV_GF`, `PANIC_GBSV_TUX`, `PANIC_GBSV_ZETA`, `PANIC_IO_ERROR`, `PANIC_SYNTAX_ERROR`, `PANIC_DIVZERO`, `PANIC_ARITHMETIC_OVERFLOW` |
| **`2`** | **Structural Specification Violations** | `PANIC_SPEC_FILENAME_MISMATCH`, `PANIC_SPEC_MISSING_LIBRARY`, `PANIC_SPEC_LINE_CYCLE`, `PANIC_SPEC_CHECKSUM`, `PANIC_SPEC_WHITESPACE_PARITY`, `PANIC_SPEC_BAD_JUMP`, `PANIC_SPEC_NO_HISTORY`, `PANIC_COMPANION_ORPHAN`, `PANIC_COMPANION_HERESY` |
| **`3`** | **Resource & Memory Safety Collapses** | `PANIC_BUDGET_EXHAUSTION`, `PANIC_STACK_GRAVITY_OVERFLOW`, `PANIC_AFFINE_USE_AFTER_MOVE`, `PANIC_TYPE_MISMATCH`, `PANIC_STACK_UNDERFLOW`, `PANIC_MEM_OUT_OF_BOUNDS`, `PANIC_DECODE_COLLAPSE`, `PANIC_MUTATION_COLLAPSE`, `PANIC_GENOME_DIVERGENCE`, `PANIC_EXECUTION_COLLAPSE` |

On successful, fully verified execution, the VM emits:
```text
Execution terminated cleanly. State verified.
```

---

## 9. Binary Companion Specification (`.tu` Container)

In modes requiring cryptographically bound companion files (`--ADVERSARIAL`), the bytecode must be paired with an 88-byte binary file named `<prefix>.tu`.

### Binary Memory Layout

```text
Offset    Size   Field Description
─────────────────────────────────────────────────────────────────
0x00      4 B    Magic Identifier: 0x54 0x55 0x58 0x32 ("TUX2")
0x04      4 B    Format Version:   0x00000002
0x08      8 B    FNV-1a 64-bit Digest of Target .tux Source
0x10      8 B    Initial Entropy Pool Seed
0x18      8 B    Genome Chromosome 0 (G0: Structural Mutation Key)
0x20      8 B    Genome Chromosome 1 (G1: Context B Code Pointer Key)
0x28      8 B    Genome Chromosome 2 (G2: Context B Data Pointer Key)
0x30      8 B    Genome Chromosome 3 (G3: Dormant Resonance Key)
0x38      8 B    Hardware Register R0 Initial State
0x40      8 B    Hardware Register R1 Initial State
0x48      8 B    Hardware Register R2 Initial State
0x50      8 B    Hardware Register R3 Initial State
─────────────────────────────────────────────────────────────────
Total: 88 Bytes strictly aligned.
```

If the companion file is missing, the VM terminates with `PANIC_COMPANION_ORPHAN` (code 2). If the FNV-1a digest does not match the `.tux` file bit-for-bit, it terminates with `PANIC_COMPANION_HERESY` (code 2).

---

## 10. Building, Verification & Tooling

### 10.1. Build System
The runtime is written in ISO C99 and requires a standard POSIX.1-2001 environment (GCC or Clang):

```bash
# Build optimized release binary
make

# Clean compilation artifacts
make clean
```

### 10.2. Formal Test Suite (100% Passing)
The validation framework includes unit, integration, invariant, and regression tests:

```bash
# Run complete test suite (90/90 suites + 74k assertions)
make test
```

#### Test Suite Composition:
1. **De Bruijn & RNS-CRT Verification (`make test-rns`):**
   - **Bézout Modular Inverses:** Complete verification of $C_i \equiv 1 \pmod{m_i}$ and $C_i \equiv 0 \pmod{m_j}$.
   - **CRT Isomorphism:** $100{,}000$ pseudorandom vectors confirming $\text{decode}(\text{encode}(N)) == N$.
   - **Pigeonhole Resolution:** Strict validation of 12-char Radix-6 coverage for $m_5=23$.
   - **Dense Stream Parsing:** Verification of operand parsing without `\0` terminators.
   - **De Bruijn Graph Completeness:** $10{,}584$ state-opcode pairs verified via BFS reachability within $\le 3$ transitions.
   - **Avalanche Diffusion:** 1-symbol mutation entropy verification across the residue ring.
2. **Galois Field Syndrome Verification (`test-gbsv`):** 24/24 tests covering field inverses, generator polynomials, and syndrome sensitivity.
3. **Classic Execution Suite (`tests/run.sh`):** 15/15 tests covering basic arithmetic, branching, and dynamic lists.
4. **Strict Specification Suite (`tests/test_cursed.sh`):** 8/8 tests verifying bit-size naming, whitespace parity, and checksums.
5. **Unified Memory Safety Suite (`tests/test_hardcore.sh`):** 12/12 tests validating affine move semantics, stack gravity, and fuel exhaustion.
6. **Golden Vectors Suite (`tests/test_golden_vectors.sh`):** 22/22 tests verifying deterministic reproducibility of FNV-1a, CRAZY64, and cellular aging.
7. **Adversarial Engine Suite (`tests/test_apocalypse.sh`):** 9/9 tests verifying dual-context scheduling, companion integrity, and thermodynamic undo.

```text
Verification Summary:
  Total Suites:        7 / 7   (100% PASS)
  Individual Suites:   90 / 90 (100% PASS)
  RNS/DeBruijn Checks: 74,307  (100% PASS)
  Compiler Warnings:   0       (-Wall -Wextra -pedantic)
```

### 10.3. Python Toolchain (`tux_helper.py`)
The repository includes a companion compiler and diagnostic CLI:

```bash
# De Bruijn opcode synthesis
python3 tux_helper.py debruijn 8 0x5A
# Outputs: Fixed-3 trajectory and minimal path resolving to OP_PUSH

# RNS operand encoding
python3 tux_helper.py rns-enc 42
# Outputs: 12-character RNS suffix "TTtuTutUTXut"

# RNS operand decoding
python3 tux_helper.py rns-dec TTtuTutUTXut
# Outputs: 42

# Generate valid 88-byte companion container
python3 tux_helper.py companion examples/127928.tux
```

---

## 11. Command-Line Interface

```text
Usage: tuxpl [MODE] [OPTIONS] <source_file>

Execution Modes (Mutually Exclusive):
  --CLASSIC              Classic stack-based execution (Free syntax)
  --STRICT               Structural constraint enforcement (Default)
  --UNIFIED-VM           Unified 64K cellular memory with affine checking
  --ADVERSARIAL          Dual-context adversarial scheduling with .tu companion

Engine Options:
  --gbsv                 Enable Galois-Base64 Syndrome Verification
  --REVERSIBLE           Enable thermodynamic reversible frame buffer
  --disasm               Dump disassembled cellular state before execution
  --help                 Display this academic specification summary
  --version              Display runtime release version
```

---

## 12. Conclusion & Academic Significance

TuxPL 2.0.0 demonstrates that esoteric programming environments can transcend arbitrary syntactical annoyance to establish rigorous mathematical testbeds for computer science research. By unifying finite-field control-flow integrity, modular residue number systems, topological graph routing, and non-preemptive adversarial concurrency, TuxPL establishes an execution domain where correctness is provable, complexity is mathematically bounded, and unauthorized state perturbation is cryptographically impossible.
