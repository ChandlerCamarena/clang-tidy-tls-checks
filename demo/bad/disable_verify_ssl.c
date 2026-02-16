#include <openssl/ssl.h>

void bad_disable_verify_ssl(SSL *ssl) {
    /* T1 variant: disable verification on SSL object */
    SSL_set_verify(ssl, SSL_VERIFY_NONE, 0);
}

