param(
    [Parameter(Mandatory=$true)][string]$SourceDirectory,
    [Parameter(Mandatory=$true)][string]$Perl,
    [Parameter(Mandatory=$true)][string]$NasmDirectory,
    [string]$Make = "nmake.exe"
)
$ErrorActionPreference = "Stop"
# Run from a Visual Studio x64 developer shell. Source must be OpenSSL 3.2.1.
$repoRoot = Split-Path $PSScriptRoot -Parent
$opensslRoot = Join-Path $repoRoot "openssl"
$env:PATH = "$(Split-Path $Perl -Parent);$NasmDirectory;$env:PATH"
Push-Location $SourceDirectory
try {
    & $Perl Configure VC-WIN64A no-shared no-pinshared no-tests zlib /FS `
        "--with-zlib-include=$opensslRoot/include" "--with-zlib-lib=$opensslRoot/lib/zlib.lib"
    if ($LASTEXITCODE -ne 0) { throw "OpenSSL configuration failed" }
    if ([IO.Path]::GetFileName($Make) -eq "jom.exe") { & $Make -j 4 build_libs }
    else { & $Make build_libs }
    if ($LASTEXITCODE -ne 0) { throw "OpenSSL build failed" }
    Copy-Item -LiteralPath libcrypto.lib,libssl.lib -Destination "$opensslRoot/lib"
    Copy-Item -Path include/openssl/*.h -Destination "$opensslRoot/include/openssl"
} finally { Pop-Location }
