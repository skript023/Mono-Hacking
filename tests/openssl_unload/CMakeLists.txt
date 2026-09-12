cmake_minimum_required(VERSION 3.20)
project(openssl_unload LANGUAGES CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../openssl")
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
add_library(tls_probe SHARED tls_probe.cpp)
target_link_libraries(tls_probe PRIVATE OpenSSL::SSL OpenSSL::Crypto Crypt32 Ws2_32 "${OPENSSL_ROOT_DIR}/lib/zlib.lib")
add_executable(unload_test unload_test.cpp)
enable_testing()
add_test(NAME openssl_unload COMMAND unload_test $<TARGET_FILE:tls_probe>)


set(DEPS "${CMAKE_CURRENT_LIST_DIR}/../../out/build/x64-Clang-Release/_deps")
add_executable(socket_lifecycle socket_lifecycle.cpp)
target_compile_features(socket_lifecycle PRIVATE cxx_std_17)
target_include_directories(socket_lifecycle PRIVATE stubs ../../src
    "${DEPS}/websocket-src" "${DEPS}/asio-src/asio/include"
    "${DEPS}/json-src/single_include")
target_link_libraries(socket_lifecycle PRIVATE OpenSSL::SSL OpenSSL::Crypto Crypt32 Ws2_32 "${OPENSSL_ROOT_DIR}/lib/zlib.lib")
add_test(NAME socket_lifecycle COMMAND socket_lifecycle)
set_tests_properties(socket_lifecycle PROPERTIES TIMEOUT 30)
