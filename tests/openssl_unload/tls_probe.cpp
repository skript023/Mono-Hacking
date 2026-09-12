#include <openssl/ssl.h>
extern "C" __declspec(dllexport) int exercise_tls() {
    auto* context = SSL_CTX_new(TLS_client_method());
    if (!context) return 0;
    SSL_CTX_free(context);
    OPENSSL_thread_stop();
    OPENSSL_cleanup();
    return 1;
}
