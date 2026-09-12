# TLS and websocket shutdown regression tests

From the repository root, with CMake and Visual Studio available:

```powershell
cmake -S tests/openssl_unload -B out/openssl-unload/test -A x64
cmake --build out/openssl-unload/test --config Release
ctest --test-dir out/openssl-unload/test -C Release --output-on-failure
```

`openssl_unload` loads a small DLL using the project's OpenSSL libraries,
creates and releases a TLS context, cleans up OpenSSL, and unloads the DLL.
It checks that the module is absent, then repeats for ten cycles. The original
vendor libcrypto fails on cycle zero because it pins the module.

`socket_lifecycle` compiles the actual socket_client header using the project's
fetched Asio/SimpleWeb dependencies. Logging and notifications are stubbed so
it runs without the game. It uses a reserved local non-listening port and
checks WS/WSS immediate disconnect, failed connection, repeated disconnect,
reconnect, and destruction across 100 client lifetimes. CTest imposes a
30-second timeout to detect a blocked join. No account/server login occurs.

These tests do not verify live game hooks or successful remote TLS handshakes.
