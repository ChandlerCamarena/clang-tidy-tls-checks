#include <openssl/ssl.h>

const SSL_METHOD *good_tls_client_method(void) {
    /* T2 good variant: modern client method */
    return TLS_client_method();
}

