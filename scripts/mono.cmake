# Aktifkan modul FetchContent
include(FetchContent)

# Tentukan repositori Mono dan versi (tag/commit/branch) yang spesifik
FetchContent_Declare(
    MonoSDK
    GIT_REPOSITORY https://github.com/mono/mono.git
    GIT_TAG        mono-6.12.0.206 # Ganti dengan tag atau commit yang relevan
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(MonoSDK)