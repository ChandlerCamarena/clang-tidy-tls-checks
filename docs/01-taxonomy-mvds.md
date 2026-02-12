# Minimum Viable Detection Set (MVDS)

## MVDS v0.1  
**OpenSSL TLS Misconfiguration Patterns**

This document defines the initial taxonomy slice of high-confidence, AST-detectable TLS/OpenSSL misconfiguration patterns.

Week 1 implementation targets are marked with ★.

---

| ID  | Pattern | AST-Detectable Condition | Risk | Safer Alternative |
|-----|---------|--------------------------|------|------------------|
| T1 ★ | Certificate verification disabled | `SSL_CTX_set_verify(..., SSL_VERIFY_NONE, ...)` or `SSL_set_verify(..., SSL_VERIFY_NONE, ...)` | MITM possible | Use `SSL_VERIFY_PEER` and enforce strict verification policy |
| T2 ★ | Insecure protocol method | `SSLv2_method`, `SSLv3_method`, `TLSv1_method`, `TLSv1_1_method` (including client/server variants) | Weak / obsolete TLS | Use `TLS_method()`, `TLS_client_method()`, or `TLS_server_method()` |
| T3 | Accept-all verify callback | Verify callback returns constant `1` or ignores `preverify_ok` | MITM possible | Return `preverify_ok` or enforce explicit validation logic |
| T4 | Verify result ignored | `SSL_get_verify_result(ssl)` return value unused | Verification not enforced | Compare return value against `X509_V_OK` |
| T5 | Handshake result ignored | Return value of `SSL_connect`, `SSL_accept`, or `SSL_do_handshake` unused | Silent failure / insecure fallback | Check return value and handle errors via OpenSSL APIs |
| T6 | Certificate/key load return ignored | Return value of `SSL_CTX_use_certificate_file` or `SSL_CTX_use_PrivateKey_file` unused | Running without intended credentials | Check return value and fail fast on error |
| T7 | Relative CA path | `SSL_CTX_load_verify_locations(ctx, "relative.pem", ...)` with relative string literal | Fragile trust store | Use system trust store or absolute paths |
| T8 | Insecure cipher string (literal) | `SSL_CTX_set_cipher_list(..., "...")` containing `aNULL`, `eNULL`, `EXPORT`, `RC4`, or `MD5` | Weak ciphers | Use modern cipher baseline or rely on secure defaults |
| T9 | Minimum protocol version too low | `SSL_CTX_set_min_proto_version(..., TLS1_VERSION/TLS1_1_VERSION)` | Weak TLS | Require TLS 1.2+ (policy-defined minimum) |
| T10 | Hostname verification explicitly disabled | Detect explicit disable API patterns (where provable via AST) | MITM possible | Enable hostname verification and enforce host checks |

---

## Design Principle

All checks must satisfy:

- High confidence.
- AST-level detectability.
- Clear security rationale.
- Actionable remediation guidance.
- Low false-positive tolerance.

Precision is prioritized over coverage.

