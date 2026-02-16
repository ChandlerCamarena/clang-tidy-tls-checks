# Expected Diagnostics (Week 1)

| File | Expected Check | Notes |
|------|----------------|------|
| demo/bad/disable_verify.c | T1 | SSL_CTX_set_verify(..., SSL_VERIFY_NONE, ...) — verify mode evaluates to 0 |
| demo/good/verify_peer.c | none | SSL_CTX_set_verify(..., SSL_VERIFY_PEER, ...) — non-zero verify mode |
| demo/bad/disable_verify_ssl.c | T1 | SSL_set_verify(..., SSL_VERIFY_NONE, ...) — verify mode evaluates to 0 |
| demo/good/verify_peer_ssl.c | none | SSL_set_verify(..., SSL_VERIFY_PEER, ...) — non-zero verify mode |
| demo/bad/insecure_method.c | T2 | TLSv1_method() — obsolete protocol-specific method |
| demo/good/tls_method.c | none | TLS_method() — modern protocol negotiation API |
| demo/bad/insecure_client_method.c | T2 | TLSv1_client_method() — obsolete protocol-specific client method |
| demo/good/tls_client_method.c | none | TLS_client_method() — modern protocol negotiation API |

