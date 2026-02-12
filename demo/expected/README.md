# Expected Diagnostics (Week 1)

| File | Expected Check | Notes |
|------|----------------|------|
| demo/bad/disable_verify.c | T1 | `SSL_CTX_set_verify(..., SSL_VERIFY_NONE, ...)` |
| demo/good/verify_peer.c | none | Uses `SSL_VERIFY_PEER` |
| demo/bad/insecure_method.c | T2 | Uses `TLSv1_method()` |
| demo/good/tls_method.c | none | Uses `TLS_method()` |

