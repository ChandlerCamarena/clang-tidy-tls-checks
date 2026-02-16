#include <openssl/ssl.h>

const SSL_METHOD *bad_insecure_client_method(void) {
    /* T2 variant: obsolete client method */
    return TLSv1_client_method();
}

