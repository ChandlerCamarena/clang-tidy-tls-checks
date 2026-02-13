#include <openssl/ssl.h>

void bad_disable_verify(SSL_CTX *ctx) {
    /* T1: certificate verification explicitly disabled */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
}

