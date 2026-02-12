#include <openssl/ssl.h>

const SSL_METHOD *good_tls_method(void) {
    /* T2 good: modern method selection */
    return TLS_method();
}

