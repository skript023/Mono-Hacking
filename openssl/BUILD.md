# OpenSSL build for DLL unloading

The static OpenSSL 3.2.1 libraries in this directory are rebuilt with
`VC-WIN64A no-shared no-pinshared no-tests zlib /FS`, using the static MSVC
runtime and NASM assembly optimizations. `no-shared` alone does not prevent
OpenSSL from pinning the DLL that links libcrypto. Defining
`OPENSSL_NO_PINSHARED` only in the application does not change a prebuilt library.

Source: https://github.com/openssl/openssl/releases/download/openssl-3.2.1/openssl-3.2.1.tar.gz

Source SHA256: `83c7329fe52c850677d75e5d0b0ca245309b97e8ecbcfdc1dfdc4ab9fac35b39`

Build in a Visual Studio x64 developer shell with native Windows Perl and NASM:

```powershell
./scripts/rebuild-openssl.ps1 -SourceDirectory <extracted-source> -Perl <perl.exe> -NasmDirectory <nasm-directory>
```

Optional `-Make <jom.exe>` enables four parallel jobs. This replaces libcrypto,
libssl and their headers; zlib remains unchanged. The original Conan metadata
records the vendor package, not these rebuilt artifacts.

OpenSSL is linked statically into the project. Shutdown must finish all socket
callbacks, join networking threads, release SSL objects, then call
`OPENSSL_cleanup()` before unloading the DLL. Threads using OpenSSL call
`OPENSSL_thread_stop()` when they finish. Do not apply global cleanup this way
to an OpenSSL DLL shared with the host.

A process that already loaded the old pinned build must be restarted before
checking the replacement DLL; unloading the new build cannot undo an old pin.

References:
- https://github.com/openssl/openssl/issues/20977
- https://github.com/openssl/openssl/blob/openssl-3.2.1/crypto/init.c
