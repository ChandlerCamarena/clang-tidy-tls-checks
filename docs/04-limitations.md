# Limitations and Threats to Validity (v0.1)

## Analysis limitations

This project intentionally prioritizes **precision** and **explainability** over broad coverage.

- **AST-local reasoning only:** Checks operate on a single translation unit and do not perform whole-program or cross-file analysis.
- **Limited dataflow:** No interprocedural dataflow or taint tracking is performed in the MVT.
- **Macro / constant folding dependence:** Some detections rely on constant evaluation at compile time; non-constant values may not be flagged.
- **Library scope:** Current checks target OpenSSL TLS APIs only (v0.1).

## False negatives (expected)

The analyzer may miss cases where:
- verification mode is computed dynamically (not a constant)
- protocol selection occurs through wrappers/abstractions not visible as direct OpenSSL calls
- project-specific macros hide the relevant call sites in non-standard ways

## False positives (mitigations)

To keep false positives low, checks:
- match only specific OpenSSL API entry points
- require strong evidence (e.g., verify mode constant equals 0)

## Threats to validity

- **Representativeness:** The demo corpus is intentionally minimal and does not claim to represent all real-world TLS usage.
- **Generalization:** Results from curated examples do not guarantee identical performance on large, heterogeneous codebases.
- **Environment sensitivity:** Header versions and compilation flags can change how code is parsed; results assume a correct compilation database.

