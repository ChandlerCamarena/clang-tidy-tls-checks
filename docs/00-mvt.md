# Minimum Viable Thesis (MVT)

## Scope Statement

**Minimum Viable Thesis (MVT):**  
A Clang-Tidy–based static analysis extension that detects high-confidence TLS/OpenSSL API misconfigurations in C/C++ and emits actionable diagnostics and safer alternatives, with a small curated taxonomy and a demonstrable evaluation on representative code snippets and production products.

---

## Scope Boundaries

### In Scope

- OpenSSL TLS misuse patterns that are directly detectable from the AST.
- Function calls and argument inspection.
- Simple callback behavior analysis.
- Ignored return value detection.
- High-confidence patterns with low false-positive potential.

### Out of Scope (Week 1)

- Whole-program reasoning across translation units.
- Advanced dataflow analysis.
- Runtime-only behavior.
- Configuration file parsing.
- Network traffic analysis.
- Exploit demonstration.

---

## Success Criteria (Week 1)

- Two checks implemented end-to-end.
- A working demo suite of "bad vs good" examples.
- A written taxonomy slice (8–12 patterns).
- Diagnostics are clear, actionable, and reproducible.

---

## Definition of Done (Thesis Minimum)

The thesis is considered minimally complete when:

- A taxonomy slice (8–12 patterns) is formally defined with rationale.
- A Clang-Tidy module implements at least 4–6 checks by the end of the semester.
- Evaluation demonstrates precision-oriented results (low false positives).
- The analyzer is tested on:
  - The curated demo suite.
  - At least one external codebase or curated corpus.

