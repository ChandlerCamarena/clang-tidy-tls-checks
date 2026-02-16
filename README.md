## What this repository is

A custom **Clang-Tidy module** that detects **high-confidence TLS/OpenSSL misconfigurations** in C/C++ using **AST-based pattern matching**, with actionable diagnostics and a small curated demo corpus.

## Implemented checks

- `tls-cert-verify-disabled`  
  Flags `SSL_CTX_set_verify(..., SSL_VERIFY_NONE, ...)` or `SSL_set_verify(..., SSL_VERIFY_NONE, ...)`

- `tls-insecure-proto-method`  
  Flags obsolete protocol-specific methods such as `TLSv1_method()` and recommends `TLS_method()` (or client/server variants)

## Quickstart (WSL2/Ubuntu)

### Dependencies
```bash
sudo apt update
sudo apt install -y clang clang-tidy llvm-18-dev libclang-18-dev clang-tools-18 cmake ninja-build libssl-dev

