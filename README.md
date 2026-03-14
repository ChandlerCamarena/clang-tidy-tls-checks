# codechecker-misconfig-checks

**Thesis prototype — Eötvös Loránd University, Faculty of Informatics**  
**Author:** Chandler Camarena | **Supervisor:** Zoltán Porkoláb

This repository contains the static analysis prototype accompanying the MSc thesis
*"Detecting and Explaining Cryptographic Misuse in C/C++ via LLVM/Clang: A
Taxonomy-Driven Integration into CodeChecker"* (Budapest, 2026).

The prototype is an out-of-tree Clang-Tidy plugin that detects security
misconfigurations in C/C++ and integrates with
[CodeChecker](https://github.com/Ericsson/codechecker) for project-scale
evaluation.

---

## Implemented Checks

| Check name | Taxonomy ref | What it detects |
|---|---|---|
| `security-misc-padding-boundary-leak` | §5.4 / Ch. 6–7 | Record objects transferred by value across `TRUST_BOUNDARY`-annotated functions when the type contains ABI-introduced padding bytes that may be uninitialized |

Additional checkers from the CM (cryptographic misuse) taxonomy family will be
added in subsequent commits.

---

## Repository Structure

```
.
├── include/
│   └── trust_boundary.h          # TRUST_BOUNDARY annotation macro
├── src/
│   ├── CMakeLists.txt
│   ├── checks/
│   │   ├── TrustBoundaryPaddingLeakCheck.h
│   │   └── TrustBoundaryPaddingLeakCheck.cpp   # thesis core checker
│   └── module/
│       └── SecurityMiscModule.cpp   # plugin entry point, registers all checks
├── synthetic_benchmarks/
│   ├── trust_boundary/
│   │   └── benchmarks.c             # Appendix A — all 8 validation cases
│   └── cross_family/
│       └── tb_representation_leak_crypto.c  # cross-family DM+CM example
├── scripts/
│   ├── build.sh                  # build the plugin
│   ├── run_codechecker.sh        # full CodeChecker evaluation pipeline
│   └── collect_metrics.py        # parse event log → Tables 8.1–8.3
└── codechecker.json              # CodeChecker analyzer profile
```

---

## Building

### Prerequisites

**Arch Linux**
```bash
sudo pacman -S clang llvm cmake ninja python
```

**Ubuntu 24.04**
```bash
sudo apt install clang-18 llvm-18-dev clang-tidy-18 cmake ninja-build python3
```

Verify that the LLVM version and the `clang-tidy` binary version match —
the plugin and the binary that loads it must be built against the same LLVM:
```bash
clang --version
llvm-config --version
```

### Compile

```bash
bash scripts/build.sh
```

Or manually:
```bash
cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Output: `build/SecurityMiscPlugin.so`

### Verify the plugin loads

```bash
clang-tidy -load build/SecurityMiscPlugin.so \
  -checks='*' -list-checks 2>/dev/null | grep security-misc
```

Expected:
```
security-misc-padding-boundary-leak
```

---

## Running the Padding-Leak Checker

### Step 1 — Annotate trust boundaries in the target project

Copy the annotation header into your project or add it to your include path:

```c
#include "trust_boundary.h"

/* Mark functions whose by-value record arguments cross a trust domain */
TRUST_BOUNDARY int send_packet(struct Packet pkt);
TRUST_BOUNDARY struct Reply receive_reply(void);
```

`TRUST_BOUNDARY` expands to `__attribute__((annotate("trust_boundary")))` under
Clang/GCC and is a no-op everywhere else.

### Step 2 — Run directly with clang-tidy

```bash
clang-tidy \
  -load build/SecurityMiscPlugin.so \
  -checks='-*,security-misc-padding-boundary-leak' \
  your_file.c -- -I./include
```

### Step 3 — Run via CodeChecker (full evaluation pipeline)

Install CodeChecker:
```bash
pip install codechecker
```

Run the evaluation script:
```bash
export PADDING_LEAK_LOG=./events.csv
./scripts/run_codechecker.sh /path/to/project /path/to/compile_commands.json
```

Collect metrics for Tables 8.1–8.3:
```bash
python3 scripts/collect_metrics.py \
  --project myproject \
  --log events.csv \
  --cc cc-results/json/results.json \
  --baseline-ms <baseline> \
  --checker-ms <checker>
```

---

## Synthetic Benchmark Suite (Appendix A)

`synthetic_benchmarks/trust_boundary/benchmarks.c` contains all 8 validation
cases from Appendix A Table A.1:

```bash
clang-tidy \
  -load build/SecurityMiscPlugin.so \
  -checks='-*,security-misc-padding-boundary-leak' \
  synthetic_benchmarks/trust_boundary/benchmarks.c \
  -- -I./include
```

| Case | Scenario | Expected |
|------|----------|----------|
| case01 | Padding present, uninit pass-by-value | **WARN** |
| case02 | Padding present, `memset` zero | no warn |
| case03 | No padding in type | no warn |
| case04 | Padding present, uninit return-by-value | **WARN** |
| case05 | `__attribute__((packed))`, no padding | no warn |
| case06 | Field-wise init only | **WARN** |
| case07 | `= {0}` whole-object init | no warn |
| case08 | Nested struct with padding | **WARN** |

---

## Diagnostic Output

A typical diagnostic looks like:

```
file.c:52:21: warning: [E3] boundary transfer of 'Msg1' (24 bytes, ~11 padding
bytes) by value at trust boundary 'send_msg': the ABI copies the full object
representation including padding bytes that may contain indeterminate or stale
data. Remediation: (a) whole-object zero-init before assignment
(e.g., `Msg1 v = {0};`), or (b) serialise only semantic fields into an explicit
boundary buffer. [security-misc-padding-boundary-leak]
```

**Evidence levels** (thesis §5.6):
- `E3` — padding present + field-wise-only initialization visible (high confidence)
- `E2` — padding present + initialization state not determinable under
  translation-unit visibility (bounded confidence)

---

## Known Limitations

Documented as future work in Chapter 9:

- **`memset` suppression** — whole-object initialization via `memset` is not
  yet detected as a suppression condition (§9.2). Produces a false positive on
  benchmark case02.
- **No interprocedural analysis** — initialization through helper functions is
  not tracked (§9.2).
- **No pointer/alias tracking** — only by-value transfers are modeled (§9.3).
- **Explicit boundaries only** — `TRUST_BOUNDARY` annotations must be added
  manually; boundary inference is future work (§9.1).

---

## License

MIT — see [LICENSE](LICENSE).
