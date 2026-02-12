# Toolchain and Architecture (v0.1)

## Goal

Implement a **custom Clang-Tidy module** that detects **high-confidence, AST-detectable TLS/OpenSSL misconfigurations** in C/C++ and reports actionable diagnostics.

This is the initial implementation strategy for the MVT. CodeChecker integration is planned after the checks are stable.

---

## Primary Design Choices

### Analyzer Strategy
- **Framework:** Clang-Tidy (custom module + custom checks)
- **Analysis level:** Clang AST (no whole-program analysis in MVT)
- **Approach:** Pattern-based detection using AST matchers (call sites, argument values, simple callback structure, ignored return values)

### Scope Control
- **Target library (v0.1):** OpenSSL only
- **In-scope evidence:** function calls, argument constants/macros, string literals, direct return-value usage/ignoring
- **Out-of-scope (MVT):** cross-translation-unit dataflow, deep taint tracking, runtime behavior, config file parsing

---

## Data Flow Overview

C/C++ project → compile_commands.json → clang-tidy (with custom module) → diagnostics

### Inputs
- Source files (.c/.cc/.cpp/.h)
- Compilation database (`compile_commands.json`)
- OpenSSL headers available to the compiler (system or project-provided)

### Outputs
- Clang-Tidy diagnostics:
  - location (file:line:column)
  - rule identifier (e.g., `tls-cert-verify-disabled`)
  - explanation of security impact
  - remediation guidance (and fix-it hints when safe)

---

## Component Breakdown

### 1) Custom Clang-Tidy Module
- Registers the TLS checks under a consistent prefix (e.g., `tls-*`)
- Provides configuration hooks via `.clang-tidy` options if needed later

### 2) Individual Checks (Rules)
Each check:
- Identifies a single misconfiguration pattern
- Emits a diagnostic with:
  - **What happened**
  - **Why it matters**
  - **What to do instead**
- Prioritizes **precision over coverage** (low false positives)

### 3) Demo and Evaluation Harness
- Curated “bad vs good” examples in `/demo`
- Repeatable run command (script planned)
- Expected outputs recorded for sanity checks

---

## Planned Integrations (Post-MVT)

### CodeChecker
- After stable checks exist, integrate by:
  - Running clang-tidy via CodeChecker’s analysis pipeline, or
  - Wrapping checks to fit CodeChecker workflows
- Integration is explicitly deferred until the checks are reliable and reproducible.

---

## Near-Term Engineering Milestones

Week 1 implementation targets:
- **T1:** Certificate verification disabled (`SSL_VERIFY_NONE`)
- **T2:** Insecure protocol methods (`SSLv3_method`, `TLSv1_method`, etc.)

Success criteria:
- Two checks implemented end-to-end
- Demo examples trigger correctly
- Diagnostics are clear and actionable

