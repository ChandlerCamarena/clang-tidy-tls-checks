# Demo Plan (Week 1)

## Purpose

Provide a small, curated set of TLS/OpenSSL examples that:
- Demonstrate **correct detections** on insecure code (“bad”)
- Demonstrate **no detections** on secure alternatives (“good”)
- Are **minimal**, **reproducible**, and **easy to explain** during defense

Design goal: each “bad” file should trigger **one primary warning** to keep the demo unambiguous.

---

## Directory Layout

- `demo/bad/`  — intentionally insecure examples
- `demo/good/` — corrected examples
- `demo/expected/` — notes about which diagnostics should trigger (optional in Week 1)

---

## Week 1 Demo Files (Planned)

### Check T1 (Certificate verification disabled)

1) `demo/bad/disable_verify.c`  
**Expected:** triggers T1  
**Pattern:** `SSL_CTX_set_verify(..., SSL_VERIFY_NONE, ...)` or `SSL_set_verify(..., SSL_VERIFY_NONE, ...)`

2) `demo/good/verify_peer.c`  
**Expected:** no T1 warning  
**Pattern:** uses `SSL_VERIFY_PEER` and does not disable verification

---

### Check T2 (Insecure protocol method)

3) `demo/bad/insecure_method.c`  
**Expected:** triggers T2  
**Pattern:** `SSLv3_method`, `TLSv1_method`, `TLSv1_1_method` (including client/server variants)

4) `demo/good/tls_method.c`  
**Expected:** no T2 warning  
**Pattern:** uses `TLS_method()` / `TLS_client_method()` / `TLS_server_method()`

---

## Demo Rules

- Each file should be ≤ 40 lines.
- Use minimal OpenSSL initialization required to compile.
- Avoid additional risky patterns that could trigger future checks (keep signals clean).
- Every “bad” example should have an obvious “good” counterpart.

---

## How the Demo Will Be Run (Target)

Once the module exists, the demo will be runnable via:

- A single command invoking `clang-tidy` against the `demo/bad` and `demo/good` files
- A compilation database (`compile_commands.json`) or a minimal invocation strategy
- A script in `scripts/` (to be added after the first check compiles)

---

## Week 1 “Pass” Criteria

- `disable_verify.c` triggers exactly one warning (T1)
- `verify_peer.c` triggers no warning for T1
- `insecure_method.c` triggers exactly one warning (T2)
- `tls_method.c` triggers no warning for T2
- Diagnostics explain **what**, **why**, and **how to fix**

