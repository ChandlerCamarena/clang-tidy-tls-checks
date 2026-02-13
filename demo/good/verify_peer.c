#include <openssl/ssl.h>

void good_verify_peer(SSL_CTX *ctx) {
    /* T1 good: enable peer verification */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, 0);
}

