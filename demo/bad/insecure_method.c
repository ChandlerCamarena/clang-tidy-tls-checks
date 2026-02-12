#include <openssl/ssl.h>

const SSL_METHOD *bad_insecure_method(void) {
    /* T2: obsolete protocol method */
    return TLSv1_method();
}

