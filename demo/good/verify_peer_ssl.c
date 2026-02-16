#include <openssl/ssl.h>

void good_verify_peer_ssl(SSL *ssl) {
    /* T1 good variant: enable peer verification on SSL object */
    SSL_set_verify(ssl, SSL_VERIFY_PEER, 0);
}

